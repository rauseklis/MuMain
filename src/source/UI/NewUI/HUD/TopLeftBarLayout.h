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

inline constexpr int kMargin = 2;
inline constexpr int kFrameWidth = 104;
inline constexpr int kFrameHeight = 34;

inline constexpr Rect kNameRow{0, 3, kFrameWidth, 12};
inline constexpr Rect kCoordinateRow{4, 18, 46, 14};
// Text must stay clear of the bevel and the ornaments at both ends of the name row.
inline constexpr int kNameTextInset = 6;

inline constexpr int kButtonCount = 3;
inline constexpr int kButtonSize = 14;
inline constexpr int kButtonFirstX = 54;
inline constexpr int kButtonStep = 16;
inline constexpr int kButtonY = 18;

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

// The atlas (one texture holds the frame and every button face).
inline constexpr int kAtlasSize = 128;
inline constexpr int kFaceCell = 16;
inline constexpr int kFaceOriginY = 40;

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
    return {kButtonFirstX + static_cast<int>(button) * kButtonStep, kButtonY, kButtonSize, kButtonSize};
}

// Where a face sits in the atlas, in texels.
[[nodiscard]] constexpr Rect FaceRect(Face face, ButtonState state)
{
    return {static_cast<int>(state) * kFaceCell, kFaceOriginY + static_cast<int>(face) * kFaceCell, kButtonSize,
            kButtonSize};
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
