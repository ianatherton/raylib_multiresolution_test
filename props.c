#include "props.h"
#include <stdlib.h>

Props InitProps(Vector3* positions, int count, const char* texturePath) {
    Props props = {0};
    
    // Allocate memory for positions and copy data
    props.positions = (Vector3*)malloc(count * sizeof(Vector3));
    for (int i = 0; i < count; i++) {
        props.positions[i] = positions[i];
    }
    
    props.count = count;
    
    // Load texture
    props.texture = LoadTexture(texturePath);
    if (props.texture.id == 0) {
        printf("Failed to load prop texture: %s\n", texturePath);
    }
    
    // Set up source rectangle for the texture
    props.sourceRec = (Rectangle){ 0.0f, 0.0f, (float)props.texture.width, (float)props.texture.height };
    
    // Default billboard size
    props.size = (Vector2){ 1.0f, 1.0f };
    
    // Allocate and initialize visibility array
    props.visible = (bool*)malloc(count * sizeof(bool));
    for (int i = 0; i < count; i++) {
        props.visible[i] = true;
    }
    
    return props;
}

void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera) {
    for (int i = 0; i < props->count; i++) {
        // Default to visible
        props->visible[i] = true;
        
        // Direction vector from camera to prop
        Vector3 direction = Vector3Subtract(props->positions[i], camera.position);
        float distance = Vector3Length(direction);
        direction = Vector3Normalize(direction);
        
        // Create a ray from camera position toward the prop
        Ray ray = { camera.position, direction };
        
        // Check for intersection with each wall
        for (int j = 0; j < scene.numWalls; j++) {
            // Check for collision with the wall's bounding box
            RayCollision collision = GetRayCollisionBox(ray, scene.wallBoxes[j]);
            
            // If there's a hit and it's closer than the prop, the prop is occluded
            if (collision.hit && collision.distance < distance) {
                props->visible[i] = false;
                break;
            }
        }
        
        // Debug: Print ray and collision info for the first prop
        if (i == 0) {
            printf("Camera: (%.2f, %.2f, %.2f) -> Prop: (%.2f, %.2f, %.2f), Visible: %s\n", 
                   camera.position.x, camera.position.y, camera.position.z,
                   props->positions[i].x, props->positions[i].y, props->positions[i].z,
                   props->visible[i] ? "Yes" : "No");
        }
    }
}

void DrawProps(Props props, Camera3D camera) {
    // Draw grass props as billboards, but only if visible
    for (int i = 0; i < props.count; i++) {
        if (props.visible[i]) {
            DrawBillboardRec(camera, props.texture, props.sourceRec, props.positions[i], props.size, WHITE);
        }
    }
}

void DrawPropsDebug(Props props, Camera3D camera) {
    // Draw rays from camera to props
    for (int i = 0; i < props.count; i++) {
        Color rayColor = props.visible[i] ? GREEN : RED;
        DrawLine3D(camera.position, props.positions[i], rayColor);
    }
}

void UnloadProps(Props props) {
    // Unload texture
    UnloadTexture(props.texture);
    
    // Free allocated memory
    free(props.positions);
    free(props.visible);
}
