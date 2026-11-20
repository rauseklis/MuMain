#pragma once

// The Invisible set: Helm, Armor, Pants, Gloves and Boots with item number 74 (groups 7 to 11). On the character a
// piece up to +6 is not drawn at all; from +7 the character's own body part is drawn in its place, with a golden
// glow. The pieces have the same defense and stat requirements at every item level.
namespace GameLogic::Items::InvisibleEquipment
{
inline constexpr int ItemNumber = 74;
inline constexpr int FirstGroup = 7; // Helm
inline constexpr int LastGroup = 11; // Boots
inline constexpr int FirstGoldenLevel = 7;
inline constexpr int FixedDefense = 200;
inline constexpr int FixedStatRequirement = 100;

// The golden glow of the body parts (the "gold" color of Data/Effects/GlowColors.json).
inline constexpr float GoldRed = 1.0f;
inline constexpr float GoldGreen = 0.7f;
inline constexpr float GoldBlue = 0.2f;

enum class Look
{
    NotInvisible, // any other item: drawn as usual
    Hidden,       // +0 to +6: nothing is drawn
    GoldenBody,   // +7 and up: the character's own body part with a golden glow
};

constexpr bool IsInvisiblePiece(int itemType, int itemsPerGroup)
{
    if (itemType < 0 || itemsPerGroup <= ItemNumber)
    {
        return false;
    }

    const int group = itemType / itemsPerGroup;
    return group >= FirstGroup && group <= LastGroup && itemType % itemsPerGroup == ItemNumber;
}

constexpr Look GetLook(int itemType, int itemsPerGroup, int level)
{
    if (!IsInvisiblePiece(itemType, itemsPerGroup))
    {
        return Look::NotInvisible;
    }

    return level >= FirstGoldenLevel ? Look::GoldenBody : Look::Hidden;
}

// The body part slot (helm = 0 ... boots = 4) a piece covers, counted from the helm.
constexpr int GetBodySlot(int itemType, int itemsPerGroup)
{
    return itemType / itemsPerGroup - FirstGroup;
}
} // namespace GameLogic::Items::InvisibleEquipment
