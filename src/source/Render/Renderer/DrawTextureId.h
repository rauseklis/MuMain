#pragma once

#include <cstdint>

namespace mu
{
// Texture a draw command samples. An explicit id always wins; only id 0 falls back to the renderer's
// BindTexture state (or the white texture when texturing is disabled).
[[nodiscard]] constexpr std::uint32_t ResolveDrawTextureId(std::uint32_t explicitId, bool texture2DEnabled,
                                                           int boundTextureId)
{
    if (explicitId != 0u)
        return explicitId;
    if (!texture2DEnabled)
        return 0u;
    return static_cast<std::uint32_t>(boundTextureId);
}
}
