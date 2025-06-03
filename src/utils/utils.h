#ifndef UTILS_H
#define UTILS_H

#include "raylib.h"

// Utility functions
Color MyColorBrightness(Color color, float factor);
Color MyColorLerp(Color a, Color b, float t);
float MyClamp(float value, float min, float max);

#endif // UTILS_H
