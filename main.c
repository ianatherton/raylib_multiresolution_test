#include "common.h"
#include "scene.h"
#include "props.h"
#include "renderer.h"
#include "lighting.h"
#include "character.h"
#include <stdlib.h>
#include <time.h>

int main(void) {
    Light light = {
        .position = (Vector3){0.0f, 6.0f, 0.0f},
        .color = WHITE,
        .intensity = 1.0f
    };

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib First Person Demo");
    SetTextureFilter(GetFontDefault().texture, MAIN_TEXTURE_FILTER_MODE);

    GameState gameState = {0};
    gameState.camera.position   = (Vector3){ 0.0f, 2.0f, 4.0f };
    gameState.camera.target     = (Vector3){ 0.0f, 1.8f, 0.0f };
    gameState.camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    gameState.camera.fovy       = 60.0f;
    gameState.camera.projection = CAMERA_PERSPECTIVE;
    gameState.showDebugBoxes    = false;

    Renderer renderer = InitRenderer();
    InitSkybox(
        &renderer,
        "raw-assets/skybox_clear/sky_105_cubemap_2k/px.png",
        "raw-assets/skybox_clear/sky_105_cubemap_2k/nx.png",
        "raw-assets/skybox_clear/sky_105_cubemap_2k/py.png",
        "raw-assets/skybox_clear/sky_105_cubemap_2k/ny.png",
        "raw-assets/skybox_clear/sky_105_cubemap_2k/pz.png",
        "raw-assets/skybox_clear/sky_105_cubemap_2k/nz.png"
    );
    InitSkyCloudDome(&renderer, "raw-assets/tiling_sky_clouds01.png");

    float roomWidth = 500.0f;
    float roomLength = 500.0f;
    float wallHeight = 8.0f;
    float wallThickness = 0.2f;

    unsigned int terrainSeed = (unsigned int)time(NULL);
    Scene scene = InitScene(roomWidth, roomLength, wallHeight, wallThickness,
                           "raw-assets/tiling_dungeon_brickwall01.png",
                           "raw-assets/tiling_dungeon_floor01.png",
                           renderer.lightingShader,
                           terrainSeed);

    const int numGrassProps = 200000;
    const int numRockProps  = 40000;
    const int totalProps    = numGrassProps + numRockProps;

    srand(time(NULL));

    Props props = InitProps(
        numGrassProps,
        numRockProps,
        "raw-assets/grass01_c.png",
        "raw-assets/rock.glb",
        "raw-assets/tilingrock02_c.png",
        "raw-assets/tilingrock02_n.png",
        renderer.lightingShader
    );

    float marginFromWall = 1.0f;
    float minX = -roomWidth/2 + marginFromWall;
    float maxX =  roomWidth/2 - marginFromWall;
    float minZ = -roomLength/2 + marginFromWall;
    float maxZ =  roomLength/2 - marginFromWall;

    for (int i = 0; i < numGrassProps; i++) {
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
        float terrainY = GetTerrainHeightAt(scene, x, z);
        AddBillboardProp(&props, (Vector3){ x, terrainY + 0.05f, z }, i);
    }

    for (int i = 0; i < numRockProps; i++) {
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
        float terrainY = GetTerrainHeightAt(scene, x, z);
        AddModelProp(&props, (Vector3){ x, terrainY + PROPS_ROCK_Y_OFFSET, z }, numGrassProps + i);
    }

    printf("Created %d grass props and %d rock props (total: %d)\n",
           numGrassProps, numRockProps, totalProps);

    BuildProxyVBO(&props);

    // Cache shader uniform locations once (not per frame)
    int locLightPos      = GetShaderLocation(renderer.lightingShader, "lightPos");
    int locLightColor    = GetShaderLocation(renderer.lightingShader, "lightColor");
    int locViewPos       = GetShaderLocation(renderer.lightingShader, "viewPos");
    int locUvScale       = GetShaderLocation(renderer.lightingShader, "uvScale");
    int locUseNormalMap  = GetShaderLocation(renderer.lightingShader, "useNormalMap");
    int locUseMetalRough = GetShaderLocation(renderer.lightingShader, "useMetalRough");
    int locUseParallax   = GetShaderLocation(renderer.lightingShader, "useParallax");
    int locParallaxScale = GetShaderLocation(renderer.lightingShader, "parallaxScale");

    int iLocLightPos      = GetShaderLocation(props.instancedShader, "lightPos");
    int iLocLightColor    = GetShaderLocation(props.instancedShader, "lightColor");
    int iLocViewPos       = GetShaderLocation(props.instancedShader, "viewPos");
    int iLocUvScale       = GetShaderLocation(props.instancedShader, "uvScale");
    int iLocUseNormalMap  = GetShaderLocation(props.instancedShader, "useNormalMap");
    int iLocUseMetalRough = GetShaderLocation(props.instancedShader, "useMetalRough");

    Character character = InitCharacter(renderer.lightingShader);
    float charX = 0.0f, charZ = -5.0f;
    character.position = (Vector3){ charX, GetTerrainHeightAt(scene, charX, charZ), charZ };
    character.yaw = 0.0f;

    DisableCursor();

    static const float parallaxLevels[5] = { 0.0f, 0.02f, 0.06f, 0.12f, 0.24f };
    float parallaxScale = parallaxLevels[2];

    float camYaw   = 0.0f;
    float camPitch = 20.0f;
    float camDist  = 5.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        ReadPropOcclusionResults(&props);
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F1)) gameState.showDebugBoxes = !gameState.showDebugBoxes;

        if (IsKeyPressed(KEY_ONE))   parallaxScale = parallaxLevels[0];
        if (IsKeyPressed(KEY_TWO))   parallaxScale = parallaxLevels[1];
        if (IsKeyPressed(KEY_THREE)) parallaxScale = parallaxLevels[2];
        if (IsKeyPressed(KEY_FOUR))  parallaxScale = parallaxLevels[3];
        if (IsKeyPressed(KEY_FIVE))  parallaxScale = parallaxLevels[4];

        Vector2 mouseDelta = GetMouseDelta();
        camYaw   -= mouseDelta.x * 0.2f;
        camPitch += mouseDelta.y * 0.2f;
        camPitch  = Clamp(camPitch, 5.0f, 75.0f);

        float sy = sinf(camYaw * DEG2RAD);
        float cy = cosf(camYaw * DEG2RAD);
        float moveDX = 0.0f, moveDZ = 0.0f;
        if (IsKeyDown(KEY_W)) { moveDX -= sy; moveDZ -= cy; }
        if (IsKeyDown(KEY_S)) { moveDX += sy; moveDZ += cy; }
        if (IsKeyDown(KEY_A)) { moveDX -= cy; moveDZ += sy; }
        if (IsKeyDown(KEY_D)) { moveDX += cy; moveDZ -= sy; }

        bool shifting = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool moving = (moveDX != 0.0f || moveDZ != 0.0f);
        if (moving) {
            float len = sqrtf(moveDX*moveDX + moveDZ*moveDZ);
            moveDX /= len;
            moveDZ /= len;
            float moveSpeed = shifting ? 4.0f * 1.6f : 4.0f;
            character.position.x += moveDX * moveSpeed * dt;
            character.position.z += moveDZ * moveSpeed * dt;
            if (shifting) {
                SetCharacterAnim(&character, CHAR_ANIM_RUN);
                character.animFPS = 30.0f;
            } else {
                SetCharacterAnim(&character, CHAR_ANIM_WALK);
                character.animFPS = 36.0f;
            }
        } else {
            SetCharacterAnim(&character, CHAR_ANIM_IDLE);
            character.animFPS = 30.0f;
        }
        character.yaw = camYaw + 180.0f;
        character.position.y = GetTerrainHeightAt(scene, character.position.x, character.position.z);

        Vector3 charFocus = { character.position.x, character.position.y + 1.4f, character.position.z };
        float cp = cosf(camPitch * DEG2RAD);
        float sp = sinf(camPitch * DEG2RAD);
        gameState.camera.target   = charFocus;
        gameState.camera.position = (Vector3){
            charFocus.x + sy * cp * camDist,
            charFocus.y + sp * camDist,
            charFocus.z + cy * cp * camDist
        };

        UpdateCharacter(&character, dt);
        UpdatePropVisibility(&props, scene, gameState.camera);

        light.position = (Vector3){ character.position.x, character.position.y + 6.0f, character.position.z };
        renderer.lightPosition = light.position;

        // Update lighting uniforms (cached locs, no string lookup)
        Vector3 lightPos   = light.position;
        Vector3 viewPos    = gameState.camera.position;
        Vector3 lightColor = ColorToVec3(light.color);

        if (locLightPos >= 0)   SetShaderValue(renderer.lightingShader, locLightPos,   &lightPos,   SHADER_UNIFORM_VEC3);
        if (locLightColor >= 0) SetShaderValue(renderer.lightingShader, locLightColor, &lightColor, SHADER_UNIFORM_VEC3);
        if (locViewPos >= 0)    SetShaderValue(renderer.lightingShader, locViewPos,    &viewPos,    SHADER_UNIFORM_VEC3);

        if (iLocLightPos >= 0)   SetShaderValue(props.instancedShader, iLocLightPos,   &lightPos,   SHADER_UNIFORM_VEC3);
        if (iLocLightColor >= 0) SetShaderValue(props.instancedShader, iLocLightColor, &lightColor, SHADER_UNIFORM_VEC3);
        if (iLocViewPos >= 0)    SetShaderValue(props.instancedShader, iLocViewPos,    &viewPos,    SHADER_UNIFORM_VEC3);

        Vector2 uvScaleScene = {1.0f, 1.0f};
        Vector2 uvScaleRocks = {PROPS_ROCK_UV_REPEAT, PROPS_ROCK_UV_REPEAT};
        float useNormalScene   = scene.floorHasNormalMap ? 1.0f : 0.0f;
        float useParallaxScene = (scene.floorHasHeightMap && parallaxScale > 0.0f) ? 1.0f : 0.0f;
        float useMetalRoughOff = 0.0f;
        float useParallaxOff   = 0.0f;

        if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &uvScaleScene,     SHADER_UNIFORM_VEC2);
        if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &useNormalScene,   SHADER_UNIFORM_FLOAT);
        if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &useMetalRoughOff, SHADER_UNIFORM_FLOAT);
        if (locUseParallax >= 0)   SetShaderValue(renderer.lightingShader, locUseParallax,   &useParallaxScene, SHADER_UNIFORM_FLOAT);
        if (locParallaxScale >= 0) SetShaderValue(renderer.lightingShader, locParallaxScale, &parallaxScale,    SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(gameState.camera);
            DrawSkybox(renderer, gameState.camera);
            DrawSkyCloudDome(renderer, gameState.camera);
            DrawScene(scene);

            // Character (overrides uvScale/normalMap/parallax for its own maps)
            float charUseNormal     = character.hasNormalMap  ? 1.0f : 0.0f;
            float charUseMetalRough = character.hasMetalRough ? 1.0f : 0.0f;
            Vector2 charUvScale = {1.0f, 1.0f};
            if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &charUvScale,       SHADER_UNIFORM_VEC2);
            if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &charUseNormal,     SHADER_UNIFORM_FLOAT);
            if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &charUseMetalRough, SHADER_UNIFORM_FLOAT);
            if (locUseParallax >= 0)   SetShaderValue(renderer.lightingShader, locUseParallax,   &useParallaxOff,    SHADER_UNIFORM_FLOAT);
            DrawCharacter(character);

            // Restore scene uniforms before issuing occlusion queries
            if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &uvScaleScene,      SHADER_UNIFORM_VEC2);
            if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &useNormalScene,    SHADER_UNIFORM_FLOAT);
            if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &useMetalRoughOff,  SHADER_UNIFORM_FLOAT);
            if (locUseParallax >= 0)   SetShaderValue(renderer.lightingShader, locUseParallax,   &useParallaxScene,  SHADER_UNIFORM_FLOAT);

            IssuePropOcclusionQueries(&props);

            // Rock shader uniforms
            float useNormalRocks = props.rockHasNormalMap ? 1.0f : 0.0f;
            if (locUvScale >= 0)      SetShaderValue(renderer.lightingShader, locUvScale,      &uvScaleRocks,   SHADER_UNIFORM_VEC2);
            if (locUseNormalMap >= 0) SetShaderValue(renderer.lightingShader, locUseNormalMap, &useNormalRocks, SHADER_UNIFORM_FLOAT);
            if (iLocUvScale >= 0)       SetShaderValue(props.instancedShader, iLocUvScale,       &uvScaleRocks,     SHADER_UNIFORM_VEC2);
            if (iLocUseNormalMap >= 0)  SetShaderValue(props.instancedShader, iLocUseNormalMap,  &useNormalRocks,   SHADER_UNIFORM_FLOAT);
            if (iLocUseMetalRough >= 0) SetShaderValue(props.instancedShader, iLocUseMetalRough, &useMetalRoughOff, SHADER_UNIFORM_FLOAT);

            DrawProps(&props, gameState.camera);

            // Restore scene uniforms after props
            if (locUvScale >= 0)      SetShaderValue(renderer.lightingShader, locUvScale,      &uvScaleScene,   SHADER_UNIFORM_VEC2);
            if (locUseNormalMap >= 0) SetShaderValue(renderer.lightingShader, locUseNormalMap, &useNormalScene, SHADER_UNIFORM_FLOAT);

            if (gameState.showDebugBoxes) {
                DrawSceneDebug(scene);
                DrawPropsDebug(&props, gameState.camera);
            }
        EndMode3D();

        DrawFPS(10, 10);
        DrawText(TextFormat("Rendered Props: %d/%d (%.1f%%)",
                 props.renderedCount, props.visibleCount,
                 props.visibleCount > 0 ? (float)props.renderedCount / props.visibleCount * 100.0f : 0),
                 10, 40, 20, WHITE);
        if (parallaxScale <= 0.0f)
            DrawText("Parallax: OFF  [1-5]", 10, 65, 20, RAYWHITE);
        else
            DrawText(TextFormat("Parallax: %.2f  [1-5]", parallaxScale), 10, 65, 20, RAYWHITE);

        EndDrawing();
    }

    UnloadCharacter(&character);
    UnloadScene(scene);
    UnloadProps(&props);
    UnloadRenderer(renderer);

    CloseWindow();
    return 0;
}
