#include "character.h"
#include <stdio.h>

static const char* ANIM_PATHS[CHAR_ANIM_COUNT] = {
    "assets/character/sword_and_shield_idle.glb",
    "assets/character/sword_and_shield_walk.glb",
    "assets/character/sword_and_shield_run.glb",
    "assets/character/sword_and_shield_attack.glb",
    "assets/character/sword_and_shield_death.glb",
};

static const char* ANIM_NAMES[CHAR_ANIM_COUNT] = {
    "idle", "walk", "run", "attack", "death"
};

Character InitCharacter(Shader lightingShader) {
    Character ch = {0};
    ch.animFPS = 30.0f;
    ch.scale = 1.0f;

    ch.model = LoadModel("assets/character/mesh.glb");
    if (ch.model.meshCount == 0) {
        printf("ERROR: Failed to load character mesh\n");
        return ch;
    }

    // Mixamo GLB exports bake the diffuse texture into the emissive slot with a black
    // baseColor; move it to albedo so our shader's texture0 picks it up.
    for (int i = 0; i < ch.model.materialCount; i++) {
        Material *mat = &ch.model.materials[i];
        if (mat->maps == NULL) continue;
        Texture2D emTex = mat->maps[MATERIAL_MAP_EMISSION].texture;
        if (emTex.id > 0) {
            mat->maps[MATERIAL_MAP_ALBEDO].texture = emTex;
            mat->maps[MATERIAL_MAP_ALBEDO].color   = WHITE;
            mat->maps[MATERIAL_MAP_EMISSION].texture = (Texture2D){0};
        }
    }

    for (int i = 0; i < ch.model.materialCount; i++) {
        ch.model.materials[i].shader = lightingShader;
    }
    ApplyTextureFilterToAllMaterialMaps(ch.model, MAIN_TEXTURE_FILTER_MODE);

    for (int i = 0; i < CHAR_ANIM_COUNT; i++) {
        int count = 0;
        ch.anims[i] = LoadModelAnimations(ANIM_PATHS[i], &count);
        if (count > 0 && ch.anims[i] != NULL) {
            ch.animFrameCounts[i] = ch.anims[i][0].frameCount;
            printf("Character anim '%s': %d frames\n", ANIM_NAMES[i], ch.animFrameCounts[i]);
        } else {
            printf("WARNING: Failed to load anim '%s'\n", ANIM_NAMES[i]);
            ch.animFrameCounts[i] = 0;
        }
    }

    return ch;
}

void SetCharacterAnim(Character* ch, CharAnimIndex anim) {
    if (ch->currentAnim == anim) return;
    ch->currentAnim = anim;
    ch->currentFrame = 0;
    ch->frameTimer = 0.0f;
}

void UpdateCharacter(Character* ch, float dt) {
    int animIdx = ch->currentAnim;
    if (ch->anims[animIdx] == NULL || ch->animFrameCounts[animIdx] == 0) return;

    ModelAnimation* anim = &ch->anims[animIdx][0];

    ch->frameTimer += dt;
    float frameDur = 1.0f / ch->animFPS;
    while (ch->frameTimer >= frameDur) {
        ch->frameTimer -= frameDur;
        ch->currentFrame++;
        if (ch->currentFrame >= anim->frameCount) ch->currentFrame = 0;
    }

    UpdateModelAnimation(ch->model, *anim, ch->currentFrame);
}

void DrawCharacter(Character ch) {
    DrawModelEx(ch.model, ch.position,
                (Vector3){0.0f, 1.0f, 0.0f}, ch.yaw,
                (Vector3){ch.scale, ch.scale, ch.scale},
                WHITE);
}

void UnloadCharacter(Character* ch) {
    for (int i = 0; i < CHAR_ANIM_COUNT; i++) {
        if (ch->anims[i] != NULL) {
            UnloadModelAnimations(ch->anims[i], 1);
            ch->anims[i] = NULL;
        }
    }
    UnloadModel(ch->model);
}
