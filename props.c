#include "props.h"
#include <stdlib.h>
#include <string.h>

Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath, Shader lightingShader) {
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
    } else {
        // Apply texture filtering to billboard texture
        SetTextureFilter(props.billboardTexture, PROPS_TEXTURE_FILTER_MODE);
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
            
            // Apply texture filtering to model texture
            SetTextureFilter(modelTexture, PROPS_TEXTURE_FILTER_MODE);
            
            // Apply texture to all materials in the model
            for (int i = 0; i < props.model.materialCount; i++) {
                SetMaterialTexture(&props.model.materials[i], MATERIAL_MAP_DIFFUSE, modelTexture);
            }

            // Assign lighting shader to all model materials with safety checks
            if (props.model.materialCount > 0 && props.model.materials != NULL) {
                for (int i = 0; i < props.model.materialCount; i++) {
                    props.model.materials[i].shader = lightingShader;
                }
            }
            
            // Debug info about the model
            printf("Rock model has %d materials and %d meshes\n", 
                   props.model.materialCount, props.model.meshCount);
        }
    }
    
    // Initialize LOS optimization fields
    props.lastCameraPosition = (Vector3){ 0.0f, 0.0f, 0.0f };
    props.needsLOSUpdate = true;  // Force initial update
    props.visibleCount = 0;
    props.renderedCount = 0;
    
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
    float cameraMoveDistance = Vector3Distance(camera.position, props->lastCameraPosition);
    bool shouldUpdate = props->needsLOSUpdate || (cameraMoveDistance >= LOS_MIN_CAMERA_MOVE);
    
    if (!shouldUpdate) return;
    
    props->lastCameraPosition = camera.position;
    props->needsLOSUpdate = false;
    
    int visibleCount = 0;
    int totalCount = 0;
    
    for (int i = 0; i < props->count; i++) {
        // Skip inactive props (position at origin)
        if (props->props[i].position.x == 0 && 
            props->props[i].position.y == 0 && 
            props->props[i].position.z == 0) continue;
        
        totalCount++;
        
        // Check if prop is beyond max distance
        Vector3 direction = Vector3Subtract(props->props[i].position, camera.position);
        float distance = Vector3Length(direction);
        
        if (distance > LOS_MAX_PROP_DISTANCE) {
            props->props[i].visible = false;
            continue;
        }
        
        // Default to visible
        props->props[i].visible = true;
        visibleCount++;
        
        // Check for wall occlusion
        direction = Vector3Normalize(direction);
        Ray ray = {camera.position, direction};
        
        for (int j = 0; j < scene.numWalls; j++) {
            RayCollision collision = GetRayCollisionBox(ray, scene.wallBoxes[j]);
            
            if (collision.hit && collision.distance < distance) {
                props->props[i].visible = false;
                visibleCount--;
                break;
            }
        }
        
        // Debug: Print ray and collision info for the first prop
        if (i == 0) {
            printf("Camera: (%.2f, %.2f, %.2f) -> Prop: (%.2f, %.2f, %.2f), Visible: %s\n", 
                   camera.position.x, camera.position.y, camera.position.z,
                   props->props[i].position.x, props->props[i].position.y, props->props[i].position.z,
                   props->props[i].visible ? "Yes" : "No");
        }
    }
    
    // Store the visible count
    props->visibleCount = visibleCount;
    
    // Debug output - show how many props are visible
    printf("LOS Update: Camera moved %.2f units\n", cameraMoveDistance);
    printf("Props visible: %d/%d (%.1f%%)\n", 
           visibleCount, totalCount, 
           totalCount > 0 ? (float)visibleCount / totalCount * 100.0f : 0.0f);
}

// Helper function to check if a point is inside the camera frustum
bool IsPointInFrustum(Vector3 point, Camera3D camera, float margin) {
    // Convert point to view space
    Matrix viewMatrix = MatrixLookAt(camera.position, camera.target, camera.up);
    Vector3 viewSpacePoint = Vector3Transform(point, viewMatrix);
    
    // Early discard points behind the camera
    if (viewSpacePoint.z > 0) return false;
    
    // Calculate frustum boundaries at the point's depth
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float nearPlaneHeight = 2.0f * fabsf(viewSpacePoint.z) * tanf(camera.fovy * 0.5f * DEG2RAD);
    float nearPlaneWidth = nearPlaneHeight * aspect;
    
    // Add margin to frustum boundaries
    nearPlaneWidth += margin;
    nearPlaneHeight += margin;
    
    // Check if point is within frustum boundaries
    return (fabsf(viewSpacePoint.x) < nearPlaneWidth * 0.5f) && 
           (fabsf(viewSpacePoint.y) < nearPlaneHeight * 0.5f);
}

void DrawProps(Props* props, Camera3D camera) {
    // Reset rendered count
    props->renderedCount = 0;
    
    // Draw props based on their type
    for (int i = 0; i < props->count; i++) {
        // Skip props that aren't visible due to LOS
        if (!props->props[i].visible) continue;
        
        // Frustum culling - skip props outside the camera frustum
        // Use a small margin (1.0f) to avoid popping at frustum edges
        if (!IsPointInFrustum(props->props[i].position, camera, 1.0f)) continue;
        
        // Increment rendered count
        props->renderedCount++;
        
        // Draw based on prop type
        switch (props->props[i].type) {
            case PROP_BILLBOARD:
                // Draw as billboard
                DrawBillboardRec(camera, props->billboardTexture, props->billboardSourceRec, 
                                props->props[i].position, props->billboardSize, WHITE);
                break;
                
            case PROP_MODEL:
                // Draw as 3D model with proper positioning and scale
                // Add a small random rotation to make each rock look different
                float scale = 0.5f;
                float rotationAngle = (float)((i * 37) % 360); // Different rotation for each rock
                
                // Draw the model with position, rotation and scale
                DrawModelEx(props->model, 
                           props->props[i].position,       // Position
                           (Vector3){0.0f, 1.0f, 0.0f},  // Rotation axis (Y-up)
                           rotationAngle,                // Rotation angle
                           (Vector3){scale, scale, scale}, // Scale
                           WHITE);                       // Tint
                break;
        }
    }
}

void DrawPropsDebug(Props* props, Camera3D camera) {
    for (int i = 0; i < props->count; i++) {
        // Skip props that aren't active
        if (props->props[i].position.x == 0.0f && 
            props->props[i].position.y == 0.0f && 
            props->props[i].position.z == 0.0f) {
            continue;
        }
        
        // Draw ray from camera to prop
        Vector3 direction = Vector3Subtract(props->props[i].position, camera.position);
        float distance = Vector3Length(direction);
        
        // Skip props that are too far away
        if (distance > LOS_MAX_PROP_DISTANCE) {
            continue;
        }
        
        // Draw ray in green if visible, red if not
        Color rayColor = props->props[i].visible ? GREEN : RED;
        DrawLine3D(camera.position, props->props[i].position, rayColor);
        
        // Draw sphere at prop position - blue for billboards, yellow for models
        Color sphereColor = props->props[i].type == PROP_BILLBOARD ? BLUE : YELLOW;
        DrawSphere(props->props[i].position, 0.1f, sphereColor);
    }
}

void UnloadProps(Props* props) {
    // Unload textures
    UnloadTexture(props->billboardTexture);
    
    // Unload model
    UnloadModel(props->model);
    
    // Free memory
    free(props->props);
}
