#include "stdafx.h"
#include "Engine/Object/DarkSkeletons.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"

namespace
{
struct DarkSkeletonLook
{
    EMonsterType monster;
    int weapon; // right hand; the left hand stays empty
    float sizeFactor; // the size compared to the Death Bone skeleton
    bool sparks;      // sparks under the feet while walking
};

// The Death Bone skeleton (monster 56) is drawn at 0.8. Dark Skeleton and Dark Reaper are 20% bigger, Dark Mage 10%.
// The weapon is attached to the hand, so it grows with the body.
constexpr DarkSkeletonLook DARK_SKELETON_LOOKS[] = {
    {MONSTER_DARK_SKELETON, MODEL_GORGON_STAFF, 1.2f, true},
    {MONSTER_DARK_REAPER, MODEL_RUNE_BLADE, 1.2f, true},
    {MONSTER_DARK_MAGE, MODEL_SKULL_STAFF, 1.1f, true},
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

// Dying: like the Kalima monsters (RenderDeathEmbers), the body bursts into ember sparks once it has faded to half:
// DEATH_EMBERS_PER_FRAME sparks per frame at the original 25 FPS, each at a random body bone moved up to
// DEATH_EMBER_SPREAD units, so they come out of the whole body.
constexpr float DEATH_EMBER_ALPHA = 0.5f;
constexpr int DEATH_EMBERS_PER_FRAME = 12;
constexpr int DEATH_EMBER_SPREAD = 20;
// The bones of the body of the player model (the ones above belong to wings and weapons), as the Nova death uses.
constexpr int BODY_BONES = 32;

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

void CreateDarkSkeletonDeathEmbers(CHARACTER* c, OBJECT* o)
{
    if (!IsDarkSkeleton(c->MonsterIndex) || c->Dead <= 0 || o->Alpha >= DEATH_EMBER_ALPHA)
    {
        return;
    }

    vec3_t angle, light, position;
    Vector(0.f, 0.f, 0.f, angle);
    SetEmberLight(light);
    for (int i = 0; i < DEATH_EMBERS_PER_FRAME; ++i)
    {
        if (RandomBodyBonePosition(o, position))
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                position[axis] += static_cast<float>(rand() % (DEATH_EMBER_SPREAD * 2) - DEATH_EMBER_SPREAD);
            }

            CreateParticleFpsChecked(BITMAP_SPARK, position, angle, light, 0);
        }
    }
}
