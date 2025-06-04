#ifndef PROPS_H
#define PROPS_H

#include "common.h"
#include "scene.h"

// Prop types
typedef enum {
    PROP_BILLBOARD,  // 2D billboard (grass, etc.)
    PROP_MODEL       // 3D model (rocks, etc.)
} PropType;

// Individual prop data
typedef struct {
    Vector3 position;
    bool visible;
    PropType type;   // Type of prop (billboard or 3D model)
} Prop;

// Props collection
typedef struct {
    Prop* props;
    int count;
    Texture2D billboardTexture;  // Texture for billboard props
    Rectangle billboardSourceRec; // Source rectangle for billboard texture
    Vector2 billboardSize;       // Size of billboards
    Model model;                 // 3D model for model props
    Vector3 lastCameraPosition;  // Last camera position when LOS was checked
    bool needsLOSUpdate;         // Flag to force LOS update
} Props;

// Initialize props with billboard and model data
Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath);

// Add a billboard prop at the specified position
void AddBillboardProp(Props* props, Vector3 position, int index);

// Add a model prop at the specified position
void AddModelProp(Props* props, Vector3 position, int index);

// Update prop visibility based on line of sight
void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera);

// Draw visible props
void DrawProps(Props props, Camera3D camera);

// Draw debug visualization for props
void DrawPropsDebug(Props props, Camera3D camera);

// Unload prop resources
void UnloadProps(Props props);

#endif // PROPS_H
