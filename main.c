#include "common.h"
#include "scene.h"  
#include "props.h"
#include "renderer.h"

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

    // Define prop positions (grass)
    Vector3 grassPositions[] = {
        (Vector3){ -2.0f, 0.05f, -2.0f },
        (Vector3){  2.0f, 0.05f, -2.0f },
        (Vector3){ -2.0f, 0.05f,  2.0f },
        (Vector3){  2.0f, 0.05f,  2.0f },
        (Vector3){  0.0f, 0.05f,  0.0f }
    };
    int numGrassProps = sizeof(grassPositions)/sizeof(grassPositions[0]);
    
    // Initialize props
    Props props = InitProps(grassPositions, numGrassProps, "raw-assets/grass01_c.png");

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
