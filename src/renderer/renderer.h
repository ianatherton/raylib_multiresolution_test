#ifndef RENDERER_H
#define RENDERER_H

#include "../core/game.h"

// Render the world
void renderWorld(GameState *gameState);

// Draw a room
void drawRoom(Room *room, Camera3D camera);

// Draw props at low resolution
void drawPropsLowRes(GameState *gameState);

// Draw props at high resolution
void drawPropsHighRes(GameState *gameState);

// Composite the final image
void compositeScene(GameState *gameState);

// Draw debug information
void drawDebugInfo(GameState *gameState);

#endif // RENDERER_H
