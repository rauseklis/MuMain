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
inline constexpr std::wstring_view kInterfaceRoot = L"interface";
inline constexpr std::wstring_view kDataFolder = L"data/";

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

// Data\Interface\a\b.tga -> Data\Interface@4x\a\b.png (same separators and case). Empty for non-interface paths.
[[nodiscard]] inline std::wstring HdPath(std::wstring_view originalPath, int scale)
{
    if (!UiTextureQuality::IsInterfaceTexturePath(originalPath))
        return {};

    std::wstring lower(originalPath);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t character)
    {
        return character == L'\\' ? L'/' : static_cast<wchar_t>(std::towlower(character));
    });
    const std::size_t rootStart = lower.find(kDataFolder) + kDataFolder.size();

    std::wstring result(originalPath);
    result.replace(rootStart, kInterfaceRoot.size(), L"Interface@" + std::to_wstring(scale) + L"x");

    const std::size_t lastSeparator = result.find_last_of(L"\\/");
    const std::size_t dot = result.find_last_of(L'.');
    if (dot == std::wstring::npos || (lastSeparator != std::wstring::npos && dot < lastSeparator))
        return result + L".png";
    result.replace(dot, std::wstring::npos, L".png");
    return result;
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
