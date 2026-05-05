#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "scene.h"
#include "props.h"

// Renderer context
typedef struct {
    RenderTexture2D fullResTarget;
    RenderTexture2D quarterResTarget;
    Shader lightingShader;         // Lighting shader
    Vector3 lightPosition;         // Light position
    Shader skyboxShader;
    Model skyboxModel;
    TextureCubemap skyboxCubemap;
    bool hasSkybox;
} Renderer;

// Initialize renderer with screen dimensions
Renderer InitRenderer(int width, int height, float propsScale);
bool InitSkybox(Renderer* renderer, const char* pxPath, const char* nxPath, const char* pyPath, const char* nyPath, const char* pzPath, const char* nzPath);
void DrawSkybox(Renderer renderer, Camera3D camera);

// Begin drawing to full resolution target
void BeginFullResRender(Renderer renderer);

// End drawing to full resolution target
void EndFullResRender(void);

// Begin drawing to quarter resolution target
void BeginQuarterResRender(Renderer renderer);

// End drawing to quarter resolution target
void EndQuarterResRender(void);

// Composite both render targets to screen
void CompositeFinalFrame(Renderer renderer, int renderedProps, int visibleProps);

// Unload renderer resources
void UnloadRenderer(Renderer renderer);

#endif // RENDERER_H
