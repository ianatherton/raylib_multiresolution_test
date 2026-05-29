#include "common.h"
#include "scene.h"
#include "props.h"
#include "renderer.h"
#include "lighting.h"
#include "character.h"
#include <stdlib.h> // For rand() and srand()
#include <time.h>   // For time()

int main(void) {
    // Create a single point light above the scene
    Light light = {
        .position = (Vector3){0.0f, 6.0f, 0.0f},
        .color = WHITE,
        .intensity = 1.0f
    };

    // Initialization

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib First Person Demo");
    SetTextureFilter(GetFontDefault().texture, MAIN_TEXTURE_FILTER_MODE);

    // Initialize game state
    GameState gameState = {0};
    
    // Define the camera to look into our 3D world
    gameState.camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };    // Camera position
    gameState.camera.target = (Vector3){ 0.0f, 1.8f, 0.0f };      // Camera looking at point
    gameState.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    gameState.camera.fovy = 60.0f;                                // Camera field-of-view Y
    gameState.camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type
    gameState.showDebugBoxes = false;                             // Debug visualization flag

    // Initialize renderer
    Renderer renderer = InitRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, PROPS_RENDER_SCALE);
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

    // Define level geometry (walls, floor)
    float roomWidth = 500.0f;
    float roomLength = 500.0f;
    float wallHeight = 8.0f;
    float wallThickness = 0.2f;

    // Initialize scene
    unsigned int terrainSeed = (unsigned int)time(NULL);
    Scene scene = InitScene(roomWidth, roomLength, wallHeight, wallThickness, 
                           "raw-assets/tiling_dungeon_brickwall01.png", 
                           "raw-assets/tiling_dungeon_floor01.png",
                           renderer.lightingShader,
                           terrainSeed);

    // Define number of props to create
    const int numGrassProps = 200000;  // 10800 grass billboards (72x original 150)
    const int numRockProps = 40000;    // 3600 rock models (72x original 50)
    const int totalProps = numGrassProps + numRockProps;
    
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize props with both grass and rock assets
    Props props = InitProps(
        numGrassProps,
        numRockProps,
        "raw-assets/grass01_c.png",
        "raw-assets/rock.glb",
        "raw-assets/tilingrock02_c.png",
        "raw-assets/tilingrock02_n.png",
        renderer.lightingShader
    );
    
    // Calculate usable room area (slightly inside the walls)
    float marginFromWall = 1.0f;
    float minX = -roomWidth/2 + marginFromWall;
    float maxX = roomWidth/2 - marginFromWall;
    float minZ = -roomLength/2 + marginFromWall;
    float maxZ = roomLength/2 - marginFromWall;
    
    // Add grass props with random positions
    for (int i = 0; i < numGrassProps; i++) {
        // Generate random position within room bounds
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
        
        // Create grass prop slightly above floor
        float terrainY = GetTerrainHeightAt(scene, x, z);
        Vector3 position = (Vector3){ x, terrainY + 0.05f, z };
        AddBillboardProp(&props, position, i);
    }
    
    // Add rock props with random positions
    for (int i = 0; i < numRockProps; i++) {
        // Generate random position within room bounds
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
        
        float terrainY = GetTerrainHeightAt(scene, x, z);
        Vector3 position = (Vector3){ x, terrainY + PROPS_ROCK_Y_OFFSET, z };
        AddModelProp(&props, position, numGrassProps + i);
    }
    
    // Print prop counts
    printf("Created %d grass props and %d rock props (total: %d)\n",
           numGrassProps, numRockProps, totalProps);

    // Upload all prop proxy positions to GPU once
    BuildProxyVBO(&props);

    // Initialize character
    Character character = InitCharacter(renderer.lightingShader);
    float charX = 0.0f, charZ = -5.0f;
    character.position = (Vector3){ charX, GetTerrainHeightAt(scene, charX, charZ), charZ };
    character.yaw = 180.0f; // face toward camera start

    DisableCursor(); // Hide cursor for FPS controls

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) {   // Detect window close button or ESC key
        // Update
        //----------------------------------------------------------------------------------
        ReadPropOcclusionResults(&props);  // non-blocking; reads last frame's GPU query results
        UpdateCamera(&gameState.camera, CAMERA_FIRST_PERSON); // Use Raylib's first person camera
        float eyeHeight = 1.8f;
        float previousY = gameState.camera.position.y;
        float terrainY = GetTerrainHeightAt(scene, gameState.camera.position.x, gameState.camera.position.z);
        gameState.camera.position.y = terrainY + eyeHeight;
        gameState.camera.target.y += (gameState.camera.position.y - previousY);

        // Toggle debug visualization with F1 key
        if (IsKeyPressed(KEY_F1)) gameState.showDebugBoxes = !gameState.showDebugBoxes;

        // Cycle character animation with Tab
        if (IsKeyPressed(KEY_TAB)) {
            int next = ((int)character.currentAnim + 1) % CHAR_ANIM_COUNT;
            SetCharacterAnim(&character, (CharAnimIndex)next);
        }

        // Update character
        UpdateCharacter(&character, GetFrameTime());
        
        // Update prop visibility based on line of sight
        UpdatePropVisibility(&props, scene, gameState.camera);

        // Update light position in renderer
        renderer.lightPosition = light.position;
        
        // Update lighting uniforms
        int locLightPos = GetShaderLocation(renderer.lightingShader, "lightPos");
        int locLightColor = GetShaderLocation(renderer.lightingShader, "lightColor");
        int locViewPos = GetShaderLocation(renderer.lightingShader, "viewPos");
        
        if (locLightPos >= 0 && locLightColor >= 0 && locViewPos >= 0) {
            Vector3 lightPos = light.position;
            Vector3 viewPos = gameState.camera.position;
            Vector3 lightColor = ColorToVec3(light.color);
            SetShaderValue(renderer.lightingShader, locLightPos, &lightPos, SHADER_UNIFORM_VEC3);
            SetShaderValue(renderer.lightingShader, locLightColor, &lightColor, SHADER_UNIFORM_VEC3);
            SetShaderValue(renderer.lightingShader, locViewPos, &viewPos, SHADER_UNIFORM_VEC3);

            int iLocLightPos   = GetShaderLocation(props.instancedShader, "lightPos");
            int iLocLightColor = GetShaderLocation(props.instancedShader, "lightColor");
            int iLocViewPos    = GetShaderLocation(props.instancedShader, "viewPos");
            if (iLocLightPos >= 0)   SetShaderValue(props.instancedShader, iLocLightPos,   &lightPos,   SHADER_UNIFORM_VEC3);
            if (iLocLightColor >= 0) SetShaderValue(props.instancedShader, iLocLightColor, &lightColor, SHADER_UNIFORM_VEC3);
            if (iLocViewPos >= 0)    SetShaderValue(props.instancedShader, iLocViewPos,    &viewPos,    SHADER_UNIFORM_VEC3);
        }

        int locUvScale      = GetShaderLocation(renderer.lightingShader, "uvScale");
        int locUseNormalMap = GetShaderLocation(renderer.lightingShader, "useNormalMap");
        int locUseMetalRough = GetShaderLocation(renderer.lightingShader, "useMetalRough");
        Vector2 uvScaleScene = {1.0f, 1.0f};
        Vector2 uvScaleRocks = {PROPS_ROCK_UV_REPEAT, PROPS_ROCK_UV_REPEAT};
        float useNormalScene = scene.floorHasNormalMap ? 1.0f : 0.0f;
        float useMetalRoughOff = 0.0f;
        if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &uvScaleScene,      SHADER_UNIFORM_VEC2);
        if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &useNormalScene,    SHADER_UNIFORM_FLOAT);
        if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &useMetalRoughOff,  SHADER_UNIFORM_FLOAT);

        // Example to re-enable cursor: Press ESC to exit, or another key to toggle
        // if (IsKeyPressed(KEY_ESCAPE)) EnableCursor();

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        // 1. Draw full-resolution environment (walls, floor) to fullResTarget
        BeginFullResRender(renderer);
            BeginMode3D(gameState.camera);
                DrawSkybox(renderer, gameState.camera);
                DrawSkyCloudDome(renderer, gameState.camera);
                // Draw scene
                DrawScene(scene);

                // Draw character with its own normal/metallic/roughness maps.
                float charUseNormal    = character.hasNormalMap  ? 1.0f : 0.0f;
                float charUseMetalRough = character.hasMetalRough ? 1.0f : 0.0f;
                Vector2 charUvScale = {1.0f, 1.0f};
                if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &charUvScale,       SHADER_UNIFORM_VEC2);
                if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &charUseNormal,     SHADER_UNIFORM_FLOAT);
                if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &charUseMetalRough, SHADER_UNIFORM_FLOAT);
                DrawCharacter(character);
                // Restore scene uniforms.
                if (locUvScale >= 0)       SetShaderValue(renderer.lightingShader, locUvScale,       &uvScaleScene,      SHADER_UNIFORM_VEC2);
                if (locUseNormalMap >= 0)  SetShaderValue(renderer.lightingShader, locUseNormalMap,  &useNormalScene,    SHADER_UNIFORM_FLOAT);
                if (locUseMetalRough >= 0) SetShaderValue(renderer.lightingShader, locUseMetalRough, &useMetalRoughOff,  SHADER_UNIFORM_FLOAT);

                // Invisible depth-writing box so prop queries treat the character as a solid occluder.
                // BLANK alpha=0 blends transparently; depth is still written independently.
                DrawCubeV(
                    (Vector3){ character.position.x,
                               character.position.y + CHAR_OCCLUDER_HEIGHT * 0.5f,
                               character.position.z },
                    (Vector3){ CHAR_OCCLUDER_WIDTH, CHAR_OCCLUDER_HEIGHT, CHAR_OCCLUDER_WIDTH },
                    BLANK
                );

                // Issue occlusion queries for in-range props against the now-rendered terrain depth
                IssuePropOcclusionQueries(&props);

                // Draw debug visualization if enabled
                if (gameState.showDebugBoxes) {
                    DrawSceneDebug(scene);
                    DrawPropsDebug(&props, gameState.camera);
                }
            EndMode3D();
        EndFullResRender();

        if (locUvScale >= 0) {
            SetShaderValue(renderer.lightingShader, locUvScale, &uvScaleRocks, SHADER_UNIFORM_VEC2);
        }
        float useNormalRocks = props.rockHasNormalMap ? 1.0f : 0.0f;
        if (locUseNormalMap >= 0) {
            SetShaderValue(renderer.lightingShader, locUseNormalMap, &useNormalRocks, SHADER_UNIFORM_FLOAT);
        }
        {
            int iLocUvScale      = GetShaderLocation(props.instancedShader, "uvScale");
            int iLocUseNormalMap = GetShaderLocation(props.instancedShader, "useNormalMap");
            int iLocUseMetalRough = GetShaderLocation(props.instancedShader, "useMetalRough");
            if (iLocUvScale >= 0)       SetShaderValue(props.instancedShader, iLocUvScale,       &uvScaleRocks,      SHADER_UNIFORM_VEC2);
            if (iLocUseNormalMap >= 0)  SetShaderValue(props.instancedShader, iLocUseNormalMap,  &useNormalRocks,    SHADER_UNIFORM_FLOAT);
            if (iLocUseMetalRough >= 0) SetShaderValue(props.instancedShader, iLocUseMetalRough, &useMetalRoughOff,  SHADER_UNIFORM_FLOAT);
        }

        // 2. Draw quarter-resolution props (grass) to quarterResTarget
        BeginQuarterResRender(renderer);
            BeginMode3D(gameState.camera);
                // Draw props
                DrawProps(&props, gameState.camera);
            EndMode3D();
        EndQuarterResRender();

        // 3. Composite to screen and draw UI
        CompositeFinalFrame(renderer, gameState.camera, props.renderedCount, props.visibleCount);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload resources
    UnloadCharacter(&character);
    UnloadScene(scene);
    UnloadProps(&props);
    UnloadRenderer(renderer);  // This now handles unloading the shader

    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
