#include "scene.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

static float Hash2D(int x, int y, unsigned int seed) {
    unsigned int h = (unsigned int)(x * 374761393u + y * 668265263u) ^ (seed * 1442695041u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (float)h / (float)UINT_MAX;
}

static float Smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static float ValueNoise2D(float x, float z, unsigned int seed) {
    int x0 = (int)floorf(x);
    int z0 = (int)floorf(z);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float tx = x - (float)x0;
    float tz = z - (float)z0;
    float sx = Smoothstep(tx);
    float sz = Smoothstep(tz);

    float n00 = Hash2D(x0, z0, seed);
    float n10 = Hash2D(x1, z0, seed);
    float n01 = Hash2D(x0, z1, seed);
    float n11 = Hash2D(x1, z1, seed);

    float nx0 = Lerp(n00, n10, sx);
    float nx1 = Lerp(n01, n11, sx);
    return Lerp(nx0, nx1, sz) * 2.0f - 1.0f;
}

static float FBM2D(float x, float z, unsigned int seed, int octaves, float lacunarity, float gain) {
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += ValueNoise2D(x * frequency, z * frequency, seed + (unsigned int)(i * 1987)) * amplitude;
        norm += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

static float RidgeNoise2D(float x, float z, unsigned int seed, int octaves, float lacunarity, float gain) {
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        float n = ValueNoise2D(x * frequency, z * frequency, seed + (unsigned int)(i * 3571));
        float ridge = 1.0f - fabsf(n);
        ridge *= ridge;
        sum += ridge * amplitude;
        norm += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

static bool BuildNormalMapPath(const char* diffusePath, char* outPath, size_t outPathSize) {
    const char* extension = strrchr(diffusePath, '.');
    if (extension != NULL && extension != diffusePath) {
        size_t baseLength = (size_t)(extension - diffusePath);
        return snprintf(outPath, outPathSize, "%.*s_n%s", (int)baseLength, diffusePath, extension) > 0;
    }
    return snprintf(outPath, outPathSize, "%s_n", diffusePath) > 0;
}

Scene InitScene(float width, float length, float height, float thickness, 
                const char* wallTexturePath, const char* floorTexturePath, Shader lightingShader, unsigned int terrainSeed) {
    Scene scene = {0};
    
    // Store dimensions
    scene.roomWidth = width;
    scene.roomLength = length;
    scene.wallHeight = height;
    scene.wallThickness = thickness;
    
    // Load textures
    scene.wallTexture = LoadTexture(wallTexturePath);
    scene.floorTexture = LoadTexture(floorTexturePath);
    scene.floorNormalMap = (Texture2D){0};
    scene.floorHasNormalMap = false;

    char floorNormalPath[512] = {0};
    if (BuildNormalMapPath(floorTexturePath, floorNormalPath, sizeof(floorNormalPath))) {
        scene.floorNormalMap = LoadTexture(floorNormalPath);
        if (scene.floorNormalMap.id > 0) {
            scene.floorHasNormalMap = true;
            printf("Floor normal map: %s (ID: %u)\n", floorNormalPath, scene.floorNormalMap.id);
        } else {
            printf("Floor normal map not found: %s\n", floorNormalPath);
        }
    }
    
    // Check if textures loaded successfully
    if (scene.wallTexture.id == 0) {
        printf("Failed to load wall texture: %s\n", wallTexturePath);
    }
    if (scene.floorTexture.id == 0) {
        printf("Failed to load floor texture: %s\n", floorTexturePath);
    }
    
    // Apply texture filtering to scene textures
    if (scene.wallTexture.id > 0) SetTextureFilter(scene.wallTexture, MAIN_TEXTURE_FILTER_MODE);
    if (scene.floorTexture.id > 0) {
        SetTextureFilter(scene.floorTexture, MAIN_TEXTURE_FILTER_MODE);
        SetTextureWrap(scene.floorTexture, TEXTURE_WRAP_REPEAT);
    }
    if (scene.floorNormalMap.id > 0) {
        SetTextureFilter(scene.floorNormalMap, MAIN_TEXTURE_FILTER_MODE);
        SetTextureWrap(scene.floorNormalMap, TEXTURE_WRAP_REPEAT);
    }
    
    scene.terrainWidth = 129;
    scene.terrainLength = 129;
    scene.terrainHeightScale = 4.8f;
    scene.terrainCellSizeX = width / (float)(scene.terrainWidth - 1);
    scene.terrainCellSizeZ = length / (float)(scene.terrainLength - 1);
    scene.terrainHeights = (float*)malloc((size_t)(scene.terrainWidth * scene.terrainLength) * sizeof(float));

    const int terrainVertexCount = scene.terrainWidth * scene.terrainLength;
    const int terrainQuadCount = (scene.terrainWidth - 1) * (scene.terrainLength - 1);
    Mesh terrainMesh = {0};
    terrainMesh.vertexCount = terrainVertexCount;
    terrainMesh.triangleCount = terrainQuadCount * 2;
    terrainMesh.vertices = (float*)MemAlloc((size_t)terrainVertexCount * 3 * sizeof(float));
    terrainMesh.texcoords = (float*)MemAlloc((size_t)terrainVertexCount * 2 * sizeof(float));
    terrainMesh.normals = (float*)MemAlloc((size_t)terrainVertexCount * 3 * sizeof(float));
    terrainMesh.indices = (unsigned short*)MemAlloc((size_t)terrainQuadCount * 6 * sizeof(unsigned short));

    float startX = -width * 0.5f;
    float startZ = -length * 0.5f;
    for (int z = 0; z < scene.terrainLength; z++) {
        for (int x = 0; x < scene.terrainWidth; x++) {
            int index = z * scene.terrainWidth + x;
            float worldX = startX + (float)x * scene.terrainCellSizeX;
            float worldZ = startZ + (float)z * scene.terrainCellSizeZ;
            float baseX = worldX * 0.012f;
            float baseZ = worldZ * 0.012f;
            float warpX = FBM2D(baseX + 37.1f, baseZ - 12.4f, terrainSeed + 911u, 3, 2.1f, 0.5f);
            float warpZ = FBM2D(baseX - 18.6f, baseZ + 25.7f, terrainSeed + 1823u, 3, 2.1f, 0.5f);
            float warpedX = baseX + warpX * 0.9f;
            float warpedZ = baseZ + warpZ * 0.9f;
            float macro = FBM2D(warpedX * 0.65f, warpedZ * 0.65f, terrainSeed, 5, 2.0f, 0.5f);
            float detail = FBM2D(warpedX * 2.3f, warpedZ * 2.3f, terrainSeed + 457u, 4, 2.2f, 0.45f);
            float ridges = RidgeNoise2D(warpedX * 1.45f, warpedZ * 1.45f, terrainSeed + 1291u, 4, 2.0f, 0.5f);
            float peakMaskRaw = FBM2D(warpedX * 0.22f, warpedZ * 0.22f, terrainSeed + 2903u, 3, 2.0f, 0.55f);
            float peakMask = Clamp((peakMaskRaw - 0.45f) / 0.55f, 0.0f, 1.0f);
            peakMask = peakMask * peakMask;
            float tallPeaks = RidgeNoise2D(warpedX * 0.85f, warpedZ * 0.85f, terrainSeed + 3761u, 4, 2.1f, 0.5f) * peakMask;
            float heightShape = macro * 0.75f + detail * 0.30f + (ridges * 2.0f - 1.0f) * 0.65f + tallPeaks * 1.6f;
            float heightValue = heightShape * (scene.terrainHeightScale * 1.55f);
            float extremePeakMask = Clamp((peakMask - 0.90f) / 0.10f, 0.0f, 1.0f);
            heightValue *= (1.0f + 0.70f * extremePeakMask);
            scene.terrainHeights[index] = heightValue;

            terrainMesh.vertices[index * 3 + 0] = worldX;
            terrainMesh.vertices[index * 3 + 1] = heightValue;
            terrainMesh.vertices[index * 3 + 2] = worldZ;
            terrainMesh.texcoords[index * 2 + 0] = ((float)x / (float)(scene.terrainWidth - 1)) * TERRAIN_UV_REPEAT;
            terrainMesh.texcoords[index * 2 + 1] = ((float)z / (float)(scene.terrainLength - 1)) * TERRAIN_UV_REPEAT;
        }
    }

    for (int z = 0; z < scene.terrainLength; z++) {
        for (int x = 0; x < scene.terrainWidth; x++) {
            int ixL = (x > 0) ? x - 1 : x;
            int ixR = (x < scene.terrainWidth - 1) ? x + 1 : x;
            int izD = (z > 0) ? z - 1 : z;
            int izU = (z < scene.terrainLength - 1) ? z + 1 : z;
            float hL = scene.terrainHeights[z * scene.terrainWidth + ixL];
            float hR = scene.terrainHeights[z * scene.terrainWidth + ixR];
            float hD = scene.terrainHeights[izD * scene.terrainWidth + x];
            float hU = scene.terrainHeights[izU * scene.terrainWidth + x];
            Vector3 normal = Vector3Normalize((Vector3){
                -(hR - hL) / (2.0f * scene.terrainCellSizeX),
                1.0f,
                -(hU - hD) / (2.0f * scene.terrainCellSizeZ)
            });
            int index = z * scene.terrainWidth + x;
            terrainMesh.normals[index * 3 + 0] = normal.x;
            terrainMesh.normals[index * 3 + 1] = normal.y;
            terrainMesh.normals[index * 3 + 2] = normal.z;
        }
    }

    int indexOffset = 0;
    for (int z = 0; z < scene.terrainLength - 1; z++) {
        for (int x = 0; x < scene.terrainWidth - 1; x++) {
            unsigned short i0 = (unsigned short)(z * scene.terrainWidth + x);
            unsigned short i1 = (unsigned short)(z * scene.terrainWidth + x + 1);
            unsigned short i2 = (unsigned short)((z + 1) * scene.terrainWidth + x);
            unsigned short i3 = (unsigned short)((z + 1) * scene.terrainWidth + x + 1);
            terrainMesh.indices[indexOffset++] = i0;
            terrainMesh.indices[indexOffset++] = i2;
            terrainMesh.indices[indexOffset++] = i1;
            terrainMesh.indices[indexOffset++] = i1;
            terrainMesh.indices[indexOffset++] = i2;
            terrainMesh.indices[indexOffset++] = i3;
        }
    }

    GenMeshTangents(&terrainMesh);
    UploadMesh(&terrainMesh, false);
    scene.terrainModel = LoadModelFromMesh(terrainMesh);
    scene.floorModel = scene.terrainModel;

    Mesh wallNS = GenMeshCube(width, height, thickness);
    GenMeshTangents(&wallNS);
    scene.wallModelNS = LoadModelFromMesh(wallNS);

    Mesh wallEW = GenMeshCube(thickness, height, length);
    GenMeshTangents(&wallEW);
    scene.wallModelEW = LoadModelFromMesh(wallEW);
    
    // Assign textures to models
    if (scene.floorTexture.id > 0) scene.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.floorTexture;
    else scene.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY; // Fallback color
    if (scene.floorNormalMap.id > 0) scene.terrainModel.materials[0].maps[MATERIAL_MAP_NORMAL].texture = scene.floorNormalMap;
    
    if (scene.wallTexture.id > 0) {
        scene.wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.wallTexture;
        scene.wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scene.wallTexture;
    } else {
        scene.wallModelNS.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
        scene.wallModelEW.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY; // Fallback color
    }

    // Assign lighting shader to all scene models' materials with safety checks
    if (scene.terrainModel.materialCount > 0) {
        scene.terrainModel.materials[0].shader = lightingShader;
    }
    if (scene.wallModelNS.materialCount > 0) {
        scene.wallModelNS.materials[0].shader = lightingShader;
    }
    if (scene.wallModelEW.materialCount > 0) {
        scene.wallModelEW.materials[0].shader = lightingShader;
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
    DrawModel(scene.terrainModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    
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

float GetTerrainHeightAt(Scene scene, float x, float z) {
    float minX = -scene.roomWidth * 0.5f;
    float minZ = -scene.roomLength * 0.5f;
    float gx = (x - minX) / scene.terrainCellSizeX;
    float gz = (z - minZ) / scene.terrainCellSizeZ;
    gx = Clamp(gx, 0.0f, (float)(scene.terrainWidth - 1));
    gz = Clamp(gz, 0.0f, (float)(scene.terrainLength - 1));

    int x0 = (int)floorf(gx);
    int z0 = (int)floorf(gz);
    int x1 = (x0 < scene.terrainWidth - 1) ? x0 + 1 : x0;
    int z1 = (z0 < scene.terrainLength - 1) ? z0 + 1 : z0;
    float tx = gx - (float)x0;
    float tz = gz - (float)z0;

    float h00 = scene.terrainHeights[z0 * scene.terrainWidth + x0];
    float h10 = scene.terrainHeights[z0 * scene.terrainWidth + x1];
    float h01 = scene.terrainHeights[z1 * scene.terrainWidth + x0];
    float h11 = scene.terrainHeights[z1 * scene.terrainWidth + x1];
    float hx0 = Lerp(h00, h10, tx);
    float hx1 = Lerp(h01, h11, tx);
    return Lerp(hx0, hx1, tz);
}

void UnloadScene(Scene scene) {
    // Unload models
    UnloadModel(scene.terrainModel);
    UnloadModel(scene.wallModelNS);
    UnloadModel(scene.wallModelEW);
    
    // Unload textures
    UnloadTexture(scene.wallTexture);
    UnloadTexture(scene.floorTexture);
    if (scene.floorNormalMap.id > 0) UnloadTexture(scene.floorNormalMap);
    
    // Free allocated memory
    free(scene.wallBoxes);
    free(scene.terrainHeights);
}
