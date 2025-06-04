#ifndef PROPS_H
#define PROPS_H

#include "common.h"
#include "scene.h"

// Prop definition
typedef struct {
    Vector3* positions;
    int count;
    Texture2D texture;
    Rectangle sourceRec;
    Vector2 size;
    bool* visible;
} Props;

// Initialize props with positions and texture
Props InitProps(Vector3* positions, int count, const char* texturePath);

// Update prop visibility based on line of sight
void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera);

// Draw visible props
void DrawProps(Props props, Camera3D camera);

// Draw debug visualization for props
void DrawPropsDebug(Props props, Camera3D camera);

// Unload prop resources
void UnloadProps(Props props);

#endif // PROPS_H
