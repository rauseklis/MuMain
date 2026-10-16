#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Render/Textures/UiTextureQuality.h"

// Optional high-resolution replacements for interface textures.
//
// A texture Data/Interface/<path>.<ext> may have a PNG at Data/Interface@<k>x/<path>.png that shows the same picture
// with k times as many texels. The bitmap keeps reporting its original (logical) size, so every draw call that
// derives UVs from that size keeps working unchanged.
namespace Render::UiHdTextures
{
inline constexpr int kMinScale = 2;
inline constexpr int kMaxScale = 4;
inline constexpr int kBytesPerPixel = 4;
inline constexpr int kMaxHdTextureSize = 8192;
inline constexpr std::wstring_view kDataFolder = L"data/";

// Texture folders that may have HD replacements: lower-case name -> folder name on disk.
struct EligibleRoot
{
    std::wstring_view lowerName;
    std::wstring_view folderName;
};
inline constexpr EligibleRoot kEligibleRoots[] = {
    {L"interface", L"Interface"},
    {L"skill", L"Skill"},
    {L"item", L"Item"},
    {L"player", L"Player"},
    {L"monster", L"Monster"},
    {L"npc", L"NPC"},
};

enum class Setting
{
    Off,
    Auto,
    Scale2,
    Scale4,
};

struct HdImage
{
    std::vector<std::uint8_t> rgba;
    int width = 0;
    int height = 0;
    int scale = 1;
};

// "off", "auto", "2" or "4" (case-insensitive); anything else falls back to Auto.
[[nodiscard]] inline Setting ParseSetting(std::wstring_view text)
{
    std::wstring lower(text);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    if (lower == L"off" || lower == L"0")
        return Setting::Off;
    if (lower == L"2" || lower == L"2x")
        return Setting::Scale2;
    if (lower == L"4" || lower == L"4x")
        return Setting::Scale4;
    return Setting::Auto;
}

// The value stored in config.ini for a setting (the inverse of ParseSetting).
[[nodiscard]] inline std::wstring SettingConfigValue(Setting setting)
{
    switch (setting)
    {
    case Setting::Off:
        return L"off";
    case Setting::Scale2:
        return L"2";
    case Setting::Scale4:
        return L"4";
    case Setting::Auto:
        break;
    }
    return L"auto";
}

// Scales to try, best first. A missing file at one scale falls back to the next lower one.
[[nodiscard]] inline std::vector<int> CandidateScales(Setting setting)
{
    switch (setting)
    {
    case Setting::Off:
        return {};
    case Setting::Scale2:
        return {2};
    case Setting::Scale4:
    case Setting::Auto:
        return {4, 2};
    }
    return {};
}

// The integer factor between an original and its HD file, or nothing when the HD file is not an exact multiple.
[[nodiscard]] inline std::optional<int> ComputeHdScale(int originalWidth, int originalHeight, int hdWidth,
                                                        int hdHeight)
{
    if (originalWidth <= 0 || originalHeight <= 0 || hdWidth <= 0 || hdHeight <= 0)
        return std::nullopt;
    if (hdWidth % originalWidth != 0 || hdHeight % originalHeight != 0)
        return std::nullopt;

    const int scaleX = hdWidth / originalWidth;
    const int scaleY = hdHeight / originalHeight;
    if (scaleX != scaleY || scaleX < kMinScale || scaleX > kMaxScale)
        return std::nullopt;
    return scaleX;
}

// World1, World2 ... and Object1, Object2 ...: a family prefix followed by digits only.
inline constexpr std::wstring_view kNumberedFamilies[] = {L"world", L"object"};

[[nodiscard]] inline bool IsNumberedFolder(std::wstring_view lowerName)
{
    for (const std::wstring_view family : kNumberedFamilies)
    {
        if (lowerName.size() <= family.size() || lowerName.substr(0, family.size()) != family)
            continue;
        const std::wstring_view digits = lowerName.substr(family.size());
        if (std::all_of(digits.begin(), digits.end(), [](wchar_t character) { return character >= L'0' && character <= L'9'; }))
            return true;
    }
    return false;
}

struct RootMatch
{
    std::size_t start = 0;
    std::size_t length = 0;
    std::wstring folderName;
};

// The first folder below "data/" when it is one of the folders that may have HD replacements.
[[nodiscard]] inline std::optional<RootMatch> FindEligibleRoot(std::wstring_view path)
{
    std::wstring lower(path);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t character)
    {
        return character == L'\\' ? L'/' : static_cast<wchar_t>(std::towlower(character));
    });
    const std::size_t dataPosition = lower.find(kDataFolder);
    if (dataPosition == std::wstring::npos)
        return std::nullopt;

    const std::size_t start = dataPosition + kDataFolder.size();
    const std::size_t end = lower.find(L'/', start);
    if (end == std::wstring::npos)
        return std::nullopt;

    const std::wstring_view name(lower.data() + start, end - start);
    for (const EligibleRoot& root : kEligibleRoots)
    {
        if (root.lowerName == name)
            return RootMatch{start, name.size(), std::wstring(root.folderName)};
    }
    if (IsNumberedFolder(name))
        return RootMatch{start, name.size(), std::wstring(path.substr(start, name.size()))};
    return std::nullopt;
}

[[nodiscard]] inline bool IsHdEligiblePath(std::wstring_view path)
{
    return FindEligibleRoot(path).has_value();
}

// Data\Interface\a\b.tga -> Data\Interface@4x\a\b.png (same separators); Skill and Item work the same way.
// Empty for textures outside the eligible folders.
[[nodiscard]] inline std::wstring HdPath(std::wstring_view originalPath, int scale)
{
    const std::optional<RootMatch> root = FindEligibleRoot(originalPath);
    if (!root)
        return {};

    std::wstring result(originalPath);
    result.replace(root->start, root->length, std::wstring(root->folderName) + L"@" + std::to_wstring(scale) + L"x");

    const std::size_t lastSeparator = result.find_last_of(L"\\/");
    const std::size_t dot = result.find_last_of(L'.');
    if (dot == std::wstring::npos || (lastSeparator != std::wstring::npos && dot < lastSeparator))
        return result + L".png";
    result.replace(dot, std::wstring::npos, L".png");
    return result;
}

// For two textures that share a name and differ only by extension (TileGrass01.jpg and TileGrass01.tga are different pictures),
// the HD file keeps the extension in its name: Data\World1\TileGrass01.jpg -> Data\World1@2x\TileGrass01.jpg.png.
[[nodiscard]] inline std::wstring HdPathWithExtension(std::wstring_view originalPath, int scale)
{
    const std::optional<RootMatch> root = FindEligibleRoot(originalPath);
    if (!root)
        return {};

    std::wstring result(originalPath);
    result.replace(root->start, root->length, std::wstring(root->folderName) + L"@" + std::to_wstring(scale) + L"x");

    const std::size_t lastSeparator = result.find_last_of(L"\/");
    const std::size_t dot = result.find_last_of(L'.');
    if (dot == std::wstring::npos || (lastSeparator != std::wstring::npos && dot < lastSeparator))
        return {};
    std::transform(result.begin() + static_cast<std::ptrdiff_t>(dot), result.end(), result.begin() + static_cast<std::ptrdiff_t>(dot),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    return result + L".png";
}

// Places a content-sized HD image at the top-left of a padded buffer. Padding is transparent black, or opaque black
// for originals that came from JPEG (which had no alpha).
[[nodiscard]] inline std::vector<std::uint8_t> PadToTexture(std::span<const std::uint8_t> rgba, int contentWidth,
                                                            int contentHeight, int paddedWidth, int paddedHeight,
                                                            bool opaquePadding)
{
    std::vector<std::uint8_t> padded(static_cast<std::size_t>(paddedWidth) * paddedHeight * kBytesPerPixel, 0u);
    if (opaquePadding)
    {
        for (std::size_t alpha = 3u; alpha < padded.size(); alpha += kBytesPerPixel)
            padded[alpha] = 0xFFu;
    }

    const std::size_t sourceRow = static_cast<std::size_t>(contentWidth) * kBytesPerPixel;
    const std::size_t destinationRow = static_cast<std::size_t>(paddedWidth) * kBytesPerPixel;
    const int rows = std::min(contentHeight, paddedHeight);
    for (int row = 0; row < rows; ++row)
    {
        std::copy_n(rgba.begin() + static_cast<std::ptrdiff_t>(row * sourceRow), std::min(sourceRow, destinationRow),
                    padded.begin() + static_cast<std::ptrdiff_t>(row * destinationRow));
    }
    return padded;
}
} // namespace Render::UiHdTextures
