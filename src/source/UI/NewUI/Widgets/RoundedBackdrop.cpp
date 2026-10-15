#include "stdafx.h"
#include "UI/NewUI/Widgets/RoundedBackdrop.h"

#include <algorithm>
#include <cmath>

#include "Render/Textures/ZzzOpenglUtil.h"

namespace UI::Frame::RoundedBackdrop
{
namespace
{
constexpr float kHalfTexel = 0.5f;

void DrawPart(GLuint image, const BITMAP_t& bitmap, float x, float y, float offsetX, float offsetY, float width,
              float height)
{
    if (width <= 0.0f || height <= 0.0f)
        return;

    const float u = (offsetX + kHalfTexel) / bitmap.Width;
    const float v = (offsetY + kHalfTexel) / bitmap.Height;
    RenderBitmap(image, x + offsetX, y + offsetY, width, height, u, v, width / bitmap.Width, height / bitmap.Height);
}

// How far in from the side a row at `rowCentre` (measured from the edge) starts on a corner of the given radius.
float CornerInset(float radius, float rowCentre)
{
    const float rise = radius - rowCentre;
    return radius - std::sqrt(std::max(radius * radius - rise * rise, 0.0f));
}
} // namespace

void Render(GLuint image, float x, float y, float width, float height)
{
    const BITMAP_t& bitmap = Bitmaps[image];
    const float radius = std::min({kCornerRadius, width * 0.5f, height * 0.5f});

    DrawPart(image, bitmap, x, y, 0.0f, radius, width, height - 2.0f * radius);

    const float slice = 1.0f / kSlicesPerPixel;
    const int slices = static_cast<int>(radius * kSlicesPerPixel);
    for (int index = 0; index < slices; ++index)
    {
        const float rowTop = index * slice;
        const float inset = CornerInset(radius, rowTop + slice * 0.5f);
        DrawPart(image, bitmap, x, y, inset, rowTop, width - 2.0f * inset, slice);
        DrawPart(image, bitmap, x, y, inset, height - rowTop - slice, width - 2.0f * inset, slice);
    }
}
} // namespace UI::Frame::RoundedBackdrop
