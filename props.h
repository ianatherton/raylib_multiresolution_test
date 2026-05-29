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
    PropType type;
    BoundingBox dummyBounds;       // CPU-side proxy volume for debug draw
    Vector3 dummyHalfExtents;
    bool isOccluder;
    unsigned int occlusionQuery;   // GL query object (0 = not yet created)
    bool queryPending;             // query issued last frame, result not yet read
    bool lastQueryVisible;         // last GPU result: true = visible (default)
    bool inRange;                  // passed distance cull this frame
} Prop;

// Props collection
typedef struct {
    Prop* props;
    int count;
    Texture2D billboardTexture;
    Rectangle billboardSourceRec;
    Vector2 billboardSize;
    Model model;
    bool rockHasNormalMap;
    int visibleCount;
    int renderedCount;
    unsigned int proxyVBO;   // one vec3 center point per prop
    unsigned int proxyVAO;
    Shader proxyShader;
    int proxyMvpLoc;
    Shader instancedShader;
    Material* rockInstancedMaterials;  // clone of model.materials using instancedShader
    Matrix* rockTransformBuffer;       // per-frame scratch for visible instance transforms
    int rockCount;
} Props;

Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath, const char* modelNormalMapPath, Shader lightingShader);

void AddBillboardProp(Props* props, Vector3 position, int index);
void AddModelProp(Props* props, Vector3 position, int index);

// Upload all prop proxy positions to the GPU VBO. Call once after all props are added.
void BuildProxyVBO(Props* props);

// Read back pending occlusion query results (non-blocking). Call at start of each frame.
void ReadPropOcclusionResults(Props* props);

// Distance-cull then update prop visibility from last query result.
void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera);

// Issue per-prop occlusion queries in the full-res pass after scene geometry is drawn.
void IssuePropOcclusionQueries(Props* props);

bool IsPointInFrustum(Vector3 point, Camera3D camera, float margin);
void DrawProps(Props* props, Camera3D camera);
void DrawPropsDebug(Props* props, Camera3D camera);
void UnloadProps(Props* props);

#endif // PROPS_H
