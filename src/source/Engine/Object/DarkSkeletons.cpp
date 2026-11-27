#include "stdafx.h"
#include "Engine/Object/DarkSkeletons.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Textures/ZzzOpenglUtil.h"

namespace
{
struct DarkSkeletonLook
{
    EMonsterType monster;
    int weapon; // right hand; the left hand stays empty
};

constexpr DarkSkeletonLook DARK_SKELETON_LOOKS[] = {
    {MONSTER_DARK_SKELETON, MODEL_GORGON_STAFF},
    {MONSTER_DARK_REAPER, MODEL_RUNE_BLADE},
};

// The Death Bone skeleton (monster 56) is drawn at 0.8; the dark skeletons are 20% bigger. The weapon is attached to
// the hand, so it grows with the body.
constexpr float DEATH_BONE_SCALE = 0.8f;
constexpr float DARK_SKELETON_SIZE_FACTOR = 1.2f;

// The light of the terrain is multiplied by this: the bones look nearly black but stay visible at night.
constexpr float DARK_SKELETON_LIGHT_FACTOR = 0.25f;

// Sparks while walking: a new burst about every SPARK_REFERENCE_FRAMES frames at the original 25 FPS (the same rate at
// any FPS), SPARKS_PER_BURST sparks at a random spot within SPARK_SPREAD units around the feet.
constexpr int SPARK_REFERENCE_FRAMES = 2;
constexpr int SPARKS_PER_BURST = 2;
constexpr int SPARK_SPREAD = 40;
constexpr float SPARK_HEIGHT = 5.f;
// The upward angle of a spark: like the sparks of the Lorencia blacksmith (90 + 60..119 degrees, a little sideways).
constexpr int SPARK_PITCH_MIN = 150;
constexpr int SPARK_PITCH_RANGE = 60;
constexpr int SPARK_YAW_RANGE = 360;

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

void CreateSpark(const OBJECT* o)
{
    vec3_t position;
    position[0] = o->Position[0] + static_cast<float>(rand() % (SPARK_SPREAD * 2) - SPARK_SPREAD);
    position[1] = o->Position[1] + static_cast<float>(rand() % (SPARK_SPREAD * 2) - SPARK_SPREAD);
    position[2] = o->Position[2] + SPARK_HEIGHT;

    vec3_t angle;
    Vector(static_cast<float>(SPARK_PITCH_MIN + rand() % SPARK_PITCH_RANGE), 0.f,
           static_cast<float>(rand() % SPARK_YAW_RANGE), angle);

    vec3_t light;
    Vector(1.f, 1.f, 1.f, light);
    CreateJoint(BITMAP_JOINT_SPARK, position, position, angle);
    CreateParticle(BITMAP_SPARK, position, angle, light);
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
    c->Object.Scale = DEATH_BONE_SCALE * DARK_SKELETON_SIZE_FACTOR;
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

void CreateDarkSkeletonFootSparks(CHARACTER* c, OBJECT* o)
{
    if (!IsDarkSkeleton(c->MonsterIndex) || c->Dead > 0 || !c->Movement)
    {
        return;
    }

    if (!rand_fps_check(SPARK_REFERENCE_FRAMES))
    {
        return;
    }

    for (int i = 0; i < SPARKS_PER_BURST; ++i)
    {
        CreateSpark(o);
    }
}
