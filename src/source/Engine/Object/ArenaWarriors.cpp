#include "stdafx.h"
#include "Engine/Object/ArenaWarriors.h"
#include "Engine/Object/ZzzCharacter.h"

namespace
{
struct ArenaWarriorLook
{
    EMonsterType monster;
    int helm; // -1: no helmet, the head of the player model stays visible
    int armor;
    int pants;
    int gloves;
    int boots;
    int weaponRight;
    int weaponLeft; // shield, or the bow of an archer
    int itemLevel;  // 0..15, only changes the glow of the armour
    float scale;
};

// One row per monster: the armour set named in its name and the weapons the owner chose. A "2 X" weapon is one in
// each hand. Plate Warrior carries a placeholder weapon until the owner decides. Mace of the King is item 2:22.
constexpr ArenaWarriorLook ARENA_WARRIOR_LOOKS[] = {
    {MONSTER_ARENA_SCALE_WARRIOR, MODEL_SCALE_HELM, MODEL_SCALE_ARMOR, MODEL_SCALE_PANTS, MODEL_SCALE_GLOVES,
     MODEL_SCALE_BOOTS, MODEL_GREAT_SCYTHE, -1, 5, 1.0f},
    {MONSTER_ARENA_BRASS_WARRIOR, MODEL_BRASS_HELM, MODEL_BRASS_ARMOR, MODEL_BRASS_PANTS, MODEL_BRASS_GLOVES,
     MODEL_BRASS_BOOTS, MODEL_BLADE, MODEL_BLADE, 6, 1.0f},
    {MONSTER_ARENA_PLATE_WARRIOR, MODEL_PLATE_HELM, MODEL_PLATE_ARMOR, MODEL_PLATE_PANTS, MODEL_PLATE_GLOVES,
     MODEL_PLATE_BOOTS, MODEL_FLAIL, MODEL_FLAIL, 7, 1.05f},
    {MONSTER_ARENA_RED_DRAGON_WARRIOR, MODEL_DRAGON_HELM, MODEL_DRAGON_ARMOR, MODEL_DRAGON_PANTS, MODEL_DRAGON_GLOVES,
     MODEL_DRAGON_BOOTS, MODEL_DOUBLE_BLADE, MODEL_DOUBLE_BLADE, 9, 1.1f},
    {MONSTER_ARENA_BLACK_DRAGON_KNIGHT, MODEL_BLACK_DRAGON_HELM, MODEL_BLACK_DRAGON_ARMOR, MODEL_BLACK_DRAGON_PANTS,
     MODEL_BLACK_DRAGON_GLOVES, MODEL_BLACK_DRAGON_BOOTS, MODEL_KNIGHT_BLADE, MODEL_KNIGHT_BLADE, 9, 1.1f},
    {MONSTER_ARENA_PHOENIX_KNIGHT, MODEL_DARK_PHOENIX_HELM, MODEL_DARK_PHOENIX_ARMOR, MODEL_DARK_PHOENIX_PANTS,
     MODEL_DARK_PHOENIX_GLOVES, MODEL_DARK_PHOENIX_BOOTS, MODEL_LORD_SCEPTER, MODEL_LORD_SCEPTER, 11, 1.2f},
    {MONSTER_ARENA_GUARDIAN_ELF, MODEL_GUARDIAN_HELM, MODEL_GUARDIAN_ARMOR, MODEL_GUARDIAN_PANTS, MODEL_GUARDIAN_GLOVES,
     MODEL_GUARDIAN_BOOTS, MODEL_GREAT_REIGN_CROSSBOW, -1, 8, 1.0f},
    {MONSTER_ARENA_SPHINX_WIZZARD, -1, MODEL_SPHINX_ARMOR, MODEL_SPHINX_PANTS, MODEL_SPHINX_GLOVES, MODEL_SPHINX_BOOTS,
     MODEL_SERPENT_STAFF, -1, 7, 1.0f},
    {MONSTER_ARENA_SOUL_WIZZARD, MODEL_DARK_SOUL_HELM, MODEL_DARK_SOUL_ARMOR, MODEL_DARK_SOUL_PANTS,
     MODEL_DARK_SOUL_GLOVES, MODEL_DARK_SOUL_BOOTS, MODEL_DRAGON_SOUL_STAFF, -1, 9, 1.05f},
    {MONSTER_ARENA_GREAT_DRAGON_WARRIOR, MODEL_GREAT_DRAGON_HELM, MODEL_GREAT_DRAGON_ARMOR, MODEL_GREAT_DRAGON_PANTS,
     MODEL_GREAT_DRAGON_GLOVES, MODEL_GREAT_DRAGON_BOOTS, MODEL_MACE + 22, MODEL_MACE + 22, 11, 1.2f},
    {MONSTER_ARENA_DRAGON_KNIGHT_WARRIOR, MODEL_DRAGON_KNIGHT_HELM, MODEL_DRAGON_KNIGHT_ARMOR,
     MODEL_DRAGON_KNIGHT_PANTS, MODEL_DRAGON_KNIGHT_GLOVES, MODEL_DRAGON_KNIGHT_BOOTS, MODEL_KNIGHT_BLADE,
     MODEL_KNIGHT_BLADE, 13, 1.3f},
};

void WearPiece(CHARACTER* c, int part, int model, int itemLevel)
{
    if (model < 0)
    {
        return;
    }
    c->BodyPart[part].Type = model;
    c->BodyPart[part].Level = itemLevel;
}

void Dress(CHARACTER* c, const ArenaWarriorLook& look)
{
    WearPiece(c, BODYPART_HELM, look.helm, look.itemLevel);
    WearPiece(c, BODYPART_ARMOR, look.armor, look.itemLevel);
    WearPiece(c, BODYPART_PANTS, look.pants, look.itemLevel);
    WearPiece(c, BODYPART_GLOVES, look.gloves, look.itemLevel);
    WearPiece(c, BODYPART_BOOTS, look.boots, look.itemLevel);
    c->Weapon[0].Type = look.weaponRight;
    c->Weapon[0].Level = look.itemLevel;
    c->Weapon[1].Type = look.weaponLeft;
    c->Weapon[1].Level = look.itemLevel;
}
} // namespace

CHARACTER* CreateArenaWarrior(EMonsterType type, int key, int positionX, int positionY)
{
    for (const ArenaWarriorLook& look : ARENA_WARRIOR_LOOKS)
    {
        if (look.monster != type)
        {
            continue;
        }

        CHARACTER* c = CreateCharacter(key, MODEL_PLAYER, positionX, positionY);
        Dress(c, look);
        SetCharacterScale(c);
        c->Object.Scale *= look.scale;
        c->Blood = true;
        return c;
    }

    return nullptr;
}
