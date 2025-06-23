#include "raylib.h"
#include <stdio.h>

int main(void) {
    // Initialize window
    InitWindow(800, 600, "Shader Test");
    
    // Load shader
    Shader shader = LoadShader(
        "resources/shaders/lighting.vs",
        "resources/shaders/lighting.fs"
    );
    
    // Check if shader loaded correctly
    if (shader.id == 0) {
        printf("ERROR: Failed to load shader!\n");
        return 1;
    }
    printf("INFO: Shader loaded successfully (ID: %u)\n", shader.id);
    
    // Get uniform locations
    int locLightPos = GetShaderLocation(shader, "lightPos");
    int locLightColor = GetShaderLocation(shader, "lightColor");
    int locViewPos = GetShaderLocation(shader, "viewPos");
    
    printf("DEBUG: Shader uniform locations - lightPos: %d, lightColor: %d, viewPos: %d\n",
           locLightPos, locLightColor, locViewPos);
    
    // Create a basic cube
    Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    Model model = LoadModelFromMesh(mesh);
    
    // Assign shader to model
    model.materials[0].shader = shader;
    
    // Camera setup
    Camera camera = { 0 };
    camera.position = (Vector3){ 3.0f, 3.0f, 3.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Light position and color
    Vector3 lightPos = (Vector3){ 2.0f, 2.0f, 0.0f };
    Vector3 lightColor = (Vector3){ 1.0f, 1.0f, 1.0f };
    
    SetTargetFPS(60);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        UpdateCamera(&camera, CAMERA_ORBITAL);
        Vector3 viewPos = camera.position;
        
        // Update shader uniforms
        SetShaderValue(shader, locLightPos, &lightPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locLightColor, &lightColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locViewPos, &viewPos, SHADER_UNIFORM_VEC3);
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);
                DrawModel(model, (Vector3){0, 0, 0}, 1.0f, RED);
            EndMode3D();
            DrawFPS(10, 10);
        EndDrawing();
    }
    
    // Cleanup
    UnloadModel(model);
    UnloadShader(shader);
    CloseWindow();
    
    return 0;
}
