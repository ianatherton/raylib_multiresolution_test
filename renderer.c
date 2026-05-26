#include "renderer.h"
#include "rlgl.h"

// Color + depth texture FBO (raylib LoadRenderTexture uses a depth renderbuffer — not sampleable for DOF)
static RenderTexture2D LoadRenderTextureDepthReadable(int width, int height) {
    RenderTexture2D target = {0};
    target.id = rlLoadFramebuffer();
    if (target.id == 0) return target;

    rlEnableFramebuffer(target.id);

    target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    target.texture.mipmaps = 1;

    target.depth.id = rlLoadTextureDepth(width, height, false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = 19;
    target.depth.mipmaps = 1;

    rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(target.id)) printf("ERROR: Depth-readable FBO incomplete\n");

    rlDisableFramebuffer();
    return target;
}

// Matches GetScreenToWorldRayEx: view = LookAt, proj = Perspective/Ortho, inv(view*proj) for depth unproject
static Matrix DofInvViewProj(Camera3D camera, int fbWidth, int fbHeight) {
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix proj;
    if (camera.projection == CAMERA_ORTHOGRAPHIC) {
        double aspect = (double)fbWidth / (double)fbHeight;
        double top = camera.fovy / 2.0;
        double right = top * aspect;
        proj = MatrixOrtho(-right, right, -top, top, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    } else {
        proj = MatrixPerspective(camera.fovy * DEG2RAD, (double)fbWidth / (double)fbHeight, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    }
    Matrix viewProj = MatrixMultiply(view, proj);
    return MatrixInvert(viewProj);
}

static const char* SKYBOX_VS = "#version 330\n"
"in vec3 vertexPosition;\n"
"out vec3 texCoord;\n"
"uniform mat4 mvp;\n"
"void main(){\n"
"texCoord = vertexPosition;\n"
"vec4 pos = mvp * vec4(vertexPosition, 1.0);\n"
"gl_Position = pos.xyww;\n"
"}\n";

static const char* SKYBOX_FS = "#version 330\n"
"in vec3 texCoord;\n"
"out vec4 finalColor;\n"
"uniform samplerCube environmentMap;\n"
"void main(){\n"
"finalColor = texture(environmentMap, normalize(texCoord));\n"
"}\n";

// Depth write is disabled for the dome; far clip is 4000m so keep radius under that
#define SKY_CLOUD_DOME_RADIUS 3800.0f
// Triplanar tiling — square UVs prevent stretch; tune this one value for cloud density
#define SKY_CLOUD_UV_TILE_X 1.0f   // half tiles = 2x cloud angular size = feels 2x farther
#define SKY_CLOUD_UV_TILE_Y 1.0f
#define SKY_CLOUD_ROT_DEG_PER_SEC 0.7f
#define SKY_CLOUD_LAYER_OPAQUE 0.40f
// Tilted rotation axis: clouds drift diagonally rather than purely horizontal
#define SKY_CLOUD_AXIS_TILT_X 0.40f
#define SKY_CLOUD_AXIS_TILT_Z 0.18f

// Triplanar cloud dome: no seam, no pole-pinch; objDir is the unit sphere direction in object space
static const char* CLOUD_DOME_VS = "#version 330 core\n"
"in vec3 vertexPosition;\n"
"uniform mat4 mvp;\n"
"out vec3 objDir;\n"
"void main(){\n"
"vec3 d = normalize(vertexPosition);\n"
"objDir = vec3(d.z, d.y, -d.x);\n" // 90° Y-axis rotation of the UV frame
"gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

static const char* CLOUD_DOME_FS = "#version 330 core\n"
"in vec3 objDir;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uvScale;\n"
"uniform float layerOpaque;\n"
"uniform float time;\n"
"void main(){\n"
// triplanar blend weights
"vec3 n = abs(objDir);\n"
"n = pow(n, vec3(6.0));\n"
"n /= (n.x + n.y + n.z);\n"
// slow per-axis UV warp — clouds morph and drift at different rates on each face
"float t = time * 0.025;\n"
"float w = 0.06;\n"
"vec2 wYZ = vec2(sin(objDir.y * 2.3 + t * 1.1), cos(objDir.z * 1.7 + t       )) * w;\n"
"vec2 wXZ = vec2(sin(objDir.z * 2.9 + t * 0.8), cos(objDir.x * 2.1 + t * 1.3)) * w;\n"
"vec2 wXY = vec2(sin(objDir.x * 2.5 + t * 1.2), cos(objDir.y * 3.1 + t * 0.7)) * w;\n"
// 5-tap soft blur per axis (15 samples total)
"float b = 0.055;\n"
"vec2 uYZ = objDir.yz * uvScale + wYZ;\n"
"vec2 uXZ = objDir.xz * uvScale + wXZ;\n"
"vec2 uXY = objDir.xy * uvScale + wXY;\n"
"vec4 cx = (texture(texture0, uYZ)"
"         + texture(texture0, uYZ + vec2(b, 0.0))"
"         + texture(texture0, uYZ - vec2(b, 0.0))"
"         + texture(texture0, uYZ + vec2(0.0, b))"
"         + texture(texture0, uYZ - vec2(0.0, b))) * 0.2;\n"
"vec4 cy = (texture(texture0, uXZ)"
"         + texture(texture0, uXZ + vec2(b, 0.0))"
"         + texture(texture0, uXZ - vec2(b, 0.0))"
"         + texture(texture0, uXZ + vec2(0.0, b))"
"         + texture(texture0, uXZ - vec2(0.0, b))) * 0.2;\n"
"vec4 cz = (texture(texture0, uXY)"
"         + texture(texture0, uXY + vec2(b, 0.0))"
"         + texture(texture0, uXY - vec2(b, 0.0))"
"         + texture(texture0, uXY + vec2(0.0, b))"
"         + texture(texture0, uXY - vec2(0.0, b))) * 0.2;\n"
"vec4 col = cx * n.x + cy * n.y + cz * n.z;\n"
"finalColor = vec4(col.rgb, col.a * layerOpaque);\n"
"}\n";

Renderer InitRenderer(int width, int height, float propsScale) {
    Renderer renderer = {0};
    
    // Full / quarter FBOs need sampleable depth for distance-based DOF
    int qw = (int)(width * propsScale);
    int qh = (int)(height * propsScale);
    if (qw < 1) qw = 1;
    if (qh < 1) qh = 1;
    renderer.fullResTarget = LoadRenderTextureDepthReadable(width, height);
    renderer.quarterResTarget = LoadRenderTextureDepthReadable(qw, qh);
    renderer.compositeTarget = LoadRenderTexture(width, height);
    renderer.blurPing = LoadRenderTexture(width, height);
    renderer.blurPong = LoadRenderTexture(width, height);
    
    // Apply texture filtering to both render targets with their respective modes
    SetTextureFilter(renderer.fullResTarget.texture, MAIN_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.quarterResTarget.texture, PROPS_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.fullResTarget.depth, TEXTURE_FILTER_POINT);
    SetTextureFilter(renderer.quarterResTarget.depth, TEXTURE_FILTER_POINT);
    SetTextureFilter(renderer.compositeTarget.texture, MAIN_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.blurPing.texture, MAIN_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.blurPong.texture, MAIN_TEXTURE_FILTER_MODE);
    
    // Load and initialize the lighting shader
    renderer.lightingShader = LoadShader(
        "resources/shaders/lighting.vs",
        "resources/shaders/lighting.fs"
    );
    
    // Check if shader loaded correctly
    if (renderer.lightingShader.id == 0) {
        printf("ERROR: Failed to load lighting shader!\n");
    } else {
        printf("INFO: Lighting shader loaded successfully (ID: %u)\n", renderer.lightingShader.id);
        renderer.lightingShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(renderer.lightingShader, "texture0");
        renderer.lightingShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(renderer.lightingShader, "texture1");
    }

    renderer.dofBlurShader = LoadShader("resources/shaders/dof_blur.vs", "resources/shaders/dof_blur.fs");
    renderer.dofCompositeShader = LoadShader("resources/shaders/dof_composite.vs", "resources/shaders/dof_composite.fs");
    if (renderer.dofBlurShader.id == 0) printf("ERROR: Failed to load DOF blur shader\n");
    if (renderer.dofCompositeShader.id == 0) printf("ERROR: Failed to load DOF composite shader\n");
    
    // Set default light position
    renderer.lightPosition = (Vector3){0.0f, 6.0f, 0.0f};
    renderer.hasSkybox = false;
    renderer.hasCloudDome = false;

    return renderer;
}

bool InitSkybox(Renderer* renderer, const char* pxPath, const char* nxPath, const char* pyPath, const char* nyPath, const char* pzPath, const char* nzPath) {
    Image px = LoadImage(pxPath);
    Image nx = LoadImage(nxPath);
    Image py = LoadImage(pyPath);
    Image ny = LoadImage(nyPath);
    Image pz = LoadImage(pzPath);
    Image nz = LoadImage(nzPath);

    if (px.data == NULL || nx.data == NULL || py.data == NULL || ny.data == NULL || pz.data == NULL || nz.data == NULL) {
        if (px.data != NULL) UnloadImage(px);
        if (nx.data != NULL) UnloadImage(nx);
        if (py.data != NULL) UnloadImage(py);
        if (ny.data != NULL) UnloadImage(ny);
        if (pz.data != NULL) UnloadImage(pz);
        if (nz.data != NULL) UnloadImage(nz);
        printf("ERROR: Failed to load one or more skybox faces\n");
        return false;
    }

    if (px.width != px.height || nx.width != px.width || py.width != px.width || ny.width != px.width || pz.width != px.width || nz.width != px.width) {
        UnloadImage(px);
        UnloadImage(nx);
        UnloadImage(py);
        UnloadImage(ny);
        UnloadImage(pz);
        UnloadImage(nz);
        printf("ERROR: Skybox face sizes must match and be square\n");
        return false;
    }

    int face = px.width;
    Image strip = GenImageColor(face * 6, face, BLACK);
    Rectangle src = {0.0f, 0.0f, (float)face, (float)face};
    ImageDraw(&strip, px, src, (Rectangle){0.0f * face, 0.0f, (float)face, (float)face}, WHITE);
    ImageDraw(&strip, nx, src, (Rectangle){1.0f * face, 0.0f, (float)face, (float)face}, WHITE);
    ImageDraw(&strip, py, src, (Rectangle){2.0f * face, 0.0f, (float)face, (float)face}, WHITE);
    ImageDraw(&strip, ny, src, (Rectangle){3.0f * face, 0.0f, (float)face, (float)face}, WHITE);
    ImageDraw(&strip, pz, src, (Rectangle){4.0f * face, 0.0f, (float)face, (float)face}, WHITE);
    ImageDraw(&strip, nz, src, (Rectangle){5.0f * face, 0.0f, (float)face, (float)face}, WHITE);

    UnloadImage(px);
    UnloadImage(nx);
    UnloadImage(py);
    UnloadImage(ny);
    UnloadImage(pz);
    UnloadImage(nz);

    renderer->skyboxCubemap = LoadTextureCubemap(strip, CUBEMAP_LAYOUT_LINE_HORIZONTAL);
    UnloadImage(strip);
    if (renderer->skyboxCubemap.id == 0) {
        printf("ERROR: Failed to create cubemap texture from skybox faces\n");
        return false;
    }
    SetTextureFilter(renderer->skyboxCubemap, MAIN_TEXTURE_FILTER_MODE);

    renderer->skyboxShader = LoadShaderFromMemory(SKYBOX_VS, SKYBOX_FS);
    if (renderer->skyboxShader.id == 0) {
        UnloadTexture(renderer->skyboxCubemap);
        renderer->skyboxCubemap.id = 0;
        printf("ERROR: Failed to load skybox shader\n");
        return false;
    }

    renderer->skyboxModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    renderer->skyboxModel.materials[0].shader = renderer->skyboxShader;
    renderer->skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = renderer->skyboxCubemap;

    int environmentMapLoc = GetShaderLocation(renderer->skyboxShader, "environmentMap");
    if (environmentMapLoc >= 0) {
        int mapIndex = MATERIAL_MAP_CUBEMAP;
        SetShaderValue(renderer->skyboxShader, environmentMapLoc, &mapIndex, SHADER_UNIFORM_INT);
    }

    renderer->hasSkybox = true;
    printf("INFO: Skybox loaded successfully\n");
    return true;
}

void DrawSkybox(Renderer renderer, Camera3D camera) {
    if (!renderer.hasSkybox) return;
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModel(renderer.skyboxModel, camera.position, 600.0f, WHITE);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

bool InitSkyCloudDome(Renderer* renderer, const char* tilingCloudPngPath) {
    if (renderer == NULL || tilingCloudPngPath == NULL) return false;
    if (renderer->hasCloudDome) return true;

    Image img = LoadImage(tilingCloudPngPath);
    if (img.data == NULL) {
        printf("ERROR: Failed to load cloud dome texture: %s\n", tilingCloudPngPath);
        return false;
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (tex.id == 0) {
        printf("ERROR: Failed to upload cloud dome texture\n");
        return false;
    }

    SetTextureFilter(tex, MAIN_TEXTURE_FILTER_MODE);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);

    Shader sh = LoadShaderFromMemory(CLOUD_DOME_VS, CLOUD_DOME_FS);
    if (sh.id == 0) {
        UnloadTexture(tex);
        printf("ERROR: Failed to load cloud dome shader\n");
        return false;
    }

    sh.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(sh, "texture0");

    Mesh sphere = GenMeshSphere(1.0f, 48, 64);
    Model model = LoadModelFromMesh(sphere);
    model.materials[0].shader = sh;
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE; // rgb/a from texture + layerOpaque in shader

    renderer->cloudDomeModel = model;
    renderer->cloudDomeShader = sh;
    renderer->cloudDomeUvScaleLoc = GetShaderLocation(sh, "uvScale");
    renderer->cloudDomeLayerOpaqueLoc = GetShaderLocation(sh, "layerOpaque");
    renderer->cloudDomeTimeLoc = GetShaderLocation(sh, "time");
    renderer->hasCloudDome = true;

    printf("INFO: Sky cloud dome loaded (%s)\n", tilingCloudPngPath);
    return true;
}

void DrawSkyCloudDome(Renderer renderer, Camera3D camera) {
    if (!renderer.hasCloudDome || renderer.cloudDomeShader.id == 0) return;

    static float cloudYawDeg = 0.0f;
    cloudYawDeg += SKY_CLOUD_ROT_DEG_PER_SEC * GetFrameTime();
    if (cloudYawDeg >= 360.0f) cloudYawDeg -= 360.0f;
    if (cloudYawDeg <= -360.0f) cloudYawDeg += 360.0f;

    Vector2 uvScale = {SKY_CLOUD_UV_TILE_X, SKY_CLOUD_UV_TILE_Y};
    float layerOpaque = SKY_CLOUD_LAYER_OPAQUE;

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE); // acts as a lighten layer: cloud brightness is added to sky behind it
    BeginShaderMode(renderer.cloudDomeShader);
    if (renderer.cloudDomeUvScaleLoc >= 0) {
        SetShaderValue(renderer.cloudDomeShader, renderer.cloudDomeUvScaleLoc, &uvScale, SHADER_UNIFORM_VEC2);
    }
    if (renderer.cloudDomeLayerOpaqueLoc >= 0) {
        SetShaderValue(renderer.cloudDomeShader, renderer.cloudDomeLayerOpaqueLoc, &layerOpaque, SHADER_UNIFORM_FLOAT);
    }
    if (renderer.cloudDomeTimeLoc >= 0) {
        float t = fmodf((float)GetTime(), 3600.0f); // clamp to avoid float precision loss over hours
        SetShaderValue(renderer.cloudDomeShader, renderer.cloudDomeTimeLoc, &t, SHADER_UNIFORM_FLOAT);
    }
    Vector3 driftAxis = Vector3Normalize((Vector3){SKY_CLOUD_AXIS_TILT_X, 1.0f, SKY_CLOUD_AXIS_TILT_Z});
    DrawModelEx( // viewed from inside; tilted axis makes clouds drift diagonally, not just along horizon
        renderer.cloudDomeModel,
        camera.position,
        driftAxis,
        cloudYawDeg,
        (Vector3){SKY_CLOUD_DOME_RADIUS, SKY_CLOUD_DOME_RADIUS, SKY_CLOUD_DOME_RADIUS},
        WHITE
    );
    EndShaderMode();
    EndBlendMode();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void BeginFullResRender(Renderer renderer) {
    BeginTextureMode(renderer.fullResTarget);
    ClearBackground(RAYWHITE); // Clear the render texture
}

void EndFullResRender(void) {
    EndTextureMode();
}

void BeginQuarterResRender(Renderer renderer) {
    BeginTextureMode(renderer.quarterResTarget);
    ClearBackground(BLANK); // Clear with transparency
}

void EndQuarterResRender(void) {
    EndTextureMode();
}

void CompositeFinalFrame(Renderer renderer, Camera3D camera, int renderedProps, int visibleProps) {
    float w = (float)renderer.fullResTarget.texture.width;
    float h = (float)renderer.fullResTarget.texture.height;
    Rectangle fullFlipped = { 0.0f, 0.0f, w, -h };
    Rectangle propsFlipped = { 0.0f, 0.0f, (float)renderer.quarterResTarget.texture.width, (float)-renderer.quarterResTarget.texture.height };
    Rectangle destFull = { 0.0f, 0.0f, w, h };

    BeginTextureMode(renderer.compositeTarget);
    ClearBackground(BLACK);
    DrawTextureRec(renderer.fullResTarget.texture, fullFlipped, (Vector2){ 0.0f, 0.0f }, WHITE);
    DrawTexturePro(renderer.quarterResTarget.texture, propsFlipped, destFull, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    EndTextureMode();

    if (renderer.dofBlurShader.id != 0 && renderer.dofCompositeShader.id != 0) {
        float scale = DOF_GAUSSIAN_PIXEL_SCALE;
        Vector2 texelH = { scale / w, 0.0f };
        Vector2 texelV = { 0.0f, scale / h };
        int locBlurImage = GetShaderLocation(renderer.dofBlurShader, "image");
        int locBlurDir = GetShaderLocation(renderer.dofBlurShader, "texelDir");

        BeginTextureMode(renderer.blurPing);
        ClearBackground(BLANK);
        BeginShaderMode(renderer.dofBlurShader);
        SetShaderValueTexture(renderer.dofBlurShader, locBlurImage, renderer.compositeTarget.texture);
        SetShaderValue(renderer.dofBlurShader, locBlurDir, &texelH, SHADER_UNIFORM_VEC2);
        DrawTextureRec(renderer.compositeTarget.texture, fullFlipped, (Vector2){ 0.0f, 0.0f }, WHITE);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(renderer.blurPong);
        ClearBackground(BLANK);
        BeginShaderMode(renderer.dofBlurShader);
        SetShaderValueTexture(renderer.dofBlurShader, locBlurImage, renderer.blurPing.texture);
        SetShaderValue(renderer.dofBlurShader, locBlurDir, &texelV, SHADER_UNIFORM_VEC2);
        DrawTextureRec(renderer.blurPing.texture, fullFlipped, (Vector2){ 0.0f, 0.0f }, WHITE);
        EndShaderMode();
        EndTextureMode();
    }

    BeginDrawing();
    ClearBackground(BLACK);

    if (renderer.dofBlurShader.id == 0 || renderer.dofCompositeShader.id == 0) {
        DrawTextureRec(renderer.compositeTarget.texture, fullFlipped, (Vector2){ 0.0f, 0.0f }, WHITE);
    } else {
        BeginShaderMode(renderer.dofCompositeShader);
        int locSharp = GetShaderLocation(renderer.dofCompositeShader, "sharpTex");
        int locBlur = GetShaderLocation(renderer.dofCompositeShader, "blurTex");
        int locDs = GetShaderLocation(renderer.dofCompositeShader, "depthScene");
        int locDp = GetShaderLocation(renderer.dofCompositeShader, "depthProps");
        int locPc = GetShaderLocation(renderer.dofCompositeShader, "propsColorTex");
        int locInvVP = GetShaderLocation(renderer.dofCompositeShader, "invViewProj");
        int locCam = GetShaderLocation(renderer.dofCompositeShader, "camPos");
        int locSharpR = GetShaderLocation(renderer.dofCompositeShader, "dofSharpRadiusM");
        int locBlurFull = GetShaderLocation(renderer.dofCompositeShader, "dofBlurFullDistM");
        SetShaderValueTexture(renderer.dofCompositeShader, locSharp, renderer.compositeTarget.texture);
        SetShaderValueTexture(renderer.dofCompositeShader, locBlur, renderer.blurPong.texture);
        SetShaderValueTexture(renderer.dofCompositeShader, locDs, renderer.fullResTarget.depth);
        SetShaderValueTexture(renderer.dofCompositeShader, locDp, renderer.quarterResTarget.depth);
        SetShaderValueTexture(renderer.dofCompositeShader, locPc, renderer.quarterResTarget.texture);
        Matrix invVP = DofInvViewProj(camera, (int)w, (int)h);
        Vector3 camPos = camera.position;
        float sharpR = DOF_SHARP_RADIUS_M;
        float blurFull = DOF_BLUR_FULL_DIST_M;
        SetShaderValueMatrix(renderer.dofCompositeShader, locInvVP, invVP);
        SetShaderValue(renderer.dofCompositeShader, locCam, &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(renderer.dofCompositeShader, locSharpR, &sharpR, SHADER_UNIFORM_FLOAT);
        SetShaderValue(renderer.dofCompositeShader, locBlurFull, &blurFull, SHADER_UNIFORM_FLOAT);
        DrawTextureRec(renderer.compositeTarget.texture, fullFlipped, (Vector2){ 0.0f, 0.0f }, WHITE);
        EndShaderMode();
    }

    DrawFPS(10, 10);
    DrawText(TextFormat("Rendered Props: %d/%d (%.1f%%)",
             renderedProps, visibleProps,
             visibleProps > 0 ? (float)renderedProps / visibleProps * 100.0f : 0),
             10, 40, 20, WHITE);

    EndDrawing();
}

void UnloadRenderer(Renderer renderer) {
    if (renderer.hasSkybox) {
        UnloadModel(renderer.skyboxModel);
        UnloadShader(renderer.skyboxShader);
    }
    if (renderer.hasCloudDome) {
        UnloadModel(renderer.cloudDomeModel);
        UnloadShader(renderer.cloudDomeShader);
    }
    UnloadRenderTexture(renderer.fullResTarget);
    UnloadRenderTexture(renderer.quarterResTarget);
    UnloadRenderTexture(renderer.compositeTarget);
    UnloadRenderTexture(renderer.blurPing);
    UnloadRenderTexture(renderer.blurPong);
    UnloadShader(renderer.dofBlurShader);
    UnloadShader(renderer.dofCompositeShader);
    UnloadShader(renderer.lightingShader);
}
