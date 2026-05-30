#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "scene.h"
#include "props.h"

typedef struct {
    Shader lightingShader;
    Vector3 lightPosition;
    Shader skyboxShader;
    Model skyboxModel;
    TextureCubemap skyboxCubemap;
    bool hasSkybox;
    Model cloudDomeModel;
    Shader cloudDomeShader;
    int cloudDomeUvScaleLoc;
    int cloudDomeLayerOpaqueLoc;
    int cloudDomeTimeLoc;
    bool hasCloudDome;
} Renderer;

Renderer InitRenderer(void);
bool InitSkybox(Renderer* renderer, const char* pxPath, const char* nxPath, const char* pyPath, const char* nyPath, const char* pzPath, const char* nzPath);
void DrawSkybox(Renderer renderer, Camera3D camera);
bool InitSkyCloudDome(Renderer* renderer, const char* tilingCloudPngPath);
void DrawSkyCloudDome(Renderer renderer, Camera3D camera);
void UnloadRenderer(Renderer renderer);

#endif // RENDERER_H
