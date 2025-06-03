#include "renderer.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>  // For sprintf

void renderWorld(GameState *gameState) {
    // Step 1: Render the main scene (rooms and walls) to the main target
    BeginTextureMode(gameState->mainTarget);
        ClearBackground(BLACK);
        BeginMode3D(gameState->camera);
            // Draw rooms (walls, floors, ceilings)
            for (int i = 0; i < gameState->world->roomCount; i++) {
                drawRoom(&gameState->world->rooms[i], gameState->camera);
            }
        EndMode3D();
    EndTextureMode();
    
    // Step 2: Render props at low resolution
    drawPropsLowRes(gameState);
    
    // Step 3: Render props at high resolution for comparison
    drawPropsHighRes(gameState);
    
    // Step 4: Composite the final scene
    compositeScene(gameState);
    
    // Step 5: Draw debug information if enabled
    if (gameState->showDebugInfo) {
        drawDebugInfo(gameState);
    }
}

void drawRoom(Room *room, Camera3D camera) {
    // Draw floor
    DrawPlane((Vector3){room->position.x, room->position.y - room->size.y/2, room->position.z}, 
             (Vector2){room->size.x, room->size.z}, room->floorColor);
    
    // Draw ceiling
    DrawPlane((Vector3){room->position.x, room->position.y + room->size.y/2, room->position.z}, 
             (Vector2){room->size.x, room->size.z}, room->ceilingColor);
    
    // Draw walls with doorways
    
    // North wall (Z-)
    if (room->doorways[0].width > 0) {
        // Draw wall with doorway
        float doorX = room->doorways[0].x;
        float doorWidth = room->doorways[0].width;
        float doorHeight = room->doorways[0].height;
        
        // Left section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + (doorX + room->size.x/2)/2, 
            room->position.y, 
            room->position.z - room->size.z/2
        }, doorX + room->size.x/2, room->size.y, 0.1f, room->wallColor);
        
        // Right section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + doorX + doorWidth + (room->size.x/2 - doorX - doorWidth)/2, 
            room->position.y, 
            room->position.z - room->size.z/2
        }, room->size.x/2 - doorX - doorWidth, room->size.y, 0.1f, room->wallColor);
        
        // Top section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + doorX + doorWidth/2, 
            room->position.y + room->size.y/2 - (room->size.y - doorHeight)/2, 
            room->position.z - room->size.z/2
        }, doorWidth, room->size.y - doorHeight, 0.1f, room->wallColor);
    } else {
        // Draw full wall
        DrawCube((Vector3){
            room->position.x, 
            room->position.y, 
            room->position.z - room->size.z/2
        }, room->size.x, room->size.y, 0.1f, room->wallColor);
    }
    
    // East wall (X+)
    if (room->doorways[1].width > 0) {
        // Draw wall with doorway
        float doorZ = room->doorways[1].x;
        float doorWidth = room->doorways[1].width;
        float doorHeight = room->doorways[1].height;
        
        // Left section
        DrawCube((Vector3){
            room->position.x + room->size.x/2, 
            room->position.y, 
            room->position.z - room->size.z/2 + (doorZ + room->size.z/2)/2
        }, 0.1f, room->size.y, doorZ + room->size.z/2, room->wallColor);
        
        // Right section
        DrawCube((Vector3){
            room->position.x + room->size.x/2, 
            room->position.y, 
            room->position.z - room->size.z/2 + doorZ + doorWidth + (room->size.z/2 - doorZ - doorWidth)/2
        }, 0.1f, room->size.y, room->size.z/2 - doorZ - doorWidth, room->wallColor);
        
        // Top section
        DrawCube((Vector3){
            room->position.x + room->size.x/2, 
            room->position.y + room->size.y/2 - (room->size.y - doorHeight)/2, 
            room->position.z - room->size.z/2 + doorZ + doorWidth/2
        }, 0.1f, room->size.y - doorHeight, doorWidth, room->wallColor);
    } else {
        // Draw full wall
        DrawCube((Vector3){
            room->position.x + room->size.x/2, 
            room->position.y, 
            room->position.z
        }, 0.1f, room->size.y, room->size.z, room->wallColor);
    }
    
    // South wall (Z+)
    if (room->doorways[2].width > 0) {
        // Draw wall with doorway
        float doorX = room->doorways[2].x;
        float doorWidth = room->doorways[2].width;
        float doorHeight = room->doorways[2].height;
        
        // Left section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + (doorX + room->size.x/2)/2, 
            room->position.y, 
            room->position.z + room->size.z/2
        }, doorX + room->size.x/2, room->size.y, 0.1f, room->wallColor);
        
        // Right section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + doorX + doorWidth + (room->size.x/2 - doorX - doorWidth)/2, 
            room->position.y, 
            room->position.z + room->size.z/2
        }, room->size.x/2 - doorX - doorWidth, room->size.y, 0.1f, room->wallColor);
        
        // Top section
        DrawCube((Vector3){
            room->position.x - room->size.x/2 + doorX + doorWidth/2, 
            room->position.y + room->size.y/2 - (room->size.y - doorHeight)/2, 
            room->position.z + room->size.z/2
        }, doorWidth, room->size.y - doorHeight, 0.1f, room->wallColor);
    } else {
        // Draw full wall
        DrawCube((Vector3){
            room->position.x, 
            room->position.y, 
            room->position.z + room->size.z/2
        }, room->size.x, room->size.y, 0.1f, room->wallColor);
    }
    
    // West wall (X-)
    if (room->doorways[3].width > 0) {
        // Draw wall with doorway
        float doorZ = room->doorways[3].x;
        float doorWidth = room->doorways[3].width;
        float doorHeight = room->doorways[3].height;
        
        // Left section
        DrawCube((Vector3){
            room->position.x - room->size.x/2, 
            room->position.y, 
            room->position.z - room->size.z/2 + (doorZ + room->size.z/2)/2
        }, 0.1f, room->size.y, doorZ + room->size.z/2, room->wallColor);
        
        // Right section
        DrawCube((Vector3){
            room->position.x - room->size.x/2, 
            room->position.y, 
            room->position.z - room->size.z/2 + doorZ + doorWidth + (room->size.z/2 - doorZ - doorWidth)/2
        }, 0.1f, room->size.y, room->size.z/2 - doorZ - doorWidth, room->wallColor);
        
        // Top section
        DrawCube((Vector3){
            room->position.x - room->size.x/2, 
            room->position.y + room->size.y/2 - (room->size.y - doorHeight)/2, 
            room->position.z - room->size.z/2 + doorZ + doorWidth/2
        }, 0.1f, room->size.y - doorHeight, doorWidth, room->wallColor);
    } else {
        // Draw full wall
        DrawCube((Vector3){
            room->position.x - room->size.x/2, 
            room->position.y, 
            room->position.z
        }, 0.1f, room->size.y, room->size.z, room->wallColor);
    }
}

void drawPropsLowRes(GameState *gameState) {
    // Render props to the low-resolution target
    BeginTextureMode(gameState->propTarget);
        ClearBackground(BLANK); // Use BLANK for transparency
        
        // Set up the same camera but for the lower resolution
        Camera3D propCamera = gameState->camera;
        
        BeginMode3D(propCamera);
            // Draw props for each room
            for (int i = 0; i < gameState->world->roomCount; i++) {
                Room *room = &gameState->world->rooms[i];
                
                for (int j = 0; j < room->propCount; j++) {
                    Prop *prop = &room->props[j];
                    
                    if (prop->loaded) {
                        // Draw the model with its texture
                        DrawModelEx(
                            prop->model,
                            prop->position,
                            (Vector3){ 0.0f, 1.0f, 0.0f },
                            prop->rotation,
                            prop->scale,
                            prop->color
                        );
                    }
                }
            }
        EndMode3D();
    EndTextureMode();
}

void drawPropsHighRes(GameState *gameState) {
    // Render props to the high-resolution target
    BeginTextureMode(gameState->highResTarget);
        ClearBackground(BLANK); // Use BLANK for transparency
        
        BeginMode3D(gameState->camera);
            // Draw props for each room with high-resolution textures
            for (int i = 0; i < gameState->world->roomCount; i++) {
                Room *room = &gameState->world->rooms[i];
                
                for (int j = 0; j < room->propCount; j++) {
                    Prop *prop = &room->props[j];
                    
                    if (prop->loaded) {
                        // Temporarily swap to high-resolution texture
                        Texture2D originalTexture = prop->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture;
                        prop->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = 
                            gameState->world->highResTextures[prop->type];
                        
                        // Draw the model with high-resolution texture
                        DrawModelEx(
                            prop->model,
                            prop->position,
                            (Vector3){ 0.0f, 1.0f, 0.0f },
                            prop->rotation,
                            prop->scale,
                            prop->color
                        );
                        
                        // Restore original texture
                        prop->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = originalTexture;
                    }
                }
            }
        EndMode3D();
    EndTextureMode();
}

void compositeScene(GameState *gameState) {
    // Draw the main scene texture (rooms and walls)
    DrawTextureRec(
        gameState->mainTarget.texture,
        (Rectangle){ 0, 0, (float)gameState->mainTarget.texture.width, (float)-gameState->mainTarget.texture.height },
        (Vector2){ 0, 0 },
        WHITE
    );
    
    // Normal mode - draw either low-res or high-res props based on toggle
    
    // Enable depth testing but disable depth writing for proper occlusion
    rlEnableDepthTest();
    rlDisableDepthMask();
    
    if (gameState->showHighRes) {
        // Draw high-resolution props
        DrawTexturePro(
            gameState->highResTarget.texture,
            (Rectangle){ 0, 0, (float)gameState->highResTarget.texture.width, (float)-gameState->highResTarget.texture.height },
            (Rectangle){ 0, 0, (float)gameState->screenWidth, (float)gameState->screenHeight },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
    } else {
        // Draw low-resolution props
        DrawTexturePro(
            gameState->propTarget.texture,
            (Rectangle){ 0, 0, (float)gameState->propTarget.texture.width, (float)-gameState->propTarget.texture.height },
            (Rectangle){ 0, 0, (float)gameState->screenWidth, (float)gameState->screenHeight },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
    }
    
    // Reset depth testing state
    rlEnableDepthMask();
    rlDisableDepthTest();
}

void drawDebugInfo(GameState *gameState) {
    // Draw resolution info
    char resolutionText[100];
    sprintf(resolutionText, "Screen: %dx%d | Prop Target: %dx%d (%.0f%%)", 
            gameState->screenWidth, gameState->screenHeight,
            gameState->propTarget.texture.width, gameState->propTarget.texture.height,
            PROP_RENDER_SCALE * 100.0f);
    
    DrawRectangle(0, gameState->screenHeight - 60, gameState->screenWidth, 60, Fade(BLACK, 0.7f));
    DrawText(resolutionText, 10, gameState->screenHeight - 50, 20, WHITE);
    
    char modeText[100];
    sprintf(modeText, "Mode: %s", 
            gameState->showHighRes ? "HIGH RESOLUTION" : "LOW RESOLUTION");
    DrawText(modeText, 10, gameState->screenHeight - 25, 20, WHITE);
    
    // Draw controls
    DrawText("Controls: H = Toggle Resolution | I = Toggle Info", 
             10, 40, 20, WHITE);
}
