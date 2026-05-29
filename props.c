#include "props.h"
#include <stdlib.h>
#include <string.h>
#include "rlgl.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

static BoundingBox BuildDummyBounds(Vector3 position, Vector3 halfExtents) {
    return (BoundingBox){
        .min = (Vector3){position.x - halfExtents.x, position.y - halfExtents.y, position.z - halfExtents.z},
        .max = (Vector3){position.x + halfExtents.x, position.y + halfExtents.y, position.z + halfExtents.z}
    };
}

// Minimal pass-through shader for single-point proxy draws.
// MVP is computed from the current rlgl modelview/projection matrices.
static const char* PROXY_VS =
    "#version 330 core\n"
    "layout(location = 0) in vec3 vertexPosition;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char* PROXY_FS =
    "#version 330 core\n"
    "out vec4 finalColor;\n"
    "void main() { finalColor = vec4(1.0); }\n";

Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath, const char* modelNormalMapPath, Shader lightingShader) {
    Props props = {0};
    props.rockHasNormalMap = false;
    props.rockCount = modelCount;
    int totalCount = billboardCount + modelCount;

    props.props = (Prop*)malloc(totalCount * sizeof(Prop));
    props.count = totalCount;

    for (int i = 0; i < totalCount; i++) {
        props.props[i].visible = false;
        props.props[i].position = (Vector3){ 0.0f, 0.0f, 0.0f };
        props.props[i].dummyHalfExtents = (Vector3){0.25f, 0.75f, 0.25f};
        props.props[i].dummyBounds = BuildDummyBounds(props.props[i].position, props.props[i].dummyHalfExtents);
        props.props[i].isOccluder = false;
        props.props[i].occlusionQuery = 0;
        props.props[i].queryPending = false;
        props.props[i].lastQueryVisible = true;  // show until first GPU result arrives
    }

    props.billboardTexture = LoadTexture(billboardTexturePath);
    if (props.billboardTexture.id == 0) {
        printf("Failed to load billboard texture: %s\n", billboardTexturePath);
    } else {
        SetTextureFilter(props.billboardTexture, PROPS_TEXTURE_FILTER_MODE);
    }

    props.billboardSourceRec = (Rectangle){ 0.0f, 0.0f, (float)props.billboardTexture.width, (float)props.billboardTexture.height };
    props.billboardSize = (Vector2){ 1.0f, 1.5f };

    props.model = LoadModel(modelPath);
    if (props.model.meshCount == 0) {
        printf("Failed to load rock model: %s\n", modelPath);
    }

    for (int mi = 0; mi < props.model.meshCount; mi++) {
        GenMeshTangents(&props.model.meshes[mi]);
        UploadMesh(&props.model.meshes[mi], false);
    }

    Texture2D rockDiffuse = {0};
    if (modelTexturePath != NULL && strlen(modelTexturePath) > 0) {
        rockDiffuse = LoadTexture(modelTexturePath);
        if (rockDiffuse.id == 0) {
            printf("Failed to load rock texture: %s\n", modelTexturePath);
        } else {
            SetTextureFilter(rockDiffuse, PROPS_TEXTURE_FILTER_MODE);
            SetTextureWrap(rockDiffuse, TEXTURE_WRAP_REPEAT);
            printf("Rock texture applied: %s (ID: %u)\n", modelTexturePath, rockDiffuse.id);
        }
    }

    Texture2D rockNormal = {0};
    if (modelNormalMapPath != NULL && strlen(modelNormalMapPath) > 0) {
        rockNormal = LoadTexture(modelNormalMapPath);
        if (rockNormal.id == 0) {
            printf("Failed to load rock normal map: %s\n", modelNormalMapPath);
        } else {
            SetTextureFilter(rockNormal, PROPS_TEXTURE_FILTER_MODE);
            SetTextureWrap(rockNormal, TEXTURE_WRAP_REPEAT);
            props.rockHasNormalMap = true;
            printf("Rock normal map: %s (ID: %u)\n", modelNormalMapPath, rockNormal.id);
        }
    }

    if (props.model.materialCount > 0 && props.model.materials != NULL) {
        for (int i = 0; i < props.model.materialCount; i++) {
            if (rockDiffuse.id > 0) {
                props.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = rockDiffuse;
                props.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
            if (rockNormal.id > 0) {
                props.model.materials[i].maps[MATERIAL_MAP_NORMAL].texture = rockNormal;
            }
            props.model.materials[i].shader = lightingShader;
        }
        printf("Rock model: %d materials, %d meshes\n", props.model.materialCount, props.model.meshCount);
    }
    ApplyTextureFilterToAllMaterialMaps(props.model, PROPS_TEXTURE_FILTER_MODE);

    // Instanced shader for rock batch draws (same fragment shader, instanced vertex shader)
    props.instancedShader = LoadShader(
        "resources/shaders/lighting_instanced.vs",
        "resources/shaders/lighting_rock.fs"
    );
    props.instancedShader.locs[SHADER_LOC_VERTEX_INSTANCE_TX] = 9;  // RL_DEFAULT_SHADER_ATTRIB_LOCATION_INSTANCE_TX
    props.instancedShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(props.instancedShader, "texture0");
    props.instancedShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(props.instancedShader, "texture1");

    props.foliageShader    = LoadShader("resources/shaders/foliage.vs",
                                        "resources/shaders/foliage.fs");
    props.foliageCamPosLoc = GetShaderLocation(props.foliageShader, "cameraPos");

    // Clone rock materials with the instanced shader so DrawMeshInstanced can use them
    props.rockInstancedMaterials = (Material*)malloc(props.model.materialCount * sizeof(Material));
    for (int i = 0; i < props.model.materialCount; i++) {
        props.rockInstancedMaterials[i] = props.model.materials[i];
        props.rockInstancedMaterials[i].shader = props.instancedShader;
    }

    // Scratch buffer for per-frame visible instance transforms (worst case: all rocks visible)
    props.rockTransformBuffer = (Matrix*)malloc(modelCount * sizeof(Matrix));

    // Proxy occlusion shader and GPU objects
    props.proxyShader = LoadShaderFromMemory(PROXY_VS, PROXY_FS);
    props.proxyMvpLoc = GetShaderLocation(props.proxyShader, "mvp");

    glGenVertexArrays(1, &props.proxyVAO);
    glGenBuffers(1, &props.proxyVBO);
    glBindVertexArray(props.proxyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, props.proxyVBO);
    // Allocate VBO for totalCount vec3 positions; filled later by BuildProxyVBO
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(totalCount * sizeof(Vector3)), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    props.visibleCount = 0;
    props.renderedCount = 0;

    return props;
}

void AddBillboardProp(Props* props, Vector3 position, int index) {
    if (index >= 0 && index < props->count) {
        props->props[index].position = position;
        props->props[index].type = PROP_BILLBOARD;
        props->props[index].dummyHalfExtents = (Vector3){0.20f, 0.75f, 0.20f};
        props->props[index].dummyBounds = BuildDummyBounds(position, props->props[index].dummyHalfExtents);
        props->props[index].isOccluder = false;
        props->props[index].visible = true;
    }
}

void AddModelProp(Props* props, Vector3 position, int index) {
    if (index >= 0 && index < props->count) {
        props->props[index].position = position;
        props->props[index].type = PROP_MODEL;
        props->props[index].dummyHalfExtents = (Vector3){0.45f, 0.55f, 0.45f};
        props->props[index].dummyBounds = BuildDummyBounds(position, props->props[index].dummyHalfExtents);
        props->props[index].isOccluder = true;
        props->props[index].visible = true;
    }
}

void BuildProxyVBO(Props* props) {
    Vector3* positions = (Vector3*)malloc(props->count * sizeof(Vector3));
    for (int i = 0; i < props->count; i++) {
        // Use the vertical center of the proxy volume as the test point
        positions[i] = (Vector3){
            props->props[i].position.x,
            props->props[i].position.y + props->props[i].dummyHalfExtents.y,
            props->props[i].position.z
        };
    }
    glBindBuffer(GL_ARRAY_BUFFER, props->proxyVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(props->count * sizeof(Vector3)), positions, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(positions);
}

void ReadPropOcclusionResults(Props* props) {
    for (int i = 0; i < props->count; i++) {
        if (!props->props[i].queryPending) continue;
        GLuint available = 0;
        glGetQueryObjectuiv(props->props[i].occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) continue;  // don't stall; keep last result for this frame
        GLuint result = 0;
        glGetQueryObjectuiv(props->props[i].occlusionQuery, GL_QUERY_RESULT, &result);
        props->props[i].lastQueryVisible = (result > 0);
        props->props[i].queryPending = false;
    }
}

void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera) {
    (void)scene;
    int visibleCount = 0;

    for (int i = 0; i < props->count; i++) {
        if (props->props[i].position.x == 0 &&
            props->props[i].position.y == 0 &&
            props->props[i].position.z == 0) continue;

        float maxDistance = (props->props[i].type == PROP_MODEL) ? LOS_MAX_ROCK_DISTANCE : LOS_MAX_GRASS_DISTANCE;
        if (Vector3Distance(camera.position, props->props[i].position) > maxDistance) {
            props->props[i].visible = false;
            props->props[i].inRange = false;
            continue;
        }

        // In range: trust the last GPU occlusion result (true by default until first query)
        props->props[i].inRange = true;
        props->props[i].visible = props->props[i].lastQueryVisible;
        visibleCount++;
    }

    props->visibleCount = visibleCount;
}

// Called inside BeginMode3D in the full-res pass, after all scene geometry has been drawn.
// Flushes the rlgl batch so terrain is in the depth buffer, then issues one GL_POINTS draw
// per in-range prop wrapped in a conservative occlusion query.  Color writes and depth writes
// are both disabled so proxies are invisible and don't disturb the depth buffer.
void IssuePropOcclusionQueries(Props* props) {
    // Flush rlgl batch so terrain/scene depth is committed before our queries
    rlDrawRenderBatchActive();

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glPointSize(2.0f);

    glUseProgram(props->proxyShader.id);
    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlSetUniformMatrix(props->proxyMvpLoc, mvp);

    glBindVertexArray(props->proxyVAO);

    for (int i = 0; i < props->count; i++) {
        if (!props->props[i].inRange) continue;     // distance-culled; skip
        if (props->props[i].queryPending) continue; // last query not yet consumed; reuse result

        if (props->props[i].occlusionQuery == 0) {
            glGenQueries(1, &props->props[i].occlusionQuery);
        }

        glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, props->props[i].occlusionQuery);
        glDrawArrays(GL_POINTS, i, 1);
        glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
        props->props[i].queryPending = true;
    }

    glBindVertexArray(0);

    // Restore GL state so subsequent rlgl draws are unaffected
    glUseProgram(rlGetShaderIdDefault());
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPointSize(1.0f);
}

bool IsPointInFrustum(Vector3 point, Camera3D camera, float margin) {
    Matrix viewMatrix = MatrixLookAt(camera.position, camera.target, camera.up);
    Vector3 viewSpacePoint = Vector3Transform(point, viewMatrix);

    if (viewSpacePoint.z > 0) return false;

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float nearPlaneHeight = 2.0f * fabsf(viewSpacePoint.z) * tanf(camera.fovy * 0.5f * DEG2RAD);
    float nearPlaneWidth = nearPlaneHeight * aspect;

    nearPlaneWidth += margin;
    nearPlaneHeight += margin;

    return (fabsf(viewSpacePoint.x) < nearPlaneWidth * 0.5f) &&
           (fabsf(viewSpacePoint.y) < nearPlaneHeight * 0.5f);
}

typedef struct {
    int index;
    float distance;
} BillboardDepthInfo;

int CompareBillboardDepth(const void* a, const void* b) {
    BillboardDepthInfo* billboardA = (BillboardDepthInfo*)a;
    BillboardDepthInfo* billboardB = (BillboardDepthInfo*)b;
    if (billboardA->distance > billboardB->distance) return -1;
    if (billboardA->distance < billboardB->distance) return 1;
    return 0;
}

static float HashToUnitFloat(unsigned int x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return (float)(x & 0x00FFFFFFU) / 16777215.0f;
}

static void GrassFieldAngles(float x, float z, float* yaw, float* pitch) {
    float nx = x * 0.026f + z * 0.014f;
    float nz = z * 0.023f - x * 0.018f;
    float a = sinf(nx) * 0.72f + sinf(nx * 0.47f + nz * 0.31f) * 0.28f;
    float b = cosf(nz) * 0.68f + cosf(nx * 0.55f - nz * 0.42f) * 0.32f;
    *yaw = (a * 0.92f + b * 0.55f) * PI;
    float c = sinf(nz * 1.15f + nx * 0.74f) * 0.62f + cosf(nx * 1.08f) * 0.38f;
    *pitch = c * (16.0f * DEG2RAD);
}

static void DrawGrassTexturedPlane(Vector3 baseCenter, Texture2D tex, Rectangle source, Vector2 size, float yaw, float pitch, float leanAx, float leanAz, Color tint) {
    float w = size.x;
    float h = size.y;
    Vector3 bl = {-w * 0.5f, 0.0f, 0.0f};
    Vector3 br = {w * 0.5f, 0.0f, 0.0f};
    Vector3 tr = {w * 0.5f, h, 0.0f};
    Vector3 tl = {-w * 0.5f, h, 0.0f};
    Matrix spatial = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
    Matrix lean = MatrixMultiply(MatrixRotateZ(leanAz), MatrixRotateX(leanAx));
    Matrix orient = MatrixMultiply(lean, spatial);
    bl = Vector3Add(baseCenter, Vector3Transform(bl, orient));
    br = Vector3Add(baseCenter, Vector3Transform(br, orient));
    tr = Vector3Add(baseCenter, Vector3Transform(tr, orient));
    tl = Vector3Add(baseCenter, Vector3Transform(tl, orient));
    float tw = (float)tex.width;
    float th = (float)tex.height;
    Vector2 uv0 = {source.x / tw, (source.y + source.height) / th};
    Vector2 uv1 = {(source.x + source.width) / tw, (source.y + source.height) / th};
    Vector2 uv2 = {(source.x + source.width) / tw, source.y / th};
    Vector2 uv3 = {source.x / tw, source.y / th};
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    rlTexCoord2f(uv0.x, uv0.y); rlVertex3f(bl.x, bl.y, bl.z);
    rlTexCoord2f(uv1.x, uv1.y); rlVertex3f(br.x, br.y, br.z);
    rlTexCoord2f(uv2.x, uv2.y); rlVertex3f(tr.x, tr.y, tr.z);
    rlTexCoord2f(uv3.x, uv3.y); rlVertex3f(tl.x, tl.y, tl.z);
    rlTexCoord2f(uv0.x, uv0.y); rlVertex3f(bl.x, bl.y, bl.z);
    rlTexCoord2f(uv3.x, uv3.y); rlVertex3f(tl.x, tl.y, tl.z);
    rlTexCoord2f(uv2.x, uv2.y); rlVertex3f(tr.x, tr.y, tr.z);
    rlTexCoord2f(uv1.x, uv1.y); rlVertex3f(br.x, br.y, br.z);
    rlEnd();
    rlSetTexture(0);
}

void DrawProps(Props* props, Camera3D camera) {
    props->renderedCount = 0;

    BillboardDepthInfo* visibleBillboards = (BillboardDepthInfo*)malloc(props->count * sizeof(BillboardDepthInfo));
    int billboardCount = 0;
    int instanceCount = 0;
    Matrix baseTransform = props->model.transform;

    for (int i = 0; i < props->count; i++) {
        if (!props->props[i].visible) continue;
        if (!IsPointInFrustum(props->props[i].position, camera, 1.0f)) continue;

        props->renderedCount++;

        if (props->props[i].type == PROP_BILLBOARD) {
            visibleBillboards[billboardCount].index = i;
            visibleBillboards[billboardCount].distance = Vector3Distance(camera.position, props->props[i].position);
            billboardCount++;
        } else if (props->props[i].type == PROP_MODEL) {
            float modelScaleRand = HashToUnitFloat((unsigned int)(i * 7919 + 101));
            float scale = 0.38f + modelScaleRand * 0.34f;
            float rotationAngle = (float)((i * 37) % 360);
            Vector3 pos = props->props[i].position;
            Matrix t = MatrixMultiply(
                MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(rotationAngle * DEG2RAD)),
                MatrixTranslate(pos.x, pos.y, pos.z)
            );
            props->rockTransformBuffer[instanceCount++] = MatrixMultiply(baseTransform, t);
        }
    }

    if (instanceCount > 0) {
        for (int mi = 0; mi < props->model.meshCount; mi++) {
            int matIdx = props->model.meshMaterial[mi];
            DrawMeshInstanced(props->model.meshes[mi],
                              props->rockInstancedMaterials[matIdx],
                              props->rockTransformBuffer,
                              instanceCount);
        }
    }

    if (billboardCount > 0) {
        qsort(visibleBillboards, billboardCount, sizeof(BillboardDepthInfo), CompareBillboardDepth);

        float t = (float)GetTime();
        BeginShaderMode(props->foliageShader);
        SetShaderValue(props->foliageShader, props->foliageCamPosLoc, &camera.position, SHADER_UNIFORM_VEC3);
        for (int i = 0; i < billboardCount; i++) {
            int index = visibleBillboards[i].index;
            Vector3 p = props->props[index].position;
            float yaw = 0.0f, pitch = 0.0f;
            GrassFieldAngles(p.x, p.z, &yaw, &pitch);
            float randA = HashToUnitFloat((unsigned int)(index * 9781 + 17));
            float randB = HashToUnitFloat((unsigned int)(index * 6271 + 53));
            float speed = 0.8f + randA * 1.6f;
            float phase = randB * PI * 2.0f;
            float maxLeanRad = (5.0f + randA * 11.0f) * DEG2RAD;
            float leanAx = sinf(t * speed + phase) * maxLeanRad;
            float leanAz = cosf(t * (speed * 0.73f) + phase * 1.37f) * maxLeanRad * 0.48f;
            DrawGrassTexturedPlane(p, props->billboardTexture, props->billboardSourceRec, props->billboardSize, yaw, pitch, leanAx, leanAz, WHITE);
        }
        EndShaderMode();
    }

    free(visibleBillboards);
}

void DrawPropsDebug(Props* props, Camera3D camera) {
    for (int i = 0; i < props->count; i++) {
        if (props->props[i].position.x == 0.0f &&
            props->props[i].position.y == 0.0f &&
            props->props[i].position.z == 0.0f) continue;

        float maxDistance = (props->props[i].type == PROP_MODEL) ? LOS_MAX_ROCK_DISTANCE : LOS_MAX_GRASS_DISTANCE;
        float distance = Vector3Distance(camera.position, props->props[i].position);
        if (distance > maxDistance) continue;

        Color rayColor = props->props[i].visible ? GREEN : RED;
        DrawLine3D(camera.position, props->props[i].position, rayColor);

        Color sphereColor = props->props[i].type == PROP_BILLBOARD ? BLUE : YELLOW;
        DrawSphere(props->props[i].position, 0.1f, sphereColor);

        Color bboxColor = props->props[i].isOccluder ? ORANGE : SKYBLUE;
        DrawBoundingBox(props->props[i].dummyBounds, bboxColor);
    }
}

void UnloadProps(Props* props) {
    // Delete per-prop occlusion query objects
    for (int i = 0; i < props->count; i++) {
        if (props->props[i].occlusionQuery != 0) {
            glDeleteQueries(1, &props->props[i].occlusionQuery);
        }
    }

    glDeleteVertexArrays(1, &props->proxyVAO);
    glDeleteBuffers(1, &props->proxyVBO);
    UnloadShader(props->proxyShader);

    free(props->rockTransformBuffer);
    free(props->rockInstancedMaterials);
    UnloadShader(props->instancedShader);
    UnloadShader(props->foliageShader);

    UnloadTexture(props->billboardTexture);
    UnloadModel(props->model);
    free(props->props);
}
