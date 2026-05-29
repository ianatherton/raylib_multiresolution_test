#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "scene.h"
#include "props.h"

// Renderer context
typedef struct {
    RenderTexture2D fullResTarget;
    RenderTexture2D compositeTarget; // sharp color: scene + props
    RenderTexture2D blurPing;
    RenderTexture2D blurPong;
    Shader lightingShader;         // Lighting shader
    Shader dofBlurShader;
    Shader dofCompositeShader;
    Vector3 lightPosition;         // Light position
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

// Initialize renderer with screen dimensions
Renderer InitRenderer(int width, int height);
bool InitSkybox(Renderer* renderer, const char* pxPath, const char* nxPath, const char* pyPath, const char* nyPath, const char* pzPath, const char* nzPath);
void DrawSkybox(Renderer renderer, Camera3D camera);
bool InitSkyCloudDome(Renderer* renderer, const char* tilingCloudPngPath);
void DrawSkyCloudDome(Renderer renderer, Camera3D camera);

// Begin drawing to full resolution target
void BeginFullResRender(Renderer renderer);

// End drawing to full resolution target
void EndFullResRender(void);

// Composite render target to screen (camera used for world-space DOF distance)
void CompositeFinalFrame(Renderer renderer, Camera3D camera, int renderedProps, int visibleProps, float parallaxScale);

// Unload renderer resources
void UnloadRenderer(Renderer renderer);

#endif // RENDERER_H
