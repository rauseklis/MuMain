#pragma once

#include <span>

#include "Core/Platform/WinCompat.h"
#include "Core/Globals/_enum.h"

struct ItemCreationParams
{
    int Group;
    int Number;
    BYTE Level;
    BYTE Durability;
    bool WithLuck;
    bool WithSkill;

    bool WithOption;
    BYTE OptionLevel;
    BYTE OptionType;

    bool HasExcellentOption;
    BYTE ExcellentFlags;

    bool IsAncient;
    BYTE AncientDiscriminator;
    BYTE AncientBonusOption;

    bool HasHarmonyOption;
    BYTE HarmonyOptionType;
    BYTE HarmonyOptionLevel;

    bool HasGuardianOption;
    BYTE SocketCount;
    BYTE SocketOptions[MAX_SOCKETS];
    BYTE SocketBonusOption;

    bool IsValid = false;

    bool WithExpiration;
    bool IsExpired;
};

ItemCreationParams ParseItemData(std::span<const BYTE> itemData);
