#include "stdafx.h"
#include "UI/NewUI/Widgets/NineSlice.h"
#include "UI/NewUI/NewUICommon.h"
#include "Render/Textures/ZzzOpenglUtil.h"

#include <algorithm>
#include <array>

namespace UI::NineSlice
{
namespace
{
// One column or row of the 3x3 grid: where it goes on screen and which texels feed it.
struct Span
{
    float destOffset = 0.f;
    float destSize = 0.f;
    float sourceOffset = 0.f;
    float sourceSize = 0.f;
};

constexpr int SpanCount = 3;
using Spans = std::array<Span, SpanCount>;

// Shrinks both caps together when the destination cannot hold them, so a small control degrades to two
// touching corners rather than corners drawn over each other.
void FitCaps(float destTotal, float& startCap, float& endCap)
{
    const float capTotal = startCap + endCap;
    if (capTotal <= destTotal || capTotal <= 0.f)
    {
        return;
    }
    const float shrink = destTotal / capTotal;
    startCap *= shrink;
    endCap *= shrink;
}

Spans BuildSpans(float destTotal, float sourceTotal, float startInset, float endInset, float texelsPerPixel)
{
    float startCap = startInset / texelsPerPixel;
    float endCap = endInset / texelsPerPixel;
    FitCaps(destTotal, startCap, endCap);

    Spans spans;
    spans[0] = {0.f, startCap, 0.f, startInset};
    spans[1] = {startCap, destTotal - startCap - endCap, startInset, sourceTotal - startInset - endInset};
    spans[2] = {destTotal - endCap, endCap, sourceTotal - endInset, endInset};
    return spans;
}
} // namespace

void Render(const Texture& texture, float x, float y, float width, float height, unsigned int color)
{
    if (width <= 0.f || height <= 0.f || texture.texelsPerPixel <= 0.f)
    {
        return;
    }

    // The textures carry their rounded outline in the alpha channel (the RGB is opaque all the way to the
    // corners to avoid fringing), so alpha must be honoured whatever blend state the caller left behind - a
    // preceding quad or text pass leaves it off, and the corners then show up as square steel-grey notches.
    EnableAlphaTest();

    const Spans columns =
        BuildSpans(width, texture.width, texture.insets.left, texture.insets.right, texture.texelsPerPixel);
    const Spans rows =
        BuildSpans(height, texture.height, texture.insets.top, texture.insets.bottom, texture.texelsPerPixel);

    for (const Span& row : rows)
    {
        for (const Span& column : columns)
        {
            if (column.destSize <= 0.f || row.destSize <= 0.f || column.sourceSize <= 0.f || row.sourceSize <= 0.f)
            {
                continue;
            }
            SEASON3B::RenderImageStretch(texture.imageIndex, x + column.destOffset, y + row.destOffset,
                column.destSize, row.destSize, column.sourceOffset, row.sourceOffset, column.sourceSize,
                row.sourceSize, color);
        }
    }
}
} // namespace UI::NineSlice
