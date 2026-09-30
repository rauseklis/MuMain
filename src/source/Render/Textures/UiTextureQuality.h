#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Render::UiTextureQuality
{
[[nodiscard]] inline bool IsInterfaceTexturePath(std::wstring_view path)
{
    std::wstring normalized(path);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](wchar_t character)
    {
        if (character == L'\\')
            return L'/';
        return static_cast<wchar_t>(std::towlower(character));
    });

    const std::size_t dataPosition = normalized.find(L"data/");
    if (dataPosition == std::wstring::npos)
        return false;

    const std::size_t rootStart = dataPosition + 5u;
    const std::size_t rootEnd = normalized.find(L'/', rootStart);
    const std::wstring_view root(normalized.data() + rootStart,
                                 rootEnd == std::wstring::npos ? normalized.size() - rootStart : rootEnd - rootStart);
    return root == L"interface";
}

inline void BleedTransparentRgb(std::span<std::uint8_t> rgba, int width, int height)
{
    constexpr int kComponents = 4;
    if (width <= 0 || height <= 0)
        return;

    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (rgba.size() < pixelCount * kComponents)
        return;

    const std::vector<std::uint8_t> source(rgba.begin(), rgba.begin() + pixelCount * kComponents);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const std::size_t destination = (static_cast<std::size_t>(y) * width + x) * kComponents;
            if (rgba[destination + 3u] != 0u)
                continue;

            std::size_t bestSource = destination;
            std::uint8_t bestAlpha = 0u;
            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                const int neighbourY = y + offsetY;
                if (neighbourY < 0 || neighbourY >= height)
                    continue;

                for (int offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int neighbourX = x + offsetX;
                    if (neighbourX < 0 || neighbourX >= width || (offsetX == 0 && offsetY == 0))
                        continue;

                    const std::size_t candidate =
                        (static_cast<std::size_t>(neighbourY) * width + neighbourX) * kComponents;
                    const std::uint8_t candidateAlpha = source[candidate + 3u];
                    if (candidateAlpha > bestAlpha)
                    {
                        bestSource = candidate;
                        bestAlpha = candidateAlpha;
                    }
                }
            }

            if (bestAlpha == 0u)
                continue;

            rgba[destination] = source[bestSource];
            rgba[destination + 1u] = source[bestSource + 1u];
            rgba[destination + 2u] = source[bestSource + 2u];
        }
    }
}
} // namespace Render::UiTextureQuality
