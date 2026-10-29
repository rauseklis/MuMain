#pragma once

#include <cstdint>

namespace Render::Models
{
// Mitigation for the intermittent "yellow folded planes" rendering bug: it disappears when plain-textured
// meshes are skinned on the CPU, so they stay off the GPU skinning path. The GPU path itself is kept and
// still serves flat colour, bright and environment-mapped meshes. Set to false to compare against it.
inline constexpr bool CpuSkinPlainTexturedMeshes = true;

struct GpuSkinningRequest
{
    bool configEnabled;      // [Render] GpuSkinning
    bool hasBoneMatrices;
    bool supportedMaterial;
    bool excludedByFlags;    // shadow map / wave meshes always use the CPU path
    bool plainTextured;      // RENDER_TEXTURE with the mesh's own texture coordinates
};

[[nodiscard]] constexpr bool IsGpuSkinningAllowed(const GpuSkinningRequest& request,
                                                  bool cpuSkinPlainTextured = CpuSkinPlainTexturedMeshes)
{
    if (!request.configEnabled || !request.hasBoneMatrices || !request.supportedMaterial || request.excludedByFlags)
        return false;
    return !(cpuSkinPlainTextured && request.plainTextured);
}

// The texture a GPU-skinned command must sample. A command is replayed at the end of the frame, so it
// must carry its texture itself instead of resolving the renderer's mutable BindTexture state; only a
// plain textured mesh samples the mesh texture, flat colour and bright meshes use the white texture
// (0) and chrome meshes the texture bound for them.
[[nodiscard]] constexpr std::uint32_t SelectSkinnedTextureId(bool plainTexturedMaterial, int meshTextureIndex)
{
    return plainTexturedMaterial && meshTextureIndex > 0 ? static_cast<std::uint32_t>(meshTextureIndex) : 0u;
}
}
