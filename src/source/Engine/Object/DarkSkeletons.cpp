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

// Flames on the body while alive: the fire of the burning arms of the Kalima Schriker (BITMAP_FIRE + 1, subtype 1)
// on FLAMES_PER_FRAME random bones every frame at the original 25 FPS.
constexpr int FLAMES_PER_FRAME = 3;
constexpr float FLAME_SCALE = 1.2f;
// Dying: sparks fly off DEATH_SPARKS_PER_FRAME random bones from the death until the body has faded out; while it
// fades, it also glitters away like a dying Kalima monster (BITMAP_BUBBLE, subtype 2, with its sound).
constexpr int DEATH_SPARKS_PER_FRAME = 2;
constexpr int GLITTER_REFERENCE_FRAMES = 3;
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

// The position of a random body bone, or false when the bones of the object aren't known yet.
bool RandomBonePosition(OBJECT* o, vec3_t position)
{
    BMD* b = &Models[o->Type];
    const int bones = std::min(BODY_BONES, static_cast<int>(b->NumBones));
    if (o->BoneTransform == nullptr || bones <= 0)
    {
        return false;
    }

    vec3_t offset;
    Vector(0.f, 0.f, 0.f, offset);
    b->TransformPosition(o->BoneTransform[rand() % bones], offset, position, true);
    return true;
}

void CreateSparkAt(vec3_t position)
{
    vec3_t angle;
    Vector(static_cast<float>(SPARK_PITCH_MIN + rand() % SPARK_PITCH_RANGE), 0.f,
           static_cast<float>(rand() % SPARK_YAW_RANGE), angle);

    vec3_t light;
    Vector(1.f, 1.f, 1.f, light);
    CreateJoint(BITMAP_JOINT_SPARK, position, position, angle);
    CreateParticle(BITMAP_SPARK, position, angle, light);
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
        CreateSpark(o);
    }
}

void CreateDarkSkeletonFlames(CHARACTER* c, OBJECT* o)
{
    if (!IsDarkSkeleton(c->MonsterIndex))
    {
        return;
    }

    vec3_t position;
    if (c->Dead <= 0)
    {
        vec3_t light;
        Vector(1.f, 0.9f, 0.9f, light);
        for (int i = 0; i < FLAMES_PER_FRAME; ++i)
        {
            if (RandomBonePosition(o, position))
            {
                CreateParticleFpsChecked(BITMAP_FIRE + 1, position, o->Angle, light, 1, FLAME_SCALE);
            }
        }

        return;
    }

    if (!rand_fps_check(1))
    {
        return;
    }

    for (int i = 0; i < DEATH_SPARKS_PER_FRAME; ++i)
    {
        if (RandomBonePosition(o, position))
        {
            CreateSparkAt(position);
        }
    }

    if (o->Alpha < 1.f && rand_fps_check(GLITTER_REFERENCE_FRAMES) && RandomBonePosition(o, position))
    {
        vec3_t light;
        Vector(1.f, 1.f, 1.f, light);
        CreateParticle(BITMAP_BUBBLE, position, o->Angle, light, 2);
    }
}
