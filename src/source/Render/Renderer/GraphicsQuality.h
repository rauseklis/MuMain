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
    if (isNumberedRoot(L"object") || isNumberedRoot(L"world"))
    {
        return true;
    }

    // Data/Skill/ is mostly small, deliberately crisp nearest-filtered sprites
    // (see the test for Data/Skill/HellGate.tga), so it stays out of the
    // general enhanced-path rule above. The few skill textures below were
    // hand-recreated at higher resolution specifically to look smoother once
    // enlarged on their 3D model/ground decal; without this they still fall
    // back to the GL_NEAREST default used for skill models (see
    // CLoadData::OpenTexture's default Type), so the sharper source data
    // renders with no mipmaps and no linear filtering, i.e. still visibly
    // blocky at normal gameplay distance/zoom despite the asset fix:
    //  - MMn2: Soul Barrier's casting streak (230173db).
    //  - magic_a01/magic_a02: the Hell Fire ground rune and flame texture
    //    (MODEL_CIRCLE/MODEL_CIRCLE_LIGHT), also shared by Nova and a few
    //    boss mechanics.
    if (root == L"skill")
    {
        const std::size_t fileSlash = normalized.find_last_of(L'/');
        const std::wstring_view fileName =
            fileSlash == std::wstring::npos ? std::wstring_view(normalized) : std::wstring_view(normalized).substr(fileSlash + 1);
        return fileName.starts_with(L"mmn2.") || fileName.starts_with(L"magic_a01.") ||
               fileName.starts_with(L"magic_a02.");
    }

    return false;
}

} // namespace Render::GraphicsQuality
