#pragma once

#include "Render/Sprites/GlobalBitmap.h"

// Draws a window's stone backdrop with round corners. The texture is mapped 1:1 from its top-left corner, exactly as a
// plain RenderImage would draw it; only the pixels outside the four corner arcs are left out, so the world shows
// through them.
namespace UI::Frame::RoundedBackdrop
{
inline constexpr float kCornerRadius = 6.0f;
inline constexpr int kSlicesPerPixel = 4;

void Render(GLuint image, float x, float y, float width, float height);
} // namespace UI::Frame::RoundedBackdrop
