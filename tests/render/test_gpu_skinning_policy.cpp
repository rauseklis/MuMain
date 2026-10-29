#include <doctest.h>

#include "Render/Models/GpuSkinningPolicy.h"
#include "Render/Renderer/DrawTextureId.h"

namespace
{
constexpr Render::Models::GpuSkinningRequest MakeRequest(bool plainTextured)
{
    return {.configEnabled = true,
            .hasBoneMatrices = true,
            .supportedMaterial = true,
            .excludedByFlags = false,
            .plainTextured = plainTextured};
}
}

TEST_CASE("plain textured meshes select CPU skinning while the mitigation is enabled [render][gpu_skinning_policy]")
{
    static_assert(Render::Models::CpuSkinPlainTexturedMeshes, "the yellow-shape mitigation must stay on by default");
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(MakeRequest(true)));
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(MakeRequest(true), true));
}

TEST_CASE("other materials keep GPU skinning under the mitigation [render][gpu_skinning_policy]")
{
    CHECK(Render::Models::IsGpuSkinningAllowed(MakeRequest(false)));
}

TEST_CASE("the GPU path stays reachable for plain textured meshes with the mitigation off [render][gpu_skinning_policy]")
{
    CHECK(Render::Models::IsGpuSkinningAllowed(MakeRequest(true), false));
}

TEST_CASE("config switch, missing bones, unsupported material and excluded flags always select CPU skinning [render][gpu_skinning_policy]")
{
    auto request = MakeRequest(false);
    request.configEnabled = false;
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(request, false));
    request = MakeRequest(false);
    request.hasBoneMatrices = false;
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(request, false));
    request = MakeRequest(false);
    request.supportedMaterial = false;
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(request, false));
    request = MakeRequest(false);
    request.excludedByFlags = true;
    CHECK_FALSE(Render::Models::IsGpuSkinningAllowed(request, false));
}

TEST_CASE("a GPU-skinned command carries the mesh texture instead of the mutable bind state [render][gpu_skinning_policy]")
{
    constexpr int meshTexture = 42;
    constexpr int laterBoundTexture = 7;
    const std::uint32_t recorded = Render::Models::SelectSkinnedTextureId(true, meshTexture);
    CHECK(recorded == 42u);
    // Another BindTexture / DisableTexture after recording must not change what the command samples.
    CHECK(mu::ResolveDrawTextureId(recorded, true, laterBoundTexture) == 42u);
    CHECK(mu::ResolveDrawTextureId(recorded, false, laterBoundTexture) == 42u);
}

TEST_CASE("flat colour and bright meshes keep resolving through the bind state [render][gpu_skinning_policy]")
{
    CHECK(Render::Models::SelectSkinnedTextureId(false, 42) == 0u);
    CHECK(mu::ResolveDrawTextureId(0u, false, 7) == 0u);
    CHECK(mu::ResolveDrawTextureId(0u, true, 7) == 7u);
}
