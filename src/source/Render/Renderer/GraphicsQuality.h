#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Render::GraphicsQuality
{

inline constexpr std::array<int, 4> SupportedMsaaSamples{8, 4, 2, 1};

[[nodiscard]] constexpr int NormalizeMsaaSamples(int requested)
{
    if (requested < 2)
    {
        return 1;
    }
    if (requested < 4)
    {
        return 2;
    }
    if (requested < 8)
    {
        return 4;
    }
    return 8;
}

[[nodiscard]] constexpr int NormalizeAnisotropy(int requested)
{
    if (requested < 2)
    {
        return 1;
    }
    if (requested < 4)
    {
        return 2;
    }
    if (requested < 8)
    {
        return 4;
    }
    if (requested < 16)
    {
        return 8;
    }
    return 16;
}

[[nodiscard]] constexpr std::uint32_t CalculateMipLevelCount(std::uint32_t width, std::uint32_t height)
{
    std::uint32_t dimension = std::max(width, height);
    std::uint32_t levels = 0;
    while (dimension > 0)
    {
        ++levels;
        dimension >>= 1u;
    }
    return std::max(levels, 1u);
}

} // namespace Render::GraphicsQuality
