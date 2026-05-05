#include "renderer.h"
#include "rlgl.h"

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

Renderer InitRenderer(int width, int height, float propsScale) {
    Renderer renderer = {0};
    
    // Create render textures for full and quarter resolution
    renderer.fullResTarget = LoadRenderTexture(width, height);
    renderer.quarterResTarget = LoadRenderTexture(width * propsScale, height * propsScale);
    
    // Apply texture filtering to both render targets with their respective modes
    SetTextureFilter(renderer.fullResTarget.texture, MAIN_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.quarterResTarget.texture, PROPS_TEXTURE_FILTER_MODE);
    
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
    
    // Set default light position
    renderer.lightPosition = (Vector3){0.0f, 6.0f, 0.0f};
    renderer.hasSkybox = false;
    
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

void CompositeFinalFrame(Renderer renderer, int renderedProps, int visibleProps) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Draw the full resolution environment
    DrawTextureRec(renderer.fullResTarget.texture,
                   (Rectangle){ 0, 0, (float)renderer.fullResTarget.texture.width, (float)-renderer.fullResTarget.texture.height }, // Source rec (flip Y)
                   (Vector2){ 0, 0 }, // Position
                   WHITE);
    
    // Draw the quarter resolution props, scaled up
    DrawTexturePro(renderer.quarterResTarget.texture,
                   (Rectangle){ 0, 0, (float)renderer.quarterResTarget.texture.width, (float)-renderer.quarterResTarget.texture.height }, // Source rec (flip Y)
                   (Rectangle){ 0, 0, renderer.fullResTarget.texture.width, renderer.fullResTarget.texture.height }, // Destination rec (scaled to full screen)
                   (Vector2){ 0, 0 }, // Origin
                   0.0f, // Rotation
                   WHITE);
    
    // Draw FPS counter
    DrawFPS(10, 10);
    
    // Draw props statistics
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
    UnloadRenderTexture(renderer.fullResTarget);
    UnloadRenderTexture(renderer.quarterResTarget);
    UnloadShader(renderer.lightingShader);
}
