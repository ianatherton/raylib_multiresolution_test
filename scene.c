#include "scene.h"
#include <stdlib.h>

Scene InitScene(float width, float length, float height, float thickness, 
                const char* wallTexturePath, const char* floorTexturePath) {
    Scene scene = {0};
    
    // Store dimensions
    scene.roomWidth = width;
    scene.roomLength = length;
    scene.wallHeight = height;
    scene.wallThickness = thickness;
    
    // Load textures
    scene.wallTexture = LoadTexture(wallTexturePath);
    scene.floorTexture = LoadTexture(floorTexturePath);
    
    // Check if textures loaded successfully
    if (scene.wallTexture.id == 0) {
        printf("Failed to load wall texture: %s\n", wallTexturePath);
    }
    if (scene.floorTexture.id == 0) {
        printf("Failed to load floor texture: %s\n", floorTexturePath);
    }
    
    // Apply texture filtering to scene textures
    if (scene.wallTexture.id > 0) SetTextureFilter(scene.wallTexture, MAIN_TEXTURE_FILTER_MODE);
    if (scene.floorTexture.id > 0) SetTextureFilter(scene.floorTexture, MAIN_TEXTURE_FILTER_MODE);
    
    // Create models for the environment
    scene.floorModel = LoadModelFromMesh(GenMeshCube(width, thickness, length));
    scene.wallModelNS = LoadModelFromMesh(GenMeshCube(width, height, thickness)); // North/South walls
    scene.wallModelEW = LoadModelFromMesh(GenMeshCube(thickness, height, length)); // East/West walls
    
    // Assign textures to models
    if (scene.floorTexture.id > 0) scene.floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.floorTexture;
    else scene.floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY; // Fallback color
    
    if (scene.wallTexture.id > 0) {
        scene.wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.wallTexture;
        scene.wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.wallTexture;
    } else {
        scene.wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
        scene.wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
    }
    
    // Define wall collision boxes
    scene.numWalls = 4;
    scene.wallBoxes = (BoundingBox*)malloc(scene.numWalls * sizeof(BoundingBox));
    
    // Front wall (+Z)
    scene.wallBoxes[0] = (BoundingBox){
        (Vector3){ -width/2.0f, 0.0f, length/2.0f - thickness/2.0f },
        (Vector3){ width/2.0f, height, length/2.0f + thickness/2.0f }
    };
    
    // Back wall (-Z)
    scene.wallBoxes[1] = (BoundingBox){
        (Vector3){ -width/2.0f, 0.0f, -length/2.0f - thickness/2.0f },
        (Vector3){ width/2.0f, height, -length/2.0f + thickness/2.0f }
    };
    
    // Left wall (-X)
    scene.wallBoxes[2] = (BoundingBox){
        (Vector3){ -width/2.0f - thickness/2.0f, 0.0f, -length/2.0f },
        (Vector3){ -width/2.0f + thickness/2.0f, height, length/2.0f }
    };
    
    // Right wall (+X)
    scene.wallBoxes[3] = (BoundingBox){
        (Vector3){ width/2.0f - thickness/2.0f, 0.0f, -length/2.0f },
        (Vector3){ width/2.0f + thickness/2.0f, height, length/2.0f }
    };
    
    return scene;
}

void DrawScene(Scene scene) {
    // Draw Floor
    DrawModel(scene.floorModel, (Vector3){ 0.0f, -scene.wallThickness/2.0f, 0.0f }, 1.0f, WHITE);
    
    // Draw Walls
    // Front wall (+Z)
    DrawModel(scene.wallModelNS, (Vector3){ 0.0f, scene.wallHeight/2.0f, scene.roomLength/2.0f }, 1.0f, WHITE);
    // Back wall (-Z)
    DrawModel(scene.wallModelNS, (Vector3){ 0.0f, scene.wallHeight/2.0f, -scene.roomLength/2.0f }, 1.0f, WHITE);
    // Left wall (-X)
    DrawModel(scene.wallModelEW, (Vector3){ -scene.roomWidth/2.0f, scene.wallHeight/2.0f, 0.0f }, 1.0f, WHITE);
    // Right wall (+X)
    DrawModel(scene.wallModelEW, (Vector3){ scene.roomWidth/2.0f, scene.wallHeight/2.0f, 0.0f }, 1.0f, WHITE);
}

void DrawSceneDebug(Scene scene) {
    // Draw collision boxes for walls
    for (int i = 0; i < scene.numWalls; i++) {
        DrawBoundingBox(scene.wallBoxes[i], RED);
    }
}

void UnloadScene(Scene scene) {
    // Unload models
    UnloadModel(scene.floorModel);
    UnloadModel(scene.wallModelNS);
    UnloadModel(scene.wallModelEW);
    
    // Unload textures
    UnloadTexture(scene.wallTexture);
    UnloadTexture(scene.floorTexture);
    
    // Free allocated memory
    free(scene.wallBoxes);
}
