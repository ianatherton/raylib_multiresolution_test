#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "scene.h"
#include "props.h"

// Renderer context
typedef struct {
    RenderTexture2D fullResTarget;
    RenderTexture2D quarterResTarget;
} Renderer;

// Initialize renderer with screen dimensions
Renderer InitRenderer(int width, int height, float propsScale);

// Begin drawing to full resolution target
void BeginFullResRender(Renderer renderer);

// End drawing to full resolution target
void EndFullResRender(void);

// Begin drawing to quarter resolution target
void BeginQuarterResRender(Renderer renderer);

// End drawing to quarter resolution target
void EndQuarterResRender(void);

// Composite both render targets to screen
void CompositeFinalFrame(Renderer renderer);

// Unload renderer resources
void UnloadRenderer(Renderer renderer);

#endif // RENDERER_H
