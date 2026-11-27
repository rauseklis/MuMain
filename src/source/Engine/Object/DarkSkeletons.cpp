#include "stdafx.h"
#include "Engine/Object/DarkSkeletons.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Core/Time/FrameTiming.h"

#include <vector>

namespace
{
struct DarkSkeletonLook
{
    EMonsterType monster;
    int weapon; // right hand; the left hand stays empty
    float sizeFactor; // the size compared to the Death Bone skeleton
    bool sparks;      // sparks under the feet while walking
    // Flames on the body like the burning arms of the Kalima Schriker.
    bool flames;
};

// The Death Bone skeleton (monster 56) is drawn at 0.8. Dark Skeleton and Dark Reaper are 20% bigger, Dark Mage 10%.
// The weapon is attached to the hand, so it grows with the body.
constexpr DarkSkeletonLook DARK_SKELETON_LOOKS[] = {
    {MONSTER_DARK_SKELETON, MODEL_GORGON_STAFF, 1.2f, true, false},
    {MONSTER_DARK_REAPER, MODEL_RUNE_BLADE, 1.2f, true, true},
    {MONSTER_DARK_MAGE, MODEL_SKULL_STAFF, 1.1f, true, false},
};

constexpr float DEATH_BONE_SCALE = 0.8f;

// The light of the terrain is multiplied by this: the bones look nearly black but stay visible at night.
constexpr float DARK_SKELETON_LIGHT_FACTOR = 0.25f;

// The ember colour of the Kalima monsters (SetEmberLight in GMHellas.cpp, rauseklis/MuMain#182).
constexpr float EMBER_LIGHT_RED = 1.f;
constexpr float EMBER_LIGHT_GREEN = 0.45f;
constexpr float EMBER_LIGHT_BLUE = 0.12f;

// Ember sparks while walking, like the Kalima monsters (CreateEmberSpark): a new burst about every
// SPARK_REFERENCE_FRAMES frames at the original 25 FPS (the same rate at any FPS), SPARKS_PER_BURST sparks at a random
// spot within SPARK_SPREAD units around the feet.
constexpr int SPARK_REFERENCE_FRAMES = 2;
constexpr int SPARKS_PER_BURST = 2;
constexpr int SPARK_SPREAD = 50;
constexpr float SPARK_HEIGHT = 5.f;

// The glow in the head: two pulsing ember lights, like the glows of the Kalima monsters.
constexpr float HEAD_GLOW_SCALE = 1.2f;
constexpr float HEAD_GLOW_PULSE_SPEED = 0.002f;
constexpr float HEAD_GLOW_MIN = 0.5f;
constexpr float HEAD_GLOW_PULSE = 0.3f;

// The flames of the dark skeletons which have them: the fire of the burning arms of the Kalima Schriker
// (BITMAP_FIRE + 1, subtype 1) on FLAMES_PER_FRAME random body bones every frame at the original 25 FPS.
constexpr int FLAMES_PER_FRAME = 3;
constexpr float FLAME_SCALE = 1.2f;

// Dying: ember sparks come out of the whole body like on a dying Kalima monster (RenderDeathEmbers). A skeleton does
// not fade out: about 15 frames after its death it falls apart into bones (SetPlayerDie). Until then
// DEATH_EMBERS_PER_FRAME sparks per frame at the original 25 FPS, and DEATH_EMBER_BURST sparks at once when it falls
// apart, each at a random body bone moved up to DEATH_EMBER_SPREAD units.
constexpr int DEATH_EMBERS_PER_FRAME = 12;
constexpr int DEATH_EMBER_BURST = 80;
constexpr int DEATH_EMBER_SPREAD = 20;
// After falling apart, the place keeps glowing: AFTERGLOW_FRAMES frames (at the original 25 FPS) of ember sparks
// rising from where the body was, AFTERGLOW_EMBERS_PER_FRAME at first, fewer and fewer until it dies down. They come
// from a box AFTERGLOW_SPREAD units wide and AFTERGLOW_HEIGHT units high (at the size of the Death Bone skeleton).
constexpr float AFTERGLOW_FRAMES = 75.f;
constexpr int AFTERGLOW_EMBERS_PER_FRAME = 8;
constexpr int AFTERGLOW_SPREAD = 50;
constexpr int AFTERGLOW_HEIGHT = 180;

// The bones of the body of the player model (the ones above belong to wings and weapons), as the Nova death uses.
constexpr int BODY_BONES = 32;

struct Afterglow
{
    vec3_t position;
    float scale;
    float framesLeft;
};

std::vector<Afterglow> g_afterglows;

const DarkSkeletonLook* FindLook(int monsterIndex)
{
    for (const DarkSkeletonLook& look : DARK_SKELETON_LOOKS)
    {
        if (look.monster == monsterIndex)
        {
            return &look;
        }
    }

    return nullptr;
}

void SetEmberLight(vec3_t light, float brightness = 1.f)
{
    Vector(brightness * EMBER_LIGHT_RED, brightness * EMBER_LIGHT_GREEN, brightness * EMBER_LIGHT_BLUE, light);
}

// The position of a bone of the object, or false when its bones aren't known yet.
bool BonePosition(const OBJECT* o, int bone, vec3_t position)
{
    const BMD* b = &Models[o->Type];
    if (o->BoneTransform == nullptr || bone < 0 || bone >= b->NumBones)
    {
        return false;
    }

    // Not BMD::TransformPosition: that places the bone at the position and size of the character the model was last
    // drawn for, which can be another player.
    vec3_t offset, local;
    Vector(0.f, 0.f, 0.f, offset);
    VectorTransform(offset, o->BoneTransform[bone], local);
    VectorScale(local, o->Scale, local);
    VectorAdd(local, o->Position, position);
    return true;
}

bool RandomBodyBonePosition(const OBJECT* o, vec3_t position)
{
    const int bones = std::min(BODY_BONES, static_cast<int>(Models[o->Type].NumBones));
    return bones > 0 && BonePosition(o, rand() % bones, position);
}

// An ember spark at a random place of the body; false when the bones aren't known.
bool CreateBodyEmber(const OBJECT* o, bool fpsChecked)
{
    vec3_t position;
    if (!RandomBodyBonePosition(o, position))
    {
        return false;
    }

    for (int axis = 0; axis < 3; ++axis)
    {
        position[axis] += static_cast<float>(rand() % (DEATH_EMBER_SPREAD * 2) - DEATH_EMBER_SPREAD);
    }

    vec3_t angle, light;
    Vector(0.f, 0.f, 0.f, angle);
    SetEmberLight(light);
    if (fpsChecked)
    {
        CreateParticleFpsChecked(BITMAP_SPARK, position, angle, light, 0);
    }
    else
    {
        CreateParticle(BITMAP_SPARK, position, angle, light, 0);
    }

    return true;
}

void CreateEmberSpark(const OBJECT* o)
{
    vec3_t position;
    position[0] = o->Position[0] + static_cast<float>(rand() % (SPARK_SPREAD * 2) - SPARK_SPREAD);
    position[1] = o->Position[1] + static_cast<float>(rand() % (SPARK_SPREAD * 2) - SPARK_SPREAD);
    position[2] = o->Position[2] + SPARK_HEIGHT;

    vec3_t light, angle;
    SetEmberLight(light);
    VectorCopy(o->Angle, angle);
    CreateParticle(BITMAP_SPARK, position, angle, light, 0);
}
} // namespace

CHARACTER* CreateDarkSkeleton(EMonsterType type, int key, int positionX, int positionY)
{
    const DarkSkeletonLook* look = FindLook(type);
    if (look == nullptr)
    {
        return nullptr;
    }

    CHARACTER* c = CreateCharacter(key, MODEL_PLAYER, positionX, positionY);
    c->Object.SubType = MODEL_SKELETON1;
    c->Object.Scale = DEATH_BONE_SCALE * look->sizeFactor;
    c->Weapon[0].Type = look->weapon;
    c->Weapon[0].Level = 0;
    c->Weapon[1].Type = -1;
    c->Blood = true;
    return c;
}

bool IsDarkSkeleton(int monsterIndex)
{
    return FindLook(monsterIndex) != nullptr;
}

void DarkenDarkSkeletonLight(vec3_t light)
{
    VectorScale(light, DARK_SKELETON_LIGHT_FACTOR, light);
}

void RenderDarkSkeletonGlow(CHARACTER* c, OBJECT* o)
{
    if (!IsDarkSkeleton(c->MonsterIndex) || c->Dead > 0)
    {
        return;
    }

    vec3_t position;
    if (!BonePosition(o, Models[o->Type].BoneHead, position))
    {
        return;
    }

    const float luminosity = sinf(WorldTime * HEAD_GLOW_PULSE_SPEED) * HEAD_GLOW_PULSE + HEAD_GLOW_MIN;
    vec3_t light;
    SetEmberLight(light, luminosity);
    CreateSprite(BITMAP_LIGHT, position, HEAD_GLOW_SCALE, light, o, WorldTime);
    CreateSprite(BITMAP_LIGHT, position, HEAD_GLOW_SCALE, light, o, WorldTime);
}

void CreateDarkSkeletonFootSparks(CHARACTER* c, OBJECT* o)
{
    const DarkSkeletonLook* look = FindLook(c->MonsterIndex);
    if (look == nullptr || !look->sparks || c->Dead > 0 || !c->Movement)
    {
        return;
    }

    if (!rand_fps_check(SPARK_REFERENCE_FRAMES))
    {
        return;
    }

    for (int i = 0; i < SPARKS_PER_BURST; ++i)
    {
        CreateEmberSpark(o);
    }
}

void CreateDarkSkeletonFlames(CHARACTER* c, OBJECT* o)
{
    const DarkSkeletonLook* look = FindLook(c->MonsterIndex);
    if (look == nullptr || !look->flames || c->Dead > 0)
    {
        return;
    }

    vec3_t light, position;
    Vector(1.f, 0.9f, 0.9f, light);
    for (int i = 0; i < FLAMES_PER_FRAME; ++i)
    {
        if (RandomBodyBonePosition(o, position))
        {
            CreateParticleFpsChecked(BITMAP_FIRE + 1, position, o->Angle, light, 1, FLAME_SCALE);
        }
    }
}

void CreateDarkSkeletonDeathEmbers(CHARACTER* c, OBJECT* o)
{
    if (!IsDarkSkeleton(c->MonsterIndex) || c->Dead <= 0)
    {
        return;
    }

    for (int i = 0; i < DEATH_EMBERS_PER_FRAME; ++i)
    {
        CreateBodyEmber(o, true);
    }
}

void CreateDarkSkeletonFallApartEmbers(CHARACTER* c)
{
    if (!IsDarkSkeleton(c->MonsterIndex))
    {
        return;
    }

    for (int i = 0; i < DEATH_EMBER_BURST; ++i)
    {
        CreateBodyEmber(&c->Object, false);
    }

    Afterglow afterglow{};
    VectorCopy(c->Object.Position, afterglow.position);
    afterglow.scale = c->Object.Scale / DEATH_BONE_SCALE;
    afterglow.framesLeft = AFTERGLOW_FRAMES;
    g_afterglows.push_back(afterglow);
}

void MoveDarkSkeletonAfterglows()
{
    vec3_t angle, light, position;
    Vector(0.f, 0.f, 0.f, angle);
    SetEmberLight(light);
    for (Afterglow& afterglow : g_afterglows)
    {
        // Fewer and fewer sparks while it dies down.
        const int embers = static_cast<int>(AFTERGLOW_EMBERS_PER_FRAME * afterglow.framesLeft / AFTERGLOW_FRAMES) + 1;
        const float spread = AFTERGLOW_SPREAD * afterglow.scale;
        for (int i = 0; i < embers; ++i)
        {
            position[0] = afterglow.position[0] + (static_cast<float>(rand() % 200) / 100.f - 1.f) * spread;
            position[1] = afterglow.position[1] + (static_cast<float>(rand() % 200) / 100.f - 1.f) * spread;
            position[2] = afterglow.position[2] + static_cast<float>(rand() % AFTERGLOW_HEIGHT) * afterglow.scale;
            CreateParticleFpsChecked(BITMAP_SPARK, position, angle, light, 0);
        }

        afterglow.framesLeft -= FPS_ANIMATION_FACTOR;
    }

    std::erase_if(g_afterglows, [](const Afterglow& afterglow) { return afterglow.framesLeft <= 0.f; });
}
