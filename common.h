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
#define PROPS_RENDER_SCALE .1 // 1/4 resolution

// LOS (Line of Sight) optimization settings
#define LOS_MIN_CAMERA_MOVE 0.5f    // Minimum distance camera must move before rechecking LOS
#define LOS_MAX_PROP_DISTANCE 9.0f // Maximum distance to check props visibility

// Game state
typedef struct {
    Camera3D camera;
    bool showDebugBoxes;
} GameState;

#endif // COMMON_H
