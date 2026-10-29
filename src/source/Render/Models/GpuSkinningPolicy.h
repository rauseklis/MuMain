#pragma once

#include <cstdint>

namespace Render::Models
{
// Switch to keep plain-textured meshes off the GPU skinning path (they are skinned on the CPU instead). It was
// the first mitigation tried for the "yellow folded planes" bug; the real cause was the Zen coin heap reading
// unskinned vertices (BMD::AddToCoinHeap). With the switch on, equipping Fenrir froze the game (the GPU stopped
// finishing frames), so it is off. Kept for comparing a model against the CPU path.
inline constexpr bool CpuSkinPlainTexturedMeshes = false;

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
