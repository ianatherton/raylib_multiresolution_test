#ifndef COMMON_H
#define COMMON_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdbool.h>

// Screen dimensions
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Rendering settings
#define PROPS_RENDER_SCALE 1.0 // 1/4 resolution

// Game state
typedef struct {
    Camera3D camera;
    bool showDebugBoxes;
} GameState;

#endif // COMMON_H
