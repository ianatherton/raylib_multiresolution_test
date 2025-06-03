#include "world.h"
#include "raymath.h"
#include "props.h"
#include <stddef.h>  // For NULL
#include "../utils/utils.h"  // For MyColorBrightness

// Forward declarations for internal functions
static void createRoom(World *world, Vector3 position, Vector3 size, Color wallColor, 
                      Color floorColor, Color ceilingColor);
static void addProp(World *world, int roomIndex, PropType type, Vector3 position, 
                   Vector3 scale, float rotation, Color color);
static void loadPropResources(World *world);
static void generateTestWorld(World *world);

void initWorld(World *world) {
    // Initialize world state
    world->roomCount = 0;
    
    // Load prop resources (models and textures)
    loadPropResources(world);
    
    // Generate test world with rooms and props
    generateTestWorld(world);
}

void updateWorld(struct GameState *gameState) {
    // Currently no dynamic world updates needed
    // This could be expanded for animations, physics, etc.
    
    // Access world via pointer: gameState->world
}

bool checkCollision(World *world, Vector3 position, float radius) {
    // Check collision with room walls
    for (int i = 0; i < world->roomCount; i++) {
        Room *room = &world->rooms[i];
        
        // Check if player is in this room
        if (position.x >= room->position.x - room->size.x/2 && 
            position.x <= room->position.x + room->size.x/2 &&
            position.z >= room->position.z - room->size.z/2 && 
            position.z <= room->position.z + room->size.z/2) {
            
            // Check collision with walls (with a small margin)
            float margin = radius + 0.1f;
            
            // Check North wall
            if (position.z <= room->position.z - room->size.z/2 + margin) {
                // Check if there's a doorway
                bool inDoorway = false;
                if (room->doorways[0].width > 0) {
                    if (position.x >= room->position.x + room->doorways[0].x && 
                        position.x <= room->position.x + room->doorways[0].x + room->doorways[0].width) {
                        inDoorway = true;
                    }
                }
                if (!inDoorway) return true;
            }
            
            // Check East wall
            if (position.x >= room->position.x + room->size.x/2 - margin) {
                bool inDoorway = false;
                if (room->doorways[1].width > 0) {
                    if (position.z >= room->position.z + room->doorways[1].x && 
                        position.z <= room->position.z + room->doorways[1].x + room->doorways[1].width) {
                        inDoorway = true;
                    }
                }
                if (!inDoorway) return true;
            }
            
            // Check South wall
            if (position.z >= room->position.z + room->size.z/2 - margin) {
                bool inDoorway = false;
                if (room->doorways[2].width > 0) {
                    if (position.x >= room->position.x + room->doorways[2].x && 
                        position.x <= room->position.x + room->doorways[2].x + room->doorways[2].width) {
                        inDoorway = true;
                    }
                }
                if (!inDoorway) return true;
            }
            
            // Check West wall
            if (position.x <= room->position.x - room->size.x/2 + margin) {
                bool inDoorway = false;
                if (room->doorways[3].width > 0) {
                    if (position.z >= room->position.z + room->doorways[3].x && 
                        position.z <= room->position.z + room->doorways[3].x + room->doorways[3].width) {
                        inDoorway = true;
                    }
                }
                if (!inDoorway) return true;
            }
            
            // Check collision with props in this room
            for (int j = 0; j < room->propCount; j++) {
                Prop *prop = &room->props[j];
                
                // Simple circular collision check
                float dx = position.x - prop->position.x;
                float dz = position.z - prop->position.z;
                float distance = sqrtf(dx*dx + dz*dz);
                
                // Adjust collision radius based on prop type
                float propRadius = 0.5f;
                switch (prop->type) {
                    case PROP_TABLE: propRadius = 1.0f; break;
                    case PROP_BOOKSHELF: propRadius = 0.7f; break;
                    case PROP_BED: propRadius = 1.2f; break;
                    case PROP_CHEST: propRadius = 0.6f; break;
                    default: propRadius = 0.5f; break;
                }
                
                if (distance < radius + propRadius) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

void unloadWorld(World *world) {
    // Unload prop models and textures
    for (int i = 0; i < PROP_COUNT; i++) {
        UnloadModel(world->propModels[i]);
        UnloadTexture(world->propTextures[i]);
        
        // Unload high-resolution textures
        if (world->highResTextures) {
            UnloadTexture(world->highResTextures[i]);
        }
    }
    
    // Free high-resolution textures array
    if (world->highResTextures) {
        MemFree(world->highResTextures);
        world->highResTextures = NULL;
    }
}

// Internal function to create a room
static void createRoom(World *world, Vector3 position, Vector3 size, Color wallColor, 
                      Color floorColor, Color ceilingColor) {
    if (world->roomCount >= MAX_ROOMS) return;
    
    Room *room = &world->rooms[world->roomCount];
    room->position = position;
    room->size = size;
    room->wallColor = wallColor;
    room->floorColor = floorColor;
    room->ceilingColor = ceilingColor;
    room->propCount = 0;
    
    // Initialize doorways (no doorways by default)
    for (int i = 0; i < 4; i++) {
        room->doorways[i] = (Rectangle){ 0, 0, 0, 0 };
    }
    
    world->roomCount++;
}

// Internal function to add a prop to a room
static void addProp(World *world, int roomIndex, PropType type, Vector3 position, 
                   Vector3 scale, float rotation, Color color) {
    if (roomIndex < 0 || roomIndex >= world->roomCount) return;
    
    Room *room = &world->rooms[roomIndex];
    if (room->propCount >= MAX_PROPS) return;
    
    Prop *prop = &room->props[room->propCount];
    prop->type = type;
    prop->position = position;
    prop->scale = scale;
    prop->rotation = rotation;
    prop->color = color;
    prop->model = world->propModels[type];
    prop->texture = world->propTextures[type];
    prop->loaded = true;
    
    room->propCount++;
}

// Internal function to load prop resources
static void loadPropResources(World *world) {
    // Create simple prop models
    world->propModels[PROP_CHAIR] = LoadModelFromMesh(GenMeshCube(0.5f, 0.8f, 0.5f));
    world->propModels[PROP_TABLE] = LoadModelFromMesh(GenMeshCube(1.2f, 0.8f, 0.8f));
    world->propModels[PROP_LAMP] = LoadModelFromMesh(GenMeshCylinder(0.2f, 1.5f, 8));
    world->propModels[PROP_BOOKSHELF] = LoadModelFromMesh(GenMeshCube(1.0f, 2.0f, 0.4f));
    world->propModels[PROP_BED] = LoadModelFromMesh(GenMeshCube(1.8f, 0.5f, 0.9f));
    world->propModels[PROP_CHEST] = LoadModelFromMesh(GenMeshCube(0.8f, 0.6f, 0.5f));
    
    // Create visually distinct textures for props using our new texture functions
    
    // Chair texture - pixelated wooden texture with large pixels
    world->propTextures[PROP_CHAIR] = createPixelatedTexture(64, 64, BROWN, 8);
    
    // Table texture - checkerboard pattern
    world->propTextures[PROP_TABLE] = createCheckerboardTexture(64, 64, BROWN, MyColorBrightness(BROWN, 0.7f), 16);
    
    // Lamp texture - gradient from yellow to white
    world->propTextures[PROP_LAMP] = createGradientTexture(64, 64, GOLD, WHITE, false);
    
    // Bookshelf texture - pixelated with small pixels
    world->propTextures[PROP_BOOKSHELF] = createPixelatedTexture(64, 64, DARKBROWN, 4);
    
    // Bed texture - horizontal gradient
    world->propTextures[PROP_BED] = createGradientTexture(64, 64, DARKBLUE, BLUE, true);
    
    // Chest texture - noisy texture
    world->propTextures[PROP_CHEST] = createNoiseTexture(64, 64, BROWN, 0.3f);
    
    // Set textures for models
    for (int i = 0; i < PROP_COUNT; i++) {
        world->propModels[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = world->propTextures[i];
    }
    
    // Store original textures for comparison
    world->highResTextures = (Texture2D*)MemAlloc(sizeof(Texture2D) * PROP_COUNT);
    
    // Create high-resolution versions of the same textures (4x resolution)
    world->highResTextures[PROP_CHAIR] = createPixelatedTexture(256, 256, BROWN, 16);
    world->highResTextures[PROP_TABLE] = createCheckerboardTexture(256, 256, BROWN, MyColorBrightness(BROWN, 0.7f), 32);
    world->highResTextures[PROP_LAMP] = createGradientTexture(256, 256, GOLD, WHITE, false);
    world->highResTextures[PROP_BOOKSHELF] = createPixelatedTexture(256, 256, DARKBROWN, 8);
    world->highResTextures[PROP_BED] = createGradientTexture(256, 256, DARKBLUE, BLUE, true);
    world->highResTextures[PROP_CHEST] = createNoiseTexture(256, 256, BROWN, 0.3f);
}

// Internal function to generate test world
static void generateTestWorld(World *world) {
    // Create main room
    createRoom(world, (Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 10.0f, 3.0f, 10.0f }, 
              LIGHTGRAY, DARKGRAY, WHITE);
    
    // Add doorway to the north
    world->rooms[0].doorways[0] = (Rectangle){ -1.0f, 0.0f, 2.0f, 2.5f };
    
    // Add props to main room
    addProp(world, 0, PROP_TABLE, (Vector3){ 0.0f, 0.4f, 0.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    addProp(world, 0, PROP_CHAIR, (Vector3){ 0.0f, 0.4f, 1.5f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    addProp(world, 0, PROP_CHAIR, (Vector3){ 0.0f, 0.4f, -1.5f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, PI, WHITE);
    addProp(world, 0, PROP_LAMP, (Vector3){ 3.0f, 0.75f, 3.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    addProp(world, 0, PROP_BOOKSHELF, (Vector3){ -4.0f, 1.0f, -4.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    
    // Create second room (north)
    createRoom(world, (Vector3){ 0.0f, 0.0f, -15.0f }, (Vector3){ 12.0f, 3.0f, 10.0f }, 
              BEIGE, DARKBROWN, WHITE);
    
    // Add doorway to the south (connecting to main room)
    world->rooms[1].doorways[2] = (Rectangle){ -1.0f, 0.0f, 2.0f, 2.5f };
    
    // Add doorway to the east
    world->rooms[1].doorways[1] = (Rectangle){ -1.0f, 0.0f, 2.0f, 2.5f };
    
    // Add props to second room
    addProp(world, 1, PROP_BED, (Vector3){ -4.0f, 0.25f, -15.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, PI/2, WHITE);
    addProp(world, 1, PROP_CHEST, (Vector3){ -4.0f, 0.3f, -17.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    addProp(world, 1, PROP_LAMP, (Vector3){ -2.0f, 0.75f, -17.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    
    // Create third room (east of second room)
    createRoom(world, (Vector3){ 15.0f, 0.0f, -15.0f }, (Vector3){ 8.0f, 3.0f, 8.0f }, 
              SKYBLUE, DARKBLUE, WHITE);
    
    // Add doorway to the west (connecting to second room)
    world->rooms[2].doorways[3] = (Rectangle){ -1.0f, 0.0f, 2.0f, 2.5f };
    
    // Add props to third room
    addProp(world, 2, PROP_BOOKSHELF, (Vector3){ 13.0f, 1.0f, -13.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, PI/4, WHITE);
    addProp(world, 2, PROP_BOOKSHELF, (Vector3){ 13.0f, 1.0f, -17.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, -PI/4, WHITE);
    addProp(world, 2, PROP_TABLE, (Vector3){ 16.0f, 0.4f, -15.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, 0.0f, WHITE);
    addProp(world, 2, PROP_CHAIR, (Vector3){ 17.5f, 0.4f, -15.0f }, 
           (Vector3){ 1.0f, 1.0f, 1.0f }, PI/2, WHITE);
}
