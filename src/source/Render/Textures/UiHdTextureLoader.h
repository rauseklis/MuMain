#pragma once

#include <optional>
#include <string>

#include "Render/Textures/UiHdTextures.h"

namespace Render::UiHdTextures
{
void SetActiveSetting(Setting setting);
[[nodiscard]] Setting GetActiveSetting();

// Finds the best available HD replacement for an interface texture whose original shows contentWidth x contentHeight
// texels inside a paddedWidth x paddedHeight GPU texture. `resolvePath` maps a wide path to the file system path.
// Returns nothing when the setting is off, no file exists or the file is not an exact multiple of the original.
[[nodiscard]] std::optional<HdImage> LoadForTexture(const std::wstring& originalPath, int contentWidth,
                                                    int contentHeight, int paddedWidth, int paddedHeight,
                                                    bool opaquePadding, std::string (*resolvePath)(const std::wstring&));
} // namespace Render::UiHdTextures
