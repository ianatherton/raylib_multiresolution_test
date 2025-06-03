#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"

// Forward declaration to avoid circular dependency
struct GameState;

// Maximum number of rooms and props
#define MAX_ROOMS 10
#define MAX_PROPS 100

// Prop types
typedef enum {
    PROP_CHAIR,
    PROP_TABLE,
    PROP_LAMP,
    PROP_BOOKSHELF,
    PROP_BED,
    PROP_CHEST,
    PROP_COUNT
} PropType;

// Prop structure
typedef struct {
    PropType type;
    Model model;
    Texture2D texture;
    Vector3 position;
    Vector3 scale;
    float rotation;
    Color color;
    bool loaded;
} Prop;

// Room structure
typedef struct {
    Vector3 position;
    Vector3 size;
    Color wallColor;
    Color floorColor;
    Color ceilingColor;
    Rectangle doorways[4];  // North, East, South, West
    Prop props[MAX_PROPS];
    int propCount;
} Room;

// World structure
typedef struct World {
    Room rooms[MAX_ROOMS];
    int roomCount;
    Model propModels[PROP_COUNT];
    Texture2D propTextures[PROP_COUNT];
    Texture2D* highResTextures;
} World;

// Initialize world
void initWorld(World *world);

// World update function
void updateWorld(struct GameState *gameState);

// Check collision with world
bool checkCollision(World *world, Vector3 position, float radius);

// Unload world resources
void unloadWorld(World *world);

#endif // WORLD_H
