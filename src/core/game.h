#ifndef GAME_H
#define GAME_H

#include "raylib.h"

// Scale for prop rendering (1/4 resolution)
#define PROP_RENDER_SCALE 0.25f

// Forward declaration
struct World;

// Game state
typedef struct GameState {
    // Window settings
    int screenWidth;
    int screenHeight;
    
    // Camera and player
    Camera3D camera;
    Vector3 playerPosition;
    Vector3 playerVelocity;
    float playerSpeed;
    float mouseSensitivity;
    
    // World
    struct World* world;
    
    // Render targets for compositing
    RenderTexture2D mainTarget;   // Full resolution target for main scene
    RenderTexture2D propTarget;   // Lower resolution target for props
    RenderTexture2D highResTarget; // Full resolution target for high-res props
    
    // Demo settings
    bool showHighRes;       // Toggle between high and low resolution props
    bool showDebugInfo;     // Show debug information
    float demoTimer;        // Timer for automatic toggling
    
    // Time
    float deltaTime;
    double lastFrameTime;
} GameState;

// Include world.h after GameState is defined
#include "../world/world.h"

// Initialize game
void initGame(GameState *gameState, int width, int height, const char *title);

// Get delta time
void updateDeltaTime(GameState *gameState);

#endif // GAME_H
