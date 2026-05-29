#include "character.h"
#include <stdio.h>

// Remove horizontal root motion from an animation. framePoses holds global bone
// transforms, so the root's XZ offset is already baked into every descendant.
// We subtract the root's per-frame XZ from all bones so the skeleton stays
// centred at origin XZ; the caller drives world position via character.position.
static void StripRootMotionXZ(ModelAnimation* anim) {
    for (int f = 0; f < anim->frameCount; f++) {
        float dx = anim->framePoses[f][0].translation.x;
        float dz = anim->framePoses[f][0].translation.z;
        for (int b = 0; b < anim->boneCount; b++) {
            anim->framePoses[f][b].translation.x -= dx;
            anim->framePoses[f][b].translation.z -= dz;
        }
    }
}

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

    // The GLB has no tangents; compute them so normal mapping works.
    for (int mi = 0; mi < ch.model.meshCount; mi++) {
        GenMeshTangents(&ch.model.meshes[mi]);
        UploadMesh(&ch.model.meshes[mi], false);
    }

    Texture2D diffuseTex = LoadTexture("assets/character/texture_diffuse.png");
    Texture2D normalTex  = LoadTexture("assets/character/texture_normal.png");

    for (int i = 0; i < ch.model.materialCount; i++) {
        Material *mat = &ch.model.materials[i];
        if (mat->maps == NULL) continue;

        // Replace the emissive-slot bake with the clean diffuse.
        if (diffuseTex.id > 0) {
            mat->maps[MATERIAL_MAP_ALBEDO].texture = diffuseTex;
            mat->maps[MATERIAL_MAP_ALBEDO].color   = WHITE;
            mat->maps[MATERIAL_MAP_EMISSION].texture = (Texture2D){0};
        } else {
            Texture2D emTex = mat->maps[MATERIAL_MAP_EMISSION].texture;
            if (emTex.id > 0) {
                mat->maps[MATERIAL_MAP_ALBEDO].texture = emTex;
                mat->maps[MATERIAL_MAP_ALBEDO].color   = WHITE;
                mat->maps[MATERIAL_MAP_EMISSION].texture = (Texture2D){0};
            }
        }

        // Shader reads normals from texture1 (MATERIAL_MAP_METALNESS slot = 1).
        if (normalTex.id > 0) mat->maps[MATERIAL_MAP_METALNESS].texture = normalTex;

        mat->shader = lightingShader;
    }

    ch.hasNormalMap  = normalTex.id > 0;
    ch.hasMetalRough = false;

    ApplyTextureFilterToAllMaterialMaps(ch.model, MAIN_TEXTURE_FILTER_MODE);

    for (int i = 0; i < CHAR_ANIM_COUNT; i++) {
        int count = 0;
        ch.anims[i] = LoadModelAnimations(ANIM_PATHS[i], &count);
        if (count > 0 && ch.anims[i] != NULL) {
            ch.animFrameCounts[i] = ch.anims[i][0].frameCount;
            printf("Character anim '%s': %d frames\n", ANIM_NAMES[i], ch.animFrameCounts[i]);
            if (i == CHAR_ANIM_WALK || i == CHAR_ANIM_RUN)
                StripRootMotionXZ(&ch.anims[i][0]);
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
