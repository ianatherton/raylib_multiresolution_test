#ifndef CHARACTER_H
#define CHARACTER_H

#include "common.h"

typedef enum {
    CHAR_ANIM_IDLE = 0,
    CHAR_ANIM_WALK,
    CHAR_ANIM_RUN,
    CHAR_ANIM_ATTACK,
    CHAR_ANIM_DEATH,
    CHAR_ANIM_COUNT
} CharAnimIndex;

typedef struct {
    Model model;
    ModelAnimation* anims[CHAR_ANIM_COUNT];
    int animFrameCounts[CHAR_ANIM_COUNT];
    CharAnimIndex currentAnim;
    int currentFrame;
    float frameTimer;
    float animFPS;
    Vector3 position;
    float yaw;
    float scale;
} Character;

Character InitCharacter(Shader lightingShader);
void SetCharacterAnim(Character* ch, CharAnimIndex anim);
void UpdateCharacter(Character* ch, float dt);
void DrawCharacter(Character ch);
void UnloadCharacter(Character* ch);

#endif // CHARACTER_H
