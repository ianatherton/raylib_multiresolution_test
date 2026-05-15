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
#define PROPS_RENDER_SCALE 0.3 // prop resolution scale

// DOF in world meters from camera: no blur at or below DOF_SHARP_RADIUS_M; full blur by DOF_BLUR_FULL_DIST_M
#define DOF_SHARP_RADIUS_M 4.0f
#define DOF_BLUR_FULL_DIST_M 55.0f
#define DOF_GAUSSIAN_PIXEL_SCALE 0.01f // wider separable blur before distance mix
// Texture filter modes:
// TEXTURE_FILTER_POINT - Nearest-neighbor filtering (pixelated)
// TEXTURE_FILTER_BILINEAR - Linear filtering (smooth)
// TEXTURE_FILTER_TRILINEAR - Bilinear filtering with mipmaps (smoother with distance)
// TEXTURE_FILTER_ANISOTROPIC_4X - Anisotropic filtering 4x (higher quality at angles)
// TEXTURE_FILTER_ANISOTROPIC_8X - Anisotropic filtering 8x (higher quality at angles)
// TEXTURE_FILTER_ANISOTROPIC_16X - Anisotropic filtering 16x (highest quality at angles)
#define MAIN_TEXTURE_FILTER_MODE TEXTURE_FILTER_BILINEAR      // Filter for full resolution render target
#define PROPS_TEXTURE_FILTER_MODE TEXTURE_FILTER_BILINEAR  // Filter for quarter resolution props render target

static inline void ApplyTextureFilterToAllMaterialMaps(Model model, int filter) { // all material maps incl. GLB embeds
    for (int i = 0; i < model.materialCount; i++) {
        Material *mat = &model.materials[i];
        if (mat->maps == NULL) continue;
        for (int m = 0; m <= MATERIAL_MAP_BRDF; m++) {
            Texture2D t = mat->maps[m].texture;
            if (t.id > 0) SetTextureFilter(t, filter);
        }
    }
}

// Terrain texture tiling density across full terrain dimensions (higher = more repeats)
#define TERRAIN_UV_REPEAT 180.0f

// How many times the rock diffuse repeats per mesh UV unit (needs TEXTURE_WRAP_REPEAT on rock texture)
#define PROPS_ROCK_UV_REPEAT 6.0f

// Rock spawn height (floor top ~y=0); negative = sink into floor — ~half buried at DrawProps scale 0.5 for a ~1 unit rock
#define PROPS_ROCK_Y_OFFSET (-0.25f)

// LOS (Line of Sight) optimization settings
#define LOS_MIN_CAMERA_MOVE 0.5f       // Minimum distance camera must move before rechecking visibility
#define LOS_MAX_GRASS_DISTANCE 45.0f   // Max grass visibility distance for cheap CPU culling
#define LOS_MAX_ROCK_DISTANCE 80.0f    // Max rock visibility distance for cheap CPU culling
#define LOS_TERRAIN_SAMPLES 8          // Cheap terrain occlusion samples per prop ray

// Game state
typedef struct {
    Camera3D camera;
    bool showDebugBoxes;
} GameState;

#endif // COMMON_H
