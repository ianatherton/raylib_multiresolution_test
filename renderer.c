#include "renderer.h"

Renderer InitRenderer(int width, int height, float propsScale) {
    Renderer renderer = {0};
    
    // Create render textures for full and quarter resolution
    renderer.fullResTarget = LoadRenderTexture(width, height);
    renderer.quarterResTarget = LoadRenderTexture(width * propsScale, height * propsScale);
    
    return renderer;
}

void BeginFullResRender(Renderer renderer) {
    BeginTextureMode(renderer.fullResTarget);
    ClearBackground(RAYWHITE); // Clear the render texture
}

void EndFullResRender(void) {
    EndTextureMode();
}

void BeginQuarterResRender(Renderer renderer) {
    BeginTextureMode(renderer.quarterResTarget);
    ClearBackground(BLANK); // Clear with transparency
}

void EndQuarterResRender(void) {
    EndTextureMode();
}

void CompositeFinalFrame(Renderer renderer) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Draw the full resolution environment
    DrawTextureRec(renderer.fullResTarget.texture,
                   (Rectangle){ 0, 0, (float)renderer.fullResTarget.texture.width, (float)-renderer.fullResTarget.texture.height }, // Source rec (flip Y)
                   (Vector2){ 0, 0 }, // Position
                   WHITE);
    
    // Draw the quarter resolution props, scaled up
    DrawTexturePro(renderer.quarterResTarget.texture,
                   (Rectangle){ 0, 0, (float)renderer.quarterResTarget.texture.width, (float)-renderer.quarterResTarget.texture.height }, // Source rec (flip Y)
                   (Rectangle){ 0, 0, renderer.fullResTarget.texture.width, renderer.fullResTarget.texture.height }, // Destination rec (scaled to full screen)
                   (Vector2){ 0, 0 }, // Origin
                   0.0f, // Rotation
                   WHITE);
    
    // Draw FPS counter
    DrawFPS(10, 10);
    
    EndDrawing();
}

void UnloadRenderer(Renderer renderer) {
    UnloadRenderTexture(renderer.fullResTarget);
    UnloadRenderTexture(renderer.quarterResTarget);
}
