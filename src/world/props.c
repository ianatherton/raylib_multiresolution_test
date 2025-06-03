#include "props.h"
#include "raylib.h"
#include "raymath.h"
#include "../utils/utils.h"

// Create detailed prop models
Model createDetailedChair() {
    Mesh mesh = GenMeshCube(0.5f, 0.8f, 0.5f);
    Model model = LoadModelFromMesh(mesh);
    
    // Add seat
    Mesh seatMesh = GenMeshCube(0.6f, 0.1f, 0.6f);
    Model seatModel = LoadModelFromMesh(seatMesh);
    
    // Add backrest
    Mesh backMesh = GenMeshCube(0.6f, 0.6f, 0.1f);
    Model backModel = LoadModelFromMesh(backMesh);
    
    return model;
}

Model createDetailedTable() {
    // Create table top
    Mesh topMesh = GenMeshCube(1.2f, 0.1f, 0.8f);
    Model model = LoadModelFromMesh(topMesh);
    
    return model;
}

Model createDetailedLamp() {
    // Create lamp base
    Mesh baseMesh = GenMeshCylinder(0.3f, 0.2f, 16);
    Model model = LoadModelFromMesh(baseMesh);
    
    return model;
}

Model createDetailedBookshelf() {
    // Create bookshelf
    Mesh shelfMesh = GenMeshCube(1.0f, 2.0f, 0.4f);
    Model model = LoadModelFromMesh(shelfMesh);
    
    return model;
}

Model createDetailedBed() {
    // Create bed base
    Mesh baseMesh = GenMeshCube(1.8f, 0.3f, 0.9f);
    Model model = LoadModelFromMesh(baseMesh);
    
    return model;
}

Model createDetailedChest() {
    // Create chest
    Mesh chestMesh = GenMeshCube(0.8f, 0.6f, 0.5f);
    Model model = LoadModelFromMesh(chestMesh);
    
    return model;
}

// Create pixelated textures for props
Texture2D createPixelatedTexture(int width, int height, Color baseColor, int pixelSize) {
    Image img = GenImageColor(width, height, baseColor);
    
    // Create a pixelated pattern
    for (int y = 0; y < height; y += pixelSize) {
        for (int x = 0; x < width; x += pixelSize) {
            Color pixelColor = MyColorBrightness(baseColor, (float)GetRandomValue(80, 120) / 100.0f);
            
            // Fill a square of pixelSize with the same color
            for (int py = 0; py < pixelSize && y + py < height; py++) {
                for (int px = 0; px < pixelSize && x + px < width; px++) {
                    ImageDrawPixel(&img, x + px, y + py, pixelColor);
                }
            }
        }
    }
    
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

// Create a checkerboard texture
Texture2D createCheckerboardTexture(int width, int height, Color color1, Color color2, int checkSize) {
    Image img = GenImageColor(width, height, color1);
    
    for (int y = 0; y < height; y += checkSize) {
        for (int x = 0; x < width; x += checkSize) {
            Color fillColor = ((x / checkSize + y / checkSize) % 2 == 0) ? color1 : color2;
            
            // Fill a square of checkSize with the color
            for (int py = 0; py < checkSize && y + py < height; py++) {
                for (int px = 0; px < checkSize && x + px < width; px++) {
                    ImageDrawPixel(&img, x + px, y + py, fillColor);
                }
            }
        }
    }
    
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

// Create a gradient texture
Texture2D createGradientTexture(int width, int height, Color startColor, Color endColor, bool horizontal) {
    Image img = GenImageColor(width, height, startColor);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float factor = horizontal ? (float)x / width : (float)y / height;
            Color pixelColor = MyColorLerp(startColor, endColor, factor);
            ImageDrawPixel(&img, x, y, pixelColor);
        }
    }
    
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

// Create a noise texture
Texture2D createNoiseTexture(int width, int height, Color baseColor, float noiseScale) {
    Image img = GenImageColor(width, height, baseColor);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float brightness = (float)GetRandomValue(0, 100) / 100.0f * noiseScale;
            Color pixelColor = MyColorBrightness(baseColor, 1.0f - brightness);
            ImageDrawPixel(&img, x, y, pixelColor);
        }
    }
    
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}
