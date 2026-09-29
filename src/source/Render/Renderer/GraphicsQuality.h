#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cwctype>
#include <string>
#include <string_view>

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

[[nodiscard]] inline bool IsEnhancedTexturePath(std::wstring_view path)
{
    std::wstring normalized(path);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](wchar_t character)
    {
        if (character == L'\\')
        {
            return L'/';
        }
        return static_cast<wchar_t>(std::towlower(character));
    });

    const std::size_t dataPosition = normalized.find(L"data/");
    if (dataPosition == std::wstring::npos)
    {
        return false;
    }

    const std::size_t rootStart = dataPosition + 5u;
    const std::size_t rootEnd = normalized.find(L'/', rootStart);
    const std::size_t rootLength = rootEnd == std::wstring::npos ? normalized.size() - rootStart : rootEnd - rootStart;
    const std::wstring_view root(normalized.data() + rootStart, rootLength);
    if (root == L"player" || root == L"monster" || root == L"npc" || root == L"item" || root == L"items")
    {
        return true;
    }

    const auto isNumberedRoot = [root](std::wstring_view prefix)
    {
        if (!root.starts_with(prefix) || root.size() == prefix.size())
        {
            return false;
        }
        return std::all_of(root.begin() + static_cast<std::ptrdiff_t>(prefix.size()), root.end(),
                           [](wchar_t character) { return std::iswdigit(character) != 0; });
    };
    return isNumberedRoot(L"object") || isNumberedRoot(L"world");
}

} // namespace Render::GraphicsQuality
