#pragma once

#include <array>
#include <cstddef>

namespace Render::Models
{
inline constexpr std::array<int, 6> BmdPolygonTriangleCorners{0, 1, 2, 0, 2, 3};

[[nodiscard]] constexpr std::size_t GetBmdTriangleCornerCount(int polygonCornerCount)
{
    if (polygonCornerCount == 3)
        return 3;
    if (polygonCornerCount == 4)
        return 6;
    return 0;
}
}
