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
#define PROPS_RENDER_SCALE .25 // 1/4 resolution
// Texture filter modes:
// TEXTURE_FILTER_POINT - Nearest-neighbor filtering (pixelated)
// TEXTURE_FILTER_BILINEAR - Linear filtering (smooth)
// TEXTURE_FILTER_TRILINEAR - Bilinear filtering with mipmaps (smoother with distance)
// TEXTURE_FILTER_ANISOTROPIC_4X - Anisotropic filtering 4x (higher quality at angles)
// TEXTURE_FILTER_ANISOTROPIC_8X - Anisotropic filtering 8x (higher quality at angles)
// TEXTURE_FILTER_ANISOTROPIC_16X - Anisotropic filtering 16x (highest quality at angles)
#define MAIN_TEXTURE_FILTER_MODE TEXTURE_FILTER_BILINEAR      // Filter for full resolution render target
#define PROPS_TEXTURE_FILTER_MODE TEXTURE_FILTER_BILINEAR  // Filter for quarter resolution props render target

// LOS (Line of Sight) optimization settings
#define LOS_MIN_CAMERA_MOVE 0.5f    // Minimum distance camera must move before rechecking LOS
#define LOS_MAX_PROP_DISTANCE 9.0f // Maximum distance to check props visibility

// Game state
typedef struct {
    Camera3D camera;
    bool showDebugBoxes;
} GameState;

#endif // COMMON_H
