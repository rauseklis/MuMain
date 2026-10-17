#include <doctest.h>

#include "Render/Textures/BlockCompression.h"

#include <cstdint>
#include <vector>

using namespace Render::BlockCompression;

namespace
{
std::vector<std::uint8_t> Solid(int width, int height, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }
    return pixels;
}

int MaxError(const std::vector<std::uint8_t>& original, const std::vector<std::uint8_t>& packed, Format format, int width,
             int height, bool includeAlpha)
{
    int worst = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto texel = DecodeTexel(format, packed, width, x, y);
            for (int channel = 0; channel < (includeAlpha ? 4 : 3); ++channel)
                worst = std::max(worst, std::abs(texel[channel] - original[(static_cast<std::size_t>(y) * width + x) * 4 + channel]));
        }
    }
    return worst;
}
} // namespace

TEST_CASE("compressed sizes follow the block counts [render][block_compression]")
{
    CHECK(CompressedSize(Format::Bc1, 16, 16) == 16u * 8u);
    CHECK(CompressedSize(Format::Bc3, 16, 16) == 16u * 16u);
    CHECK(CompressedSize(Format::Bc1, 5, 5) == 4u * 8u);
    CHECK(CompressedSize(Format::Bc3, 4, 4) == 16u);
}

TEST_CASE("a solid colour survives BC1 and BC3 within the 5:6:5 rounding [render][block_compression]")
{
    const auto pixels = Solid(8, 8, 200, 100, 50, 255);
    for (const Format format : {Format::Bc1, Format::Bc3})
    {
        const auto packed = Compress(format, pixels, 8, 8);
        REQUIRE(packed.size() == CompressedSize(format, 8, 8));
        CHECK(MaxError(pixels, packed, format, 8, 8, false) <= 8);
    }
}

TEST_CASE("fully transparent and fully opaque alpha are exact in BC3 [render][block_compression]")
{
    std::vector<std::uint8_t> pixels = Solid(4, 4, 10, 20, 30, 255);
    for (int i = 0; i < 8; ++i)
        pixels[static_cast<std::size_t>(i) * 4 + 3] = 0;
    const auto packed = Compress(Format::Bc3, pixels, 4, 4);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            CHECK(DecodeTexel(Format::Bc3, packed, 4, x, y)[3] == pixels[(static_cast<std::size_t>(y) * 4 + x) * 4 + 3]);
}

TEST_CASE("a smooth gradient stays close to the original [render][block_compression]")
{
    std::vector<std::uint8_t> pixels(32 * 32 * 4);
    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 32; ++x)
        {
            const std::size_t offset = (static_cast<std::size_t>(y) * 32 + x) * 4;
            pixels[offset] = static_cast<std::uint8_t>(x * 8);
            pixels[offset + 1] = static_cast<std::uint8_t>(y * 8);
            pixels[offset + 2] = static_cast<std::uint8_t>((x + y) * 4);
            pixels[offset + 3] = 255;
        }
    }
    const auto packed = Compress(Format::Bc1, pixels, 32, 32);
    CHECK(MaxError(pixels, packed, Format::Bc1, 32, 32, false) <= 24);
}

TEST_CASE("a hard edge between two colours keeps both colours [render][block_compression]")
{
    std::vector<std::uint8_t> pixels(4 * 4 * 4);
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            const std::size_t offset = (static_cast<std::size_t>(y) * 4 + x) * 4;
            const bool bright = x >= 2;
            pixels[offset] = bright ? 240 : 16;
            pixels[offset + 1] = bright ? 232 : 24;
            pixels[offset + 2] = bright ? 224 : 32;
            pixels[offset + 3] = 255;
        }
    }
    const auto packed = Compress(Format::Bc1, pixels, 4, 4);
    CHECK(MaxError(pixels, packed, Format::Bc1, 4, 4, false) <= 12);
}

TEST_CASE("downsampling averages 2x2 texels [render][block_compression]")
{
    std::vector<std::uint8_t> pixels(2 * 2 * 4, 0);
    const std::uint8_t values[4] = {0, 100, 200, 100};
    for (int i = 0; i < 4; ++i)
        pixels[static_cast<std::size_t>(i) * 4] = values[i];
    int width = 0;
    int height = 0;
    const auto half = Downsample(pixels, 2, 2, width, height);
    CHECK(width == 1);
    CHECK(height == 1);
    CHECK(half[0] == 100);
}

TEST_CASE("the mip chain stops at a whole block [render][block_compression]")
{
    const auto pixels = Solid(64, 32, 1, 2, 3, 255);
    const auto chain = CompressChain(Format::Bc1, pixels, 64, 32, true);
    REQUIRE(chain.size() == 4u); // 64x32, 32x16, 16x8, 8x4
    CHECK(chain.front().width == 64);
    CHECK(chain.back().width == 8);
    CHECK(chain.back().height == 4);
    CHECK(chain.back().data.size() == CompressedSize(Format::Bc1, 8, 4));
    CHECK(CompressChain(Format::Bc1, pixels, 64, 32, false).size() == 1u);
}
