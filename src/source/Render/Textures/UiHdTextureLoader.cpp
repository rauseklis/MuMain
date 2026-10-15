#include "stdafx.h"
#include "Render/Textures/UiHdTextureLoader.h"

#include <SDL3/SDL_surface.h>

#include <atomic>
#include <cstring>
#include <memory>

#include "Core/Utilities/Log/MuLogger.h"

namespace Render::UiHdTextures
{
namespace
{
std::atomic<Setting> g_activeSetting{Setting::Auto};

struct SurfaceDeleter
{
    void operator()(SDL_Surface* surface) const
    {
        SDL_DestroySurface(surface);
    }
};
using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

SurfacePtr DecodePng(const std::string& path)
{
    SurfacePtr decoded(SDL_LoadPNG(path.c_str()));
    if (!decoded)
        return nullptr;
    return SurfacePtr(SDL_ConvertSurface(decoded.get(), SDL_PIXELFORMAT_RGBA32));
}

std::vector<std::uint8_t> CopyPixels(const SDL_Surface& surface)
{
    const std::size_t rowBytes = static_cast<std::size_t>(surface.w) * kBytesPerPixel;
    std::vector<std::uint8_t> pixels(rowBytes * surface.h);
    const auto* source = static_cast<const std::uint8_t*>(surface.pixels);
    for (int row = 0; row < surface.h; ++row)
        std::memcpy(&pixels[row * rowBytes], source + static_cast<std::size_t>(row) * surface.pitch, rowBytes);
    return pixels;
}

std::optional<HdImage> LoadAtScale(const std::wstring& originalPath, int scale, int contentWidth, int contentHeight,
                                   int paddedWidth, int paddedHeight, bool opaquePadding,
                                   std::string (*resolvePath)(const std::wstring&))
{
    const std::wstring hdPath = HdPath(originalPath, scale);
    SurfacePtr surface = DecodePng(resolvePath(hdPath));
    if (!surface)
        return std::nullopt;

    const auto detected = ComputeHdScale(contentWidth, contentHeight, surface->w, surface->h);
    const bool tooLarge = paddedWidth * scale > kMaxHdTextureSize || paddedHeight * scale > kMaxHdTextureSize;
    if (!detected || *detected != scale || tooLarge)
    {
        mu::log::Get("render")->warn("HD texture rejected (size {}x{} does not match {}x{} at {}x): {}", surface->w,
                                     surface->h, contentWidth, contentHeight, scale, mu_wchar_to_utf8(hdPath.c_str()));
        return std::nullopt;
    }

    HdImage image;
    image.scale = scale;
    image.width = paddedWidth * scale;
    image.height = paddedHeight * scale;
    mu::log::Get("render")->debug("HD texture {}x loaded: {}", scale, mu_wchar_to_utf8(hdPath.c_str()));
    image.rgba = PadToTexture(CopyPixels(*surface), surface->w, surface->h, image.width, image.height, opaquePadding);
    return image;
}
} // namespace

void SetActiveSetting(Setting setting)
{
    g_activeSetting.store(setting);
}

Setting GetActiveSetting()
{
    return g_activeSetting.load();
}

std::optional<HdImage> LoadForTexture(const std::wstring& originalPath, int contentWidth, int contentHeight,
                                      int paddedWidth, int paddedHeight, bool opaquePadding,
                                      std::string (*resolvePath)(const std::wstring&))
{
    if (!UiTextureQuality::IsInterfaceTexturePath(originalPath))
        return std::nullopt;

    for (const int scale : CandidateScales(GetActiveSetting()))
    {
        auto image = LoadAtScale(originalPath, scale, contentWidth, contentHeight, paddedWidth, paddedHeight,
                                 opaquePadding, resolvePath);
        if (image)
            return image;
    }
    return std::nullopt;
}
} // namespace Render::UiHdTextures
