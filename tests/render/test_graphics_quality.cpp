#include <doctest.h>

#include "Render/Renderer/GraphicsQuality.h"

TEST_CASE("MSAA requests normalize to supported quality levels [render][graphics_quality]")
{
    using Render::GraphicsQuality::NormalizeMsaaSamples;

    CHECK(NormalizeMsaaSamples(-1) == 1);
    CHECK(NormalizeMsaaSamples(0) == 1);
    CHECK(NormalizeMsaaSamples(1) == 1);
    CHECK(NormalizeMsaaSamples(2) == 2);
    CHECK(NormalizeMsaaSamples(3) == 2);
    CHECK(NormalizeMsaaSamples(4) == 4);
    CHECK(NormalizeMsaaSamples(7) == 4);
    CHECK(NormalizeMsaaSamples(8) == 8);
    CHECK(NormalizeMsaaSamples(16) == 8);
}

TEST_CASE("anisotropy requests normalize to power-of-two quality levels [render][graphics_quality]")
{
    using Render::GraphicsQuality::NormalizeAnisotropy;

    CHECK(NormalizeAnisotropy(0) == 1);
    CHECK(NormalizeAnisotropy(2) == 2);
    CHECK(NormalizeAnisotropy(3) == 2);
    CHECK(NormalizeAnisotropy(4) == 4);
    CHECK(NormalizeAnisotropy(7) == 4);
    CHECK(NormalizeAnisotropy(8) == 8);
    CHECK(NormalizeAnisotropy(15) == 8);
    CHECK(NormalizeAnisotropy(16) == 16);
    CHECK(NormalizeAnisotropy(64) == 16);
}

TEST_CASE("mip count includes the base level and the one-by-one level [render][graphics_quality]")
{
    using Render::GraphicsQuality::CalculateMipLevelCount;

    CHECK(CalculateMipLevelCount(0, 0) == 1);
    CHECK(CalculateMipLevelCount(1, 1) == 1);
    CHECK(CalculateMipLevelCount(2, 1) == 2);
    CHECK(CalculateMipLevelCount(256, 128) == 9);
    CHECK(CalculateMipLevelCount(1024, 1024) == 11);
}
