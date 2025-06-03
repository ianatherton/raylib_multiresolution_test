#include "raylib.h"
#include "core/game.h"
#include "core/input.h"
#include "renderer/renderer.h"
#include "world/world.h"

int main(void) {
    // Initialize game
    GameState gameState;
    initGame(&gameState, 1280, 720, "Dangerous Forest");
    
    // Game loop
    while (!WindowShouldClose()) {
        // Update
        updateInput(&gameState);
        updateWorld(&gameState);
        
        // Render
        BeginDrawing();
            ClearBackground(RAYWHITE);
            renderWorld(&gameState);
            
            // Draw FPS
            DrawFPS(10, 10);
        EndDrawing();
    }
    
    // Cleanup
    unloadWorld(gameState.world);
    MemFree(gameState.world);
    
    // Unload render targets
    UnloadRenderTexture(gameState.mainTarget);
    UnloadRenderTexture(gameState.propTarget);
    UnloadRenderTexture(gameState.highResTarget);
    
    CloseWindow();
    
    return 0;
}
