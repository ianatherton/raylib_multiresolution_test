#ifndef SCENE_H
#define SCENE_H

#include "common.h"

// Scene geometry
typedef struct {
    float roomWidth;
    float roomLength;
    float wallHeight;
    float wallThickness;
    
    Model floorModel;
    Model wallModelNS;  // North/South walls
    Model wallModelEW;  // East/West walls
    
    Texture2D wallTexture;
    Texture2D floorTexture;
    
    // Collision boxes for walls
    BoundingBox* wallBoxes;
    int numWalls;
} Scene;

// Initialize scene with dimensions and textures
Scene InitScene(float width, float length, float height, float thickness, 
                const char* wallTexturePath, const char* floorTexturePath);

// Draw scene (walls, floor)
void DrawScene(Scene scene);

// Draw debug visualization for scene (bounding boxes)
void DrawSceneDebug(Scene scene);

// Unload scene resources
void UnloadScene(Scene scene);

#endif // SCENE_H
