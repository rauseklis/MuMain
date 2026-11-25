#pragma once

#include <cmath>

// Geometry of the compact top-left bar (map name above, coordinates and three buttons below).
//
// Everything is in logical pixels of the 640 x 480 interface space and matches the art made by
// tools/top_left_bar/make_top_left_bar.py. The frame is drawn from, and the mouse is tested against, the same
// rectangles, so what is drawn is what is clickable.
namespace UI::HUD::TopLeftBar
{
struct Rect
{
    int x;
    int y;
    int width;
    int height;
};

// A rectangle in logical pixels whose edges land on whole screen pixels.
struct SnappedRect
{
    float x;
    float y;
    float width;
    float height;
};

// The bar is the original three pieces, scaled down: a left end, a middle that is stretched, and a right end with the
// three button slots. The left end and the middle are taller than the right end, as in the original.
inline constexpr int kLeftWidth = 18;
inline constexpr int kMiddleSourceWidth = 22;
inline constexpr int kMiddleWidth = 76;
inline constexpr int kRightWidth = 74;
inline constexpr int kFrameHeight = 20;
inline constexpr int kRightHeight = 16;
inline constexpr int kFrameWidth = kLeftWidth + kMiddleWidth + kRightWidth;

inline constexpr Rect kLeftPiece{0, 0, kLeftWidth, kFrameHeight};
inline constexpr Rect kMiddlePiece{kLeftWidth, 0, kMiddleWidth, kFrameHeight};
inline constexpr Rect kRightPiece{kLeftWidth + kMiddleWidth, 0, kRightWidth, kRightHeight};

// Where the map name and coordinates are written (centred in this box).
inline constexpr Rect kTextBox{10, 1, 88, 14};
inline constexpr int kTextInset = 2;

inline constexpr int kButtonCount = 3;
inline constexpr int kButtonWidth = 14;
inline constexpr int kButtonHeight = 10;
inline constexpr int kButtonFirstX = kLeftWidth + kMiddleWidth + 15;
inline constexpr int kButtonStep = 14;
inline constexpr int kButtonY = 1;

enum class Button
{
    Settings = 0,
    MuHelper = 1,
    ExperienceHunter = 2,
};

enum class ButtonState
{
    Up = 0,
    Over = 1,
    Down = 2,
};

// The atlas (one texture holds the three pieces and every button face).
inline constexpr int kAtlasSize = 128;
inline constexpr Rect kLeftSource{0, 0, kLeftWidth, kFrameHeight};
inline constexpr Rect kMiddleSource{24, 0, kMiddleSourceWidth, kFrameHeight};
inline constexpr Rect kRightSource{0, 26, kRightWidth, kRightHeight};
inline constexpr int kFaceStrideX = 16;
inline constexpr int kFaceStrideY = 12;
inline constexpr int kFaceOriginY = 48;

enum class Face
{
    Settings = 0,
    MuHelperStart = 1,
    MuHelperStop = 2,
    ExperienceOff = 3,
    ExperienceOn = 4,
};

[[nodiscard]] constexpr Rect ButtonRect(Button button)
{
    return {kButtonFirstX + static_cast<int>(button) * kButtonStep, kButtonY, kButtonWidth, kButtonHeight};
}

// Where a face sits in the atlas, in texels.
[[nodiscard]] constexpr Rect FaceRect(Face face, ButtonState state)
{
    return {static_cast<int>(state) * kFaceStrideX, kFaceOriginY + static_cast<int>(face) * kFaceStrideY, kButtonWidth,
            kButtonHeight};
}

[[nodiscard]] constexpr float AtlasUv(int texel)
{
    return static_cast<float>(texel) / static_cast<float>(kAtlasSize);
}

[[nodiscard]] constexpr Rect Offset(const Rect& rect, int dx, int dy)
{
    return {rect.x + dx, rect.y + dy, rect.width, rect.height};
}

[[nodiscard]] constexpr bool Contains(const Rect& rect, int x, int y)
{
    return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}

// A point (relative to the bar's top-left corner) over any drawn part of the bar.
[[nodiscard]] constexpr bool BarContains(int x, int y)
{
    return Contains(Rect{0, 0, kLeftWidth + kMiddleWidth, kFrameHeight}, x, y) || Contains(kRightPiece, x, y);
}

// The logical coordinate nearest to `logical` that maps to a whole screen pixel (screen = logical * rate + offset).
[[nodiscard]] inline float SnapEdge(float logical, float rate, float offset)
{
    return (std::round(logical * rate + offset) - offset) / rate;
}

// Both edges are snapped, so neighbouring rectangles share an edge exactly and the size is a whole number of pixels.
[[nodiscard]] inline SnappedRect SnapRect(const Rect& rect, float rateX, float rateY, float offsetX, float offsetY)
{
    const float left = SnapEdge(static_cast<float>(rect.x), rateX, offsetX);
    const float top = SnapEdge(static_cast<float>(rect.y), rateY, offsetY);
    const float right = SnapEdge(static_cast<float>(rect.x + rect.width), rateX, offsetX);
    const float bottom = SnapEdge(static_cast<float>(rect.y + rect.height), rateY, offsetY);
    return {left, top, right - left, bottom - top};
}
} // namespace UI::HUD::TopLeftBar
