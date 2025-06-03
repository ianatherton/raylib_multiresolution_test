#ifndef PROPS_H
#define PROPS_H

#include "raylib.h"

// Create detailed prop models
Model createDetailedChair();
Model createDetailedTable();
Model createDetailedLamp();
Model createDetailedBookshelf();
Model createDetailedBed();
Model createDetailedChest();

// Create pixelated textures for props
Texture2D createPixelatedTexture(int width, int height, Color baseColor, int pixelSize);

// Create a checkerboard texture
Texture2D createCheckerboardTexture(int width, int height, Color color1, Color color2, int checkSize);

// Create a gradient texture
Texture2D createGradientTexture(int width, int height, Color startColor, Color endColor, bool horizontal);

// Create a noise texture
Texture2D createNoiseTexture(int width, int height, Color baseColor, float noiseScale);

#endif // PROPS_H
