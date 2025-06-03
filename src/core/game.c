#include "game.h"
#include "raymath.h"

void initGame(GameState *gameState, int width, int height, const char *title) {
    // Initialize window
    InitWindow(width, height, title);
    SetTargetFPS(60);
    
    // Store screen dimensions
    gameState->screenWidth = width;
    gameState->screenHeight = height;
    
    // Initialize camera
    gameState->camera = (Camera3D){
        .position = (Vector3){ 0.0f, 1.8f, 0.0f },    // Camera position (player height)
        .target = (Vector3){ 1.0f, 1.8f, 0.0f },      // Camera looking at point
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },          // Camera up vector
        .fovy = 60.0f,                                // Camera field-of-view Y
        .projection = CAMERA_PERSPECTIVE              // Camera projection type
    };
    
    // Initialize player
    gameState->playerPosition = gameState->camera.position;
    gameState->playerVelocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    gameState->playerSpeed = 5.0f;
    gameState->mouseSensitivity = 0.003f;
    
    // Initialize render targets for compositing
    gameState->mainTarget = LoadRenderTexture(width, height);
    
    // Props render target at 1/4 resolution
    int propWidth = (int)(width * PROP_RENDER_SCALE);
    int propHeight = (int)(height * PROP_RENDER_SCALE);
    gameState->propTarget = LoadRenderTexture(propWidth, propHeight);
    
    // High-resolution target for comparison (full resolution)
    gameState->highResTarget = LoadRenderTexture(width, height);
    
    // Initialize demo settings
    gameState->showHighRes = false;
    gameState->showDebugInfo = true;
    gameState->demoTimer = 0.0f;
    
    // Initialize time
    gameState->deltaTime = 0.0f;
    gameState->lastFrameTime = GetTime();
    
    // Allocate and initialize world
    gameState->world = (World*)MemAlloc(sizeof(World));
    initWorld(gameState->world);
    
    // Disable cursor for first-person camera
    DisableCursor();
}

void updateDeltaTime(GameState *gameState) {
    double currentTime = GetTime();
    gameState->deltaTime = (float)(currentTime - gameState->lastFrameTime);
    gameState->lastFrameTime = currentTime;
}
