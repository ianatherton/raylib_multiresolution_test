#ifndef PROPS_H
#define PROPS_H

#include "common.h"
#include "scene.h"

typedef enum {
    PROP_BILLBOARD,
    PROP_MODEL
} PropType;

typedef struct {
    float x, y, z;
    float yaw;
    float pitch;
    float speed;
    float phase;
    float maxLean;
} GrassInstance;

typedef struct {
    Vector3 position;
    bool visible;
    PropType type;
    BoundingBox dummyBounds;
    Vector3 dummyHalfExtents;
    bool isOccluder;
    unsigned int occlusionQuery;
    bool queryPending;
    bool lastQueryVisible;
    bool inRange;
} Prop;

typedef struct {
    Prop* props;
    int count;
    int rockCount;

    // Billboard (grass) instancing
    GrassInstance* grassInstances;      // precomputed per-grass data, indexed [0..count-rockCount)
    GrassInstance* grassVisibleScratch; // per-frame packed visible subset
    unsigned int grassVAO;
    unsigned int grassQuadVBO;
    unsigned int grassInstanceVBO;
    Shader grassShader;
    int grassMvpLoc;
    int grassTimeLoc;
    int grassCamPosLoc;
    int grassTexLoc;

    Texture2D billboardTexture;
    Rectangle billboardSourceRec;
    Vector2 billboardSize;

    // Rock instancing
    Model model;
    bool rockHasNormalMap;
    Shader instancedShader;
    Material* rockInstancedMaterials;
    Matrix* rockTransformBuffer;

    // Occlusion proxy
    unsigned int proxyVBO;
    unsigned int proxyVAO;
    Shader proxyShader;
    int proxyMvpLoc;

    int visibleCount;
    int renderedCount;
} Props;

Props InitProps(int billboardCount, int modelCount, const char* billboardTexturePath, const char* modelPath, const char* modelTexturePath, const char* modelNormalMapPath, Shader lightingShader);

void AddBillboardProp(Props* props, Vector3 position, int index);
void AddModelProp(Props* props, Vector3 position, int index);

void BuildProxyVBO(Props* props);
void ReadPropOcclusionResults(Props* props);
void UpdatePropVisibility(Props* props, Scene scene, Camera3D camera);
void IssuePropOcclusionQueries(Props* props);

bool IsPointInFrustum(Vector3 point, Camera3D camera, float margin);
void DrawProps(Props* props, Camera3D camera);
void DrawPropsDebug(Props* props, Camera3D camera);
void UnloadProps(Props* props);

#endif // PROPS_H
