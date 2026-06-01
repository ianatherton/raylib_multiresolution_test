#include "props.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
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

// Grass quad: two triangles, each vertex is (x, y, u, v).
// Width = 1.0, height = 1.5 baked in.
static const float GRASS_QUAD_VERTS[] = {
    -0.5f, 0.0f,  0.0f, 1.0f,
     0.5f, 0.0f,  1.0f, 1.0f,
     0.5f, 1.5f,  1.0f, 0.0f,
    -0.5f, 0.0f,  0.0f, 1.0f,
     0.5f, 1.5f,  1.0f, 0.0f,
    -0.5f, 1.5f,  0.0f, 0.0f,
};

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
        props.props[i].lastQueryVisible = true;
    }

    // Grass precomputed instance data (indexed [0..billboardCount))
    props.grassInstances      = (GrassInstance*)malloc(billboardCount * sizeof(GrassInstance));
    props.grassVisibleScratch = (GrassInstance*)malloc(billboardCount * sizeof(GrassInstance));

    props.billboardTexture = LoadTexture(billboardTexturePath);
    if (props.billboardTexture.id == 0) {
        printf("Failed to load billboard texture: %s\n", billboardTexturePath);
    } else {
        GenTextureMipmaps(&props.billboardTexture);
        SetTextureFilter(props.billboardTexture, PROPS_TEXTURE_FILTER_MODE);
    }

    props.billboardSourceRec = (Rectangle){ 0.0f, 0.0f, (float)props.billboardTexture.width, (float)props.billboardTexture.height };
    props.billboardSize = (Vector2){ 1.0f, 1.5f };

    // Grass instanced shader + VAO
    props.grassShader = LoadShader("resources/shaders/foliage.vs", "resources/shaders/foliage.fs");
    props.grassMvpLoc    = GetShaderLocation(props.grassShader, "mvp");
    props.grassTimeLoc   = GetShaderLocation(props.grassShader, "time");
    props.grassCamPosLoc = GetShaderLocation(props.grassShader, "cameraPos");
    props.grassTexLoc    = GetShaderLocation(props.grassShader, "texture0");

    glGenVertexArrays(1, &props.grassVAO);
    glGenBuffers(1, &props.grassQuadVBO);
    glGenBuffers(1, &props.grassInstanceVBO);

    glBindVertexArray(props.grassVAO);

    // Base quad VBO: vec2 pos + vec2 uv
    glBindBuffer(GL_ARRAY_BUFFER, props.grassQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GRASS_QUAD_VERTS), GRASS_QUAD_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Instance VBO: GrassInstance (pos xyz, yaw, pitch, speed, phase, maxLean)
    glBindBuffer(GL_ARRAY_BUFFER, props.grassInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(billboardCount * sizeof(GrassInstance)), NULL, GL_DYNAMIC_DRAW);
    size_t stride = sizeof(GrassInstance);
    glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, x));
    glEnableVertexAttribArray(2); glVertexAttribDivisor(2, 1);
    glVertexAttribPointer (3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, yaw));
    glEnableVertexAttribArray(3); glVertexAttribDivisor(3, 1);
    glVertexAttribPointer (4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, pitch));
    glEnableVertexAttribArray(4); glVertexAttribDivisor(4, 1);
    glVertexAttribPointer (5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, speed));
    glEnableVertexAttribArray(5); glVertexAttribDivisor(5, 1);
    glVertexAttribPointer (6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, phase));
    glEnableVertexAttribArray(6); glVertexAttribDivisor(6, 1);
    glVertexAttribPointer (7, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GrassInstance, maxLean));
    glEnableVertexAttribArray(7); glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Rock model
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
            GenTextureMipmaps(&rockDiffuse);
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
            GenTextureMipmaps(&rockNormal);
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

    props.instancedShader = LoadShader(
        "resources/shaders/lighting_instanced.vs",
        "resources/shaders/lighting_rock.fs"
    );
    props.instancedShader.locs[SHADER_LOC_VERTEX_INSTANCE_TX] = 9;
    props.instancedShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(props.instancedShader, "texture0");
    props.instancedShader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(props.instancedShader, "texture1");

    props.rockInstancedMaterials = (Material*)malloc(props.model.materialCount * sizeof(Material));
    for (int i = 0; i < props.model.materialCount; i++) {
        props.rockInstancedMaterials[i] = props.model.materials[i];
        props.rockInstancedMaterials[i].shader = props.instancedShader;
    }

    props.rockTransformBuffer = (Matrix*)malloc(modelCount * sizeof(Matrix));

    // Occlusion proxy
    props.proxyShader = LoadShaderFromMemory(PROXY_VS, PROXY_FS);
    props.proxyMvpLoc = GetShaderLocation(props.proxyShader, "mvp");

    glGenVertexArrays(1, &props.proxyVAO);
    glGenBuffers(1, &props.proxyVBO);
    glBindVertexArray(props.proxyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, props.proxyVBO);
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
    if (index < 0 || index >= props->count) return;
    props->props[index].position = position;
    props->props[index].type = PROP_BILLBOARD;
    props->props[index].dummyHalfExtents = (Vector3){0.20f, 0.75f, 0.20f};
    props->props[index].dummyBounds = BuildDummyBounds(position, props->props[index].dummyHalfExtents);
    props->props[index].isOccluder = false;
    props->props[index].visible = true;

    // Precompute per-blade data (called once at spawn, not every frame)
    float yaw, pitch;
    GrassFieldAngles(position.x, position.z, &yaw, &pitch);
    float randA = HashToUnitFloat((unsigned int)(index * 9781 + 17));
    float randB = HashToUnitFloat((unsigned int)(index * 6271 + 53));
    props->grassInstances[index].x        = position.x;
    props->grassInstances[index].y        = position.y;
    props->grassInstances[index].z        = position.z;
    props->grassInstances[index].yaw      = yaw;
    props->grassInstances[index].pitch    = pitch;
    props->grassInstances[index].speed    = 0.8f + randA * 1.6f;
    props->grassInstances[index].phase    = randB * PI * 2.0f;
    props->grassInstances[index].maxLean  = (5.0f + randA * 11.0f) * DEG2RAD;
}

void AddModelProp(Props* props, Vector3 position, int index) {
    if (index < 0 || index >= props->count) return;
    props->props[index].position = position;
    props->props[index].type = PROP_MODEL;
    props->props[index].dummyHalfExtents = (Vector3){0.45f, 0.55f, 0.45f};
    props->props[index].dummyBounds = BuildDummyBounds(position, props->props[index].dummyHalfExtents);
    props->props[index].isOccluder = true;
    props->props[index].visible = true;
}

void BuildProxyVBO(Props* props) {
    Vector3* positions = (Vector3*)malloc(props->count * sizeof(Vector3));
    for (int i = 0; i < props->count; i++) {
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
        if (!available) continue;
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

        props->props[i].inRange = true;
        props->props[i].visible = props->props[i].lastQueryVisible;
        visibleCount++;
    }

    props->visibleCount = visibleCount;
}

void IssuePropOcclusionQueries(Props* props) {
    rlDrawRenderBatchActive();

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glPointSize(2.0f);

    glUseProgram(props->proxyShader.id);
    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlSetUniformMatrix(props->proxyMvpLoc, mvp);

    glBindVertexArray(props->proxyVAO);

    for (int i = 0; i < props->count; i++) {
        if (!props->props[i].inRange) continue;
        if (props->props[i].queryPending) continue;

        if (props->props[i].occlusionQuery == 0) {
            glGenQueries(1, &props->props[i].occlusionQuery);
        }

        glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, props->props[i].occlusionQuery);
        glDrawArrays(GL_POINTS, i, 1);
        glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
        props->props[i].queryPending = true;
    }

    glBindVertexArray(0);
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

void DrawProps(Props* props, Camera3D camera) {
    props->renderedCount = 0;

    // --- Rocks: instanced draw ---
    int instanceCount = 0;
    Matrix baseTransform = props->model.transform;
    int billboardCount = props->count - props->rockCount;

    for (int i = billboardCount; i < props->count; i++) {
        if (!props->props[i].visible) continue;
        if (!IsPointInFrustum(props->props[i].position, camera, 1.0f)) continue;

        props->renderedCount++;
        int ri = i - billboardCount;
        float modelScaleRand = HashToUnitFloat((unsigned int)(ri * 7919 + 101));
        float scale = 0.38f + modelScaleRand * 0.34f;
        float rotationAngle = (float)((ri * 37) % 360);
        Vector3 pos = props->props[i].position;
        Matrix t = MatrixMultiply(
            MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(rotationAngle * DEG2RAD)),
            MatrixTranslate(pos.x, pos.y, pos.z)
        );
        props->rockTransformBuffer[instanceCount++] = MatrixMultiply(baseTransform, t);
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

    // --- Grass: instanced GPU draw ---
    // Flush rlgl so rocks are in depth buffer before we do raw GL
    rlDrawRenderBatchActive();

    int grassVisible = 0;
    for (int i = 0; i < billboardCount; i++) {
        if (!props->props[i].visible) continue;
        props->grassVisibleScratch[grassVisible++] = props->grassInstances[i];
    }
    props->renderedCount += grassVisible;

    if (grassVisible > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, props->grassInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(grassVisible * sizeof(GrassInstance)), props->grassVisibleScratch);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        float t = fmodf((float)GetTime(), 3600.0f);

        glUseProgram(props->grassShader.id);
        rlSetUniformMatrix(props->grassMvpLoc, mvp);
        glUniform1f(props->grassTimeLoc, t);
        glUniform3f(props->grassCamPosLoc, camera.position.x, camera.position.y, camera.position.z);
        glUniform1i(props->grassTexLoc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, props->billboardTexture.id);

        rlDisableBackfaceCulling();
        glBindVertexArray(props->grassVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grassVisible);
        glBindVertexArray(0);
        rlEnableBackfaceCulling();

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(rlGetShaderIdDefault());
    }
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
    for (int i = 0; i < props->count; i++) {
        if (props->props[i].occlusionQuery != 0) {
            glDeleteQueries(1, &props->props[i].occlusionQuery);
        }
    }

    glDeleteVertexArrays(1, &props->proxyVAO);
    glDeleteBuffers(1, &props->proxyVBO);
    UnloadShader(props->proxyShader);

    glDeleteVertexArrays(1, &props->grassVAO);
    glDeleteBuffers(1, &props->grassQuadVBO);
    glDeleteBuffers(1, &props->grassInstanceVBO);
    UnloadShader(props->grassShader);
    free(props->grassInstances);
    free(props->grassVisibleScratch);

    free(props->rockTransformBuffer);
    free(props->rockInstancedMaterials);
    UnloadShader(props->instancedShader);

    UnloadTexture(props->billboardTexture);
    UnloadModel(props->model);
    free(props->props);
}
