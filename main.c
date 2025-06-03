#include "raylib.h"
#include "raymath.h" // For Vector3, Matrix, etc.
#include <stdio.h>   // For printf (debugging)

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define PROPS_RENDER_SCALE 0.25 // 1/4 resolution

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib First Person Demo");

    // Define the camera to look into our 3D world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 1.8f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 60.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // Render textures for full and quarter resolution
    RenderTexture2D fullResTarget = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RenderTexture2D quarterResTarget = LoadRenderTexture(SCREEN_WIDTH * PROPS_RENDER_SCALE, SCREEN_HEIGHT * PROPS_RENDER_SCALE);

    // Load textures from raw-assets
    Texture2D wallTexture = LoadTexture("raw-assets/tiling_dungeon_brickwall01.png");
    Texture2D floorTexture = LoadTexture("raw-assets/tiling_dungeon_floor01.png");
    Texture2D grassTexture = LoadTexture("raw-assets/grass01_c.png");

    // Check if textures loaded successfully
    if (wallTexture.id == 0) {
        printf("Failed to load wall texture!\n");
        // You might want to set a default color or handle this error more gracefully
    }
    if (floorTexture.id == 0) {
        printf("Failed to load floor texture!\n");
    }
    if (grassTexture.id == 0) {
        printf("Failed to load grass texture!\n");
    }

    // Define level geometry (walls, floor)
    float roomWidth = 16.0f;
    float roomLength = 16.0f;
    float wallHeight = 8.0f;
    float wallThickness = 0.2f;

    // Create models for the environment
    Model floorModel = LoadModelFromMesh(GenMeshCube(roomWidth, wallThickness, roomLength));
    Model wallModelNS = LoadModelFromMesh(GenMeshCube(roomWidth, wallHeight, wallThickness)); // North/South walls
    Model wallModelEW = LoadModelFromMesh(GenMeshCube(wallThickness, wallHeight, roomLength)); // East/West walls

    // Assign textures to models
    if (floorTexture.id > 0) floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = floorTexture;
    else floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY; // Fallback color

    if (wallTexture.id > 0) {
        wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wallTexture;
        wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wallTexture;
    } else {
        wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
        wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
    }

    // Define prop positions (grass)
    Vector3 grassPositions[] = {
        (Vector3){ -2.0f, 0.05f, -2.0f },
        (Vector3){  2.0f, 0.05f, -2.0f },
        (Vector3){ -2.0f, 0.05f,  2.0f },
        (Vector3){  2.0f, 0.05f,  2.0f },
        (Vector3){  0.0f, 0.05f,  0.0f }
    };
    int numGrassProps = sizeof(grassPositions)/sizeof(grassPositions[0]);
    Rectangle grassSourceRec = { 0.0f, 0.0f, (float)grassTexture.width, (float)grassTexture.height };
    Vector2 grassSize = { 1.0f, 1.0f }; // Size of the billboard

    DisableCursor(); // Hide cursor for FPS controls

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) {   // Detect window close button or ESC key
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_FIRST_PERSON); // Use Raylib's first person camera

        // Example to re-enable cursor: Press ESC to exit, or another key to toggle
        // if (IsKeyPressed(KEY_ESCAPE)) EnableCursor();


        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        // 1. Draw full-resolution environment (walls, floor) to fullResTarget
        BeginTextureMode(fullResTarget);
            ClearBackground(RAYWHITE); // Clear the render texture
            BeginMode3D(camera);

                // Draw Floor
                DrawModel(floorModel, (Vector3){ 0.0f, -wallThickness/2.0f, 0.0f }, 1.0f, WHITE);

                // Draw Walls
                // Front wall (+Z)
                DrawModel(wallModelNS, (Vector3){ 0.0f, wallHeight/2.0f, roomLength/2.0f }, 1.0f, WHITE);
                // Back wall (-Z)
                DrawModel(wallModelNS, (Vector3){ 0.0f, wallHeight/2.0f, -roomLength/2.0f }, 1.0f, WHITE);
                // Left wall (-X)
                DrawModel(wallModelEW, (Vector3){ -roomWidth/2.0f, wallHeight/2.0f, 0.0f }, 1.0f, WHITE);
                // Right wall (+X)
                DrawModel(wallModelEW, (Vector3){ roomWidth/2.0f, wallHeight/2.0f, 0.0f }, 1.0f, WHITE);


            EndMode3D();
        EndTextureMode();

        // 2. Draw quarter-resolution props (grass) to quarterResTarget
        BeginTextureMode(quarterResTarget);
            ClearBackground(BLANK); // Clear with transparency
            BeginMode3D(camera);

                // Draw grass props as billboards
                for (int i = 0; i < numGrassProps; i++) {
                    DrawBillboardRec(camera, grassTexture, grassSourceRec, grassPositions[i], grassSize, WHITE);
                }

            EndMode3D();
        EndTextureMode();

        // 3. Composite to screen
        BeginDrawing();
            ClearBackground(BLACK);

            // Draw the full resolution environment
            DrawTextureRec(fullResTarget.texture,
                           (Rectangle){ 0, 0, (float)fullResTarget.texture.width, (float)-fullResTarget.texture.height }, // Source rec (flip Y)
                           (Vector2){ 0, 0 }, // Position
                           WHITE);

            // Draw the quarter resolution props, scaled up
            DrawTexturePro(quarterResTarget.texture,
                           (Rectangle){ 0, 0, (float)quarterResTarget.texture.width, (float)-quarterResTarget.texture.height }, // Source rec (flip Y)
                           (Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, // Destination rec (scaled to full screen)
                           (Vector2){ 0, 0 }, // Origin
                           0.0f, // Rotation
                           WHITE);

            DrawFPS(10, 10);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(fullResTarget);
    UnloadRenderTexture(quarterResTarget);
    // Unload models
    UnloadModel(floorModel);
    UnloadModel(wallModelNS);
    UnloadModel(wallModelEW);

    // Unload textures
    UnloadTexture(wallTexture);
    UnloadTexture(floorTexture);
    UnloadTexture(grassTexture);

    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
