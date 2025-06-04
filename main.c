#include "common.h"
#include "scene.h"  
#include "props.h"
#include "renderer.h"
#include <stdlib.h> // For rand() and srand()
#include <time.h>   // For time()

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib First Person Demo");

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

    // Define level geometry (walls, floor)
    float roomWidth = 16.0f;
    float roomLength = 16.0f;
    float wallHeight = 8.0f;
    float wallThickness = 0.2f;

    // Initialize scene
    Scene scene = InitScene(roomWidth, roomLength, wallHeight, wallThickness, 
                           "raw-assets/tiling_dungeon_brickwall01.png", 
                           "raw-assets/tiling_dungeon_floor01.png");

    // Define number of props to create
    const int numGrassProps = 150;  // 150 grass billboards
    const int numRockProps = 50;    // 50 rock models
    const int totalProps = numGrassProps + numRockProps;
    
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize props with both grass and rock assets
    Props props = InitProps(
        numGrassProps,
        numRockProps,
        "raw-assets/grass01_c.png",    // Grass texture
        "raw-assets/rock.glb",         // Rock model
        "raw-assets/tilingrock01_c.png" // Rock texture
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
        Vector3 position = (Vector3){ x, 0.05f, z };
        AddBillboardProp(&props, position, i);
    }
    
    // Add rock props with random positions
    for (int i = 0; i < numRockProps; i++) {
        // Generate random position within room bounds
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
        
        // Create rock prop on the floor
        Vector3 position = (Vector3){ x, 0.0f, z };
        AddModelProp(&props, position, numGrassProps + i);
    }
    
    // Print prop counts
    printf("Created %d grass props and %d rock props (total: %d)\n", 
           numGrassProps, numRockProps, totalProps);

    DisableCursor(); // Hide cursor for FPS controls

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) {   // Detect window close button or ESC key
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&gameState.camera, CAMERA_FIRST_PERSON); // Use Raylib's first person camera

        // Toggle debug visualization with F1 key
        if (IsKeyPressed(KEY_F1)) gameState.showDebugBoxes = !gameState.showDebugBoxes;
        
        // Update prop visibility based on line of sight
        UpdatePropVisibility(&props, scene, gameState.camera);

        // Example to re-enable cursor: Press ESC to exit, or another key to toggle
        // if (IsKeyPressed(KEY_ESCAPE)) EnableCursor();

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        // 1. Draw full-resolution environment (walls, floor) to fullResTarget
        BeginFullResRender(renderer);
            BeginMode3D(gameState.camera);
                // Draw scene
                DrawScene(scene);
                
                // Draw debug visualization if enabled
                if (gameState.showDebugBoxes) {
                    DrawSceneDebug(scene);
                    DrawPropsDebug(props, gameState.camera);
                }
            EndMode3D();
        EndFullResRender();

        // 2. Draw quarter-resolution props (grass) to quarterResTarget
        BeginQuarterResRender(renderer);
            BeginMode3D(gameState.camera);
                // Draw props
                DrawProps(props, gameState.camera);
            EndMode3D();
        EndQuarterResRender();

        // 3. Composite to screen
        CompositeFinalFrame(renderer);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload all resources
    UnloadRenderer(renderer);
    UnloadScene(scene);
    UnloadProps(props);
    
    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
