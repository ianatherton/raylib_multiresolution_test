#include "renderer.h"

Renderer InitRenderer(int width, int height, float propsScale) {
    Renderer renderer = {0};
    
    // Create render textures for full and quarter resolution
    renderer.fullResTarget = LoadRenderTexture(width, height);
    renderer.quarterResTarget = LoadRenderTexture(width * propsScale, height * propsScale);
    
    // Apply texture filtering to both render targets with their respective modes
    SetTextureFilter(renderer.fullResTarget.texture, MAIN_TEXTURE_FILTER_MODE);
    SetTextureFilter(renderer.quarterResTarget.texture, PROPS_TEXTURE_FILTER_MODE);
    
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

void CompositeFinalFrame(Renderer renderer, int renderedProps, int visibleProps) {
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
    
    // Draw props statistics
    DrawText(TextFormat("Rendered Props: %d/%d (%.1f%%)", 
             renderedProps, visibleProps, 
             visibleProps > 0 ? (float)renderedProps / visibleProps * 100.0f : 0), 
             10, 40, 20, WHITE);
    
    EndDrawing();
}

void UnloadRenderer(Renderer renderer) {
    UnloadRenderTexture(renderer.fullResTarget);
    UnloadRenderTexture(renderer.quarterResTarget);
}
