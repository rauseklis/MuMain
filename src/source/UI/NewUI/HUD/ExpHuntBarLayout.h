#pragma once

#include "UI/NewUI/HUD/TopLeftBarLayout.h"

#include <algorithm>

// Geometry of the Experience Hunter bar: the original three pieces (a left end, a stretched middle and the same end
// mirrored), scaled down. Everything is in logical pixels of the 640 x 480 interface space and matches the art made by
// tools/exp_hunt_bar/make_exp_hunt_bar.py.
namespace UI::HUD::ExpHuntBar
{
using Rect = TopLeftBar::Rect;

inline constexpr int kCapWidth = 16;
inline constexpr int kHeight = 18;
// The part of the picture that is drawn (the bottom rows of the art are transparent); this is what the mouse hits.
inline constexpr int kHitHeight = 13;
// Free space between an end and the text.
inline constexpr int kTextPadding = 4;
// The narrowest bar, whatever the text.
inline constexpr int kMinWidth = 96;
// Vertical centre of the dark band the text sits in.
inline constexpr int kTextCentreY = 6;

// The atlas: 64 x 64, every piece has two pixels of repeated edge around it.
inline constexpr int kAtlasSize = 64;
inline constexpr Rect kLeftSource{2, 2, kCapWidth, kHeight};
inline constexpr Rect kRightSource{22, 2, kCapWidth, kHeight};
inline constexpr Rect kMiddleSource{42, 2, kCapWidth, kHeight};

// Width of the bar that holds a line of text `textWidth` wide.
[[nodiscard]] constexpr int BarWidth(int textWidth)
{
    return std::max(kMinWidth, textWidth + 2 * (kCapWidth + kTextPadding));
}

[[nodiscard]] constexpr Rect LeftPiece()
{
    return {0, 0, kCapWidth, kHeight};
}

[[nodiscard]] constexpr Rect MiddlePiece(int barWidth)
{
    return {kCapWidth, 0, barWidth - 2 * kCapWidth, kHeight};
}

[[nodiscard]] constexpr Rect RightPiece(int barWidth)
{
    return {barWidth - kCapWidth, 0, kCapWidth, kHeight};
}

// Where the text is centred: between the two ends, with the padding left free on both sides.
[[nodiscard]] constexpr Rect TextBox(int barWidth)
{
    return {kCapWidth + kTextPadding, 0, barWidth - 2 * (kCapWidth + kTextPadding), kHeight};
}

[[nodiscard]] constexpr Rect HitBox(int barWidth)
{
    return {0, 0, barWidth, kHitHeight};
}

[[nodiscard]] constexpr float AtlasUv(int texel)
{
    return static_cast<float>(texel) / static_cast<float>(kAtlasSize);
}
} // namespace UI::HUD::ExpHuntBar
