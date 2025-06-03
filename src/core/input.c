#include "input.h"
#include "raymath.h"

void updateInput(GameState *gameState) {
    // Update delta time
    updateDeltaTime(gameState);
    
    // Toggle demo features with keyboard
    if (IsKeyPressed(KEY_H)) {
        gameState->showHighRes = !gameState->showHighRes;
    }
    
    if (IsKeyPressed(KEY_I)) {
        gameState->showDebugInfo = !gameState->showDebugInfo;
    }
    
    // Update demo timer for automatic toggling
    gameState->demoTimer += gameState->deltaTime;
    if (gameState->demoTimer > 3.0f) {
        gameState->demoTimer = 0.0f;
        gameState->showHighRes = !gameState->showHighRes;
    }
    
    // Mouse look (first-person camera rotation)
    if (IsWindowFocused()) {
        // Get mouse delta
        Vector2 mouseDelta = GetMouseDelta();
        
        // Calculate camera rotation
        float yaw = atan2f(gameState->camera.target.x - gameState->camera.position.x,
                          gameState->camera.target.z - gameState->camera.position.z);
        
        // Apply mouse movement to camera rotation (yaw and pitch)
        yaw -= mouseDelta.x * gameState->mouseSensitivity;
        
        // Calculate forward vector based on yaw
        Vector3 forward = {
            sinf(yaw),
            0.0f,
            cosf(yaw)
        };
        
        // Update camera target
        gameState->camera.target.x = gameState->camera.position.x + forward.x;
        gameState->camera.target.z = gameState->camera.position.z + forward.z;
        
        // Apply pitch (limited to avoid flipping)
        float pitch = atan2f(gameState->camera.target.y - gameState->camera.position.y,
                           sqrtf(powf(gameState->camera.target.x - gameState->camera.position.x, 2) +
                                 powf(gameState->camera.target.z - gameState->camera.position.z, 2)));
        
        pitch -= mouseDelta.y * gameState->mouseSensitivity;
        
        // Clamp pitch to avoid flipping
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
        
        // Calculate distance to target (to maintain same distance after pitch change)
        float targetDistance = Vector3Distance(gameState->camera.position, gameState->camera.target);
        
        // Update target Y position based on pitch
        float height = targetDistance * sinf(pitch);
        gameState->camera.target.y = gameState->camera.position.y + height;
        
        // Normalize the forward vector after pitch adjustment
        forward = Vector3Normalize(Vector3Subtract(gameState->camera.target, gameState->camera.position));
        
        // Scale to maintain original distance
        forward = Vector3Scale(forward, targetDistance);
        
        // Set final target
        gameState->camera.target = Vector3Add(gameState->camera.position, forward);
    }
    
    // Movement (WASD)
    Vector3 moveVec = { 0.0f, 0.0f, 0.0f };
    
    // Calculate forward and right vectors from camera
    Vector3 forward = Vector3Normalize(Vector3Subtract(
        (Vector3){ gameState->camera.target.x, 0, gameState->camera.target.z },
        (Vector3){ gameState->camera.position.x, 0, gameState->camera.position.z }
    ));
    
    Vector3 right = Vector3CrossProduct(forward, (Vector3){ 0.0f, 1.0f, 0.0f });
    
    // Apply movement based on key input
    if (IsKeyDown(KEY_W)) moveVec = Vector3Add(moveVec, forward);
    if (IsKeyDown(KEY_S)) moveVec = Vector3Subtract(moveVec, forward);
    if (IsKeyDown(KEY_D)) moveVec = Vector3Add(moveVec, right);
    if (IsKeyDown(KEY_A)) moveVec = Vector3Subtract(moveVec, right);
    
    // Normalize movement vector if not zero
    if (Vector3Length(moveVec) > 0) {
        moveVec = Vector3Normalize(moveVec);
    }
    
    // Apply speed and delta time
    moveVec = Vector3Scale(moveVec, gameState->playerSpeed * gameState->deltaTime);
    
    // Update player position
    Vector3 newPosition = Vector3Add(gameState->playerPosition, moveVec);
    
    // Check collision with walls and adjust position if needed
    if (!checkCollision(gameState->world, newPosition, 0.5f)) {
        gameState->playerPosition = newPosition;
        
        // Update camera position
        gameState->camera.position = (Vector3){
            gameState->playerPosition.x,
            gameState->playerPosition.y + 1.8f,  // Eye height
            gameState->playerPosition.z
        };
        
        // Update camera target to maintain the same view direction
        Vector3 direction = Vector3Normalize(Vector3Subtract(gameState->camera.target, 
                                                           (Vector3){
                                                               gameState->camera.position.x - moveVec.x,
                                                               gameState->camera.position.y - moveVec.y,
                                                               gameState->camera.position.z - moveVec.z
                                                           }));
        
        gameState->camera.target = Vector3Add(gameState->camera.position, direction);
    }
}
