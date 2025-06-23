#ifndef LIGHTING_H
#define LIGHTING_H

#include "raylib.h"

// Simple point light structure
typedef struct {
    Vector3 position;
    Color color;
    float intensity;
} Light;

// Helper to convert Color to normalized vec3
static inline Vector3 ColorToVec3(Color c) {
    return (Vector3){c.r/255.0f, c.g/255.0f, c.b/255.0f};
}

#endif // LIGHTING_H
