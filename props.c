#include "props.h"
#include <stdlib.h>
#include <string.h>

Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath) {
    Props props = {0};
    int totalCount = billboardCount + modelCount;
    
    // Allocate memory for props array
    props.props = (Prop*)malloc(totalCount * sizeof(Prop));
    props.count = totalCount;
    
    // Initialize all props as invisible initially
    for (int i = 0; i < totalCount; i++) {
        props.props[i].visible = false;
        props.props[i].position = (Vector3){ 0.0f, 0.0f, 0.0f };
    }
    
    // Load billboard texture
    props.billboardTexture = LoadTexture(billboardTexturePath);
    if (props.billboardTexture.id == 0) {
        printf("Failed to load billboard texture: %s\n", billboardTexturePath);
    }
    
    // Set up source rectangle for the billboard texture
    props.billboardSourceRec = (Rectangle){ 0.0f, 0.0f, (float)props.billboardTexture.width, (float)props.billboardTexture.height };
    
    // Default billboard size
    props.billboardSize = (Vector2){ 1.0f, 1.0f };
    
    // Load 3D model for rocks
    props.model = LoadModel(modelPath);
    
    // Load model texture if provided
    if (modelTexturePath != NULL && strlen(modelTexturePath) > 0) {
        Texture2D modelTexture = LoadTexture(modelTexturePath);
        if (modelTexture.id == 0) {
            printf("Failed to load rock texture: %s\n", modelTexturePath);
        } else {
            printf("Successfully loaded rock texture: %s (ID: %u)\n", modelTexturePath, modelTexture.id);
            
            // Apply texture to all materials in the model
            for (int i = 0; i < props.model.materialCount; i++) {
                SetMaterialTexture(&props.model.materials[i], MATERIAL_MAP_DIFFUSE, modelTexture);
            }
            
            // Debug info about the model
            printf("Rock model has %d materials and %d meshes\n", 
                   props.model.materialCount, props.model.meshCount);
        }
    }
    
    // Initialize LOS optimization fields
    props.lastCameraPosition = (Vector3){ 0.0f, 0.0f, 0.0f };
    props.needsLOSUpdate = true;  // Force initial update
    
    return props;
}

void AddBillboardProp(Props* props, Vector3 position, int index) {
    if (index >= 0 && index < props->count) {
        props->props[index].position = position;
        props->props[index].type = PROP_BILLBOARD;
        props->props[index].visible = true;
    }
}

void AddModelProp(Props* props, Vector3 position, int index) {
    if (index >= 0 && index < props->count) {
        props->props[index].position = position;
        props->props[index].type = PROP_MODEL;
        props->props[index].visible = true;
    }
}

void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera) {
    // Check if we need to update LOS based on camera movement
    float cameraMoveDistance = Vector3Distance(camera.position, props->lastCameraPosition);
    bool shouldUpdate = props->needsLOSUpdate || (cameraMoveDistance >= LOS_MIN_CAMERA_MOVE);
    
    // If no update needed, return early
    if (!shouldUpdate) {
        return;
    }
    
    // Update the last camera position and reset the flag
    props->lastCameraPosition = camera.position;
    props->needsLOSUpdate = false;
    
    // Debug: Print when LOS update happens
    printf("LOS Update: Camera moved %.2f units\n", cameraMoveDistance);
    
    int visibleCount = 0;
    int totalCount = 0;
    
    for (int i = 0; i < props->count; i++) {
        // Skip props that aren't active
        if (props->props[i].position.x == 0.0f && 
            props->props[i].position.y == 0.0f && 
            props->props[i].position.z == 0.0f) {
            continue;
        }
        
        totalCount++;
        
        // Direction vector from camera to prop
        Vector3 direction = Vector3Subtract(props->props[i].position, camera.position);
        float distance = Vector3Length(direction);
        
        // Skip props that are too far away (automatically mark as not visible)
        if (distance > LOS_MAX_PROP_DISTANCE) {
            props->props[i].visible = false;
            continue;
        }
        
        // Default to visible for props within range
        props->props[i].visible = true;
        
        // Normalize direction for ray casting
        direction = Vector3Normalize(direction);
        
        // Create a ray from camera position toward the prop
        Ray ray = { camera.position, direction };
        
        // Check for intersection with each wall
        for (int j = 0; j < scene.numWalls; j++) {
            // Check for collision with the wall's bounding box
            RayCollision collision = GetRayCollisionBox(ray, scene.wallBoxes[j]);
            
            // If there's a hit and it's closer than the prop, the prop is occluded
            if (collision.hit && collision.distance < distance) {
                props->props[i].visible = false;
                break;
            }
        }
        
        if (props->props[i].visible) {
            visibleCount++;
        }
        
        // Debug: Print ray and collision info for the first prop
        if (i == 0) {
            printf("Camera: (%.2f, %.2f, %.2f) -> Prop: (%.2f, %.2f, %.2f), Visible: %s\n", 
                   camera.position.x, camera.position.y, camera.position.z,
                   props->props[i].position.x, props->props[i].position.y, props->props[i].position.z,
                   props->props[i].visible ? "Yes" : "No");
        }
    }
    
    // Print visibility statistics
    printf("Props visible: %d/%d (%.1f%%)\n", visibleCount, totalCount, 
           totalCount > 0 ? (float)visibleCount / totalCount * 100.0f : 0.0f);
}

void DrawProps(Props props, Camera3D camera) {
    // Draw props based on their type
    for (int i = 0; i < props.count; i++) {
        // Skip props that aren't visible
        if (!props.props[i].visible) continue;
        
        // Draw based on prop type
        switch (props.props[i].type) {
            case PROP_BILLBOARD:
                // Draw as billboard
                DrawBillboardRec(camera, props.billboardTexture, props.billboardSourceRec, 
                                props.props[i].position, props.billboardSize, WHITE);
                break;
                
            case PROP_MODEL:
                // Draw as 3D model with proper positioning and scale
                // Add a small random rotation to make each rock look different
                float scale = 0.5f;
                float rotationAngle = (float)((i * 37) % 360); // Different rotation for each rock
                
                // Draw the model with position, rotation and scale
                DrawModelEx(props.model, 
                           props.props[i].position,       // Position
                           (Vector3){0.0f, 1.0f, 0.0f},  // Rotation axis (Y-up)
                           rotationAngle,                // Rotation angle
                           (Vector3){scale, scale, scale}, // Scale
                           WHITE);                       // Tint
                break;
        }
    }
}

void DrawPropsDebug(Props props, Camera3D camera) {
    // Draw rays from camera to props
    for (int i = 0; i < props.count; i++) {
        // Skip props that aren't active
        if (props.props[i].position.x == 0.0f && 
            props.props[i].position.y == 0.0f && 
            props.props[i].position.z == 0.0f) {
            continue;
        }
        
        // Draw ray with color based on visibility
        Color rayColor = props.props[i].visible ? GREEN : RED;
        DrawLine3D(camera.position, props.props[i].position, rayColor);
        
        // Draw a small sphere at the prop position
        DrawSphere(props.props[i].position, 0.1f, props.props[i].type == PROP_BILLBOARD ? BLUE : YELLOW);
    }
}

void UnloadProps(Props props) {
    // Unload textures and models
    UnloadTexture(props.billboardTexture);
    UnloadModel(props.model);
    
    // Free allocated memory
    free(props.props);
}
