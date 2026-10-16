#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

// CPU encoders for the GPU block formats BC1 (opaque colour, 8 bytes per 4x4 block) and BC3 (colour plus 8-level alpha,
// 16 bytes per block), plus the mip chain they need (compressed textures cannot be mipmapped on the GPU).
//
// Used for the large HD model and world textures: RGBA8 costs 4 bytes per pixel, BC3 one byte and BC1 half a byte, and a
// 4x enlargement that is block-compressed still holds far more detail than the original at full size.
namespace Render::BlockCompression
{
enum class Format
{
    Bc1,
    Bc3,
};

inline constexpr int kBlockSize = 4;
inline constexpr int kBytesPerPixelRgba = 4;

[[nodiscard]] inline std::size_t BytesPerBlock(Format format)
{
    return format == Format::Bc1 ? 8u : 16u;
}

[[nodiscard]] inline int BlocksAcross(int pixels)
{
    return (pixels + kBlockSize - 1) / kBlockSize;
}

[[nodiscard]] inline std::size_t CompressedSize(Format format, int width, int height)
{
    return static_cast<std::size_t>(BlocksAcross(width)) * static_cast<std::size_t>(BlocksAcross(height)) *
           BytesPerBlock(format);
}

namespace detail
{
struct Rgb
{
    int r, g, b;
};

[[nodiscard]] inline std::uint16_t Pack565(float r, float g, float b)
{
    const auto clampTo = [](float value, float maximum) { return std::clamp(value, 0.0f, maximum); };
    const int red = static_cast<int>(clampTo(r / 255.0f * 31.0f + 0.5f, 31.0f));
    const int green = static_cast<int>(clampTo(g / 255.0f * 63.0f + 0.5f, 63.0f));
    const int blue = static_cast<int>(clampTo(b / 255.0f * 31.0f + 0.5f, 31.0f));
    return static_cast<std::uint16_t>((red << 11) | (green << 5) | blue);
}

[[nodiscard]] inline Rgb Unpack565(std::uint16_t value)
{
    const int red = (value >> 11) & 31;
    const int green = (value >> 5) & 63;
    const int blue = value & 31;
    return {(red << 3) | (red >> 2), (green << 2) | (green >> 4), (blue << 3) | (blue >> 2)};
}

inline void WriteU16(std::uint8_t* destination, std::uint16_t value)
{
    destination[0] = static_cast<std::uint8_t>(value & 0xFFu);
    destination[1] = static_cast<std::uint8_t>(value >> 8);
}

// The 16 pixels of the block at (blockX, blockY); texels beyond the image repeat the edge.
inline void GatherBlock(std::span<const std::uint8_t> rgba, int width, int height, int blockX, int blockY,
                        std::array<std::array<std::uint8_t, 4>, 16>& pixels)
{
    for (int row = 0; row < kBlockSize; ++row)
    {
        const int y = std::min(blockY * kBlockSize + row, height - 1);
        for (int column = 0; column < kBlockSize; ++column)
        {
            const int x = std::min(blockX * kBlockSize + column, width - 1);
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * kBytesPerPixelRgba;
            for (int channel = 0; channel < 4; ++channel)
                pixels[row * kBlockSize + column][channel] = rgba[offset + channel];
        }
    }
}

// Encodes the colour of one block as two 5:6:5 endpoints and sixteen 2-bit indices (the BC1 colour block, also used
// inside BC3). The endpoints lie on the block's principal colour axis.
inline void EncodeColourBlock(const std::array<std::array<std::uint8_t, 4>, 16>& pixels, std::uint8_t* out)
{
    float mean[3] = {0, 0, 0};
    for (const auto& pixel : pixels)
        for (int channel = 0; channel < 3; ++channel)
            mean[channel] += pixel[channel];
    for (float& value : mean)
        value /= 16.0f;

    float covariance[3][3] = {};
    for (const auto& pixel : pixels)
    {
        const float delta[3] = {pixel[0] - mean[0], pixel[1] - mean[1], pixel[2] - mean[2]};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                covariance[i][j] += delta[i] * delta[j];
    }

    // Power iteration for the dominant axis; a flat block keeps the default and compresses exactly anyway.
    float axis[3] = {0.57735f, 0.57735f, 0.57735f};
    for (int iteration = 0; iteration < 6; ++iteration)
    {
        float next[3];
        for (int i = 0; i < 3; ++i)
            next[i] = covariance[i][0] * axis[0] + covariance[i][1] * axis[1] + covariance[i][2] * axis[2];
        const float length = std::sqrt(next[0] * next[0] + next[1] * next[1] + next[2] * next[2]);
        if (length < 1e-6f)
            break;
        for (int i = 0; i < 3; ++i)
            axis[i] = next[i] / length;
    }

    float low = 1e30f;
    float high = -1e30f;
    for (const auto& pixel : pixels)
    {
        const float t = (pixel[0] - mean[0]) * axis[0] + (pixel[1] - mean[1]) * axis[1] + (pixel[2] - mean[2]) * axis[2];
        low = std::min(low, t);
        high = std::max(high, t);
    }

    std::uint16_t endpoint0 = Pack565(mean[0] + axis[0] * high, mean[1] + axis[1] * high, mean[2] + axis[2] * high);
    std::uint16_t endpoint1 = Pack565(mean[0] + axis[0] * low, mean[1] + axis[1] * low, mean[2] + axis[2] * low);
    if (endpoint0 < endpoint1)
        std::swap(endpoint0, endpoint1);

    const Rgb colour0 = Unpack565(endpoint0);
    const Rgb colour1 = Unpack565(endpoint1);
    const Rgb palette[4] = {
        colour0,
        colour1,
        {(2 * colour0.r + colour1.r) / 3, (2 * colour0.g + colour1.g) / 3, (2 * colour0.b + colour1.b) / 3},
        {(colour0.r + 2 * colour1.r) / 3, (colour0.g + 2 * colour1.g) / 3, (colour0.b + 2 * colour1.b) / 3},
    };

    std::uint32_t indices = 0;
    for (int index = 0; index < 16; ++index)
    {
        int best = 0;
        int bestError = 1 << 30;
        for (int candidate = 0; candidate < 4; ++candidate)
        {
            const int dr = pixels[index][0] - palette[candidate].r;
            const int dg = pixels[index][1] - palette[candidate].g;
            const int db = pixels[index][2] - palette[candidate].b;
            const int error = dr * dr + dg * dg + db * db;
            if (error < bestError)
            {
                bestError = error;
                best = candidate;
            }
        }
        indices |= static_cast<std::uint32_t>(best) << (2 * index);
    }

    WriteU16(out, endpoint0);
    WriteU16(out + 2, endpoint1);
    for (int byte = 0; byte < 4; ++byte)
        out[4 + byte] = static_cast<std::uint8_t>((indices >> (8 * byte)) & 0xFFu);
}

// The BC3 alpha block: two 8-bit endpoints (alpha0 > alpha1 selects six interpolated levels between them) and sixteen
// 3-bit indices. Fully transparent and fully opaque texels are always represented exactly.
inline void EncodeAlphaBlock(const std::array<std::array<std::uint8_t, 4>, 16>& pixels, std::uint8_t* out)
{
    int low = 255;
    int high = 0;
    for (const auto& pixel : pixels)
    {
        low = std::min<int>(low, pixel[3]);
        high = std::max<int>(high, pixel[3]);
    }

    int palette[8];
    palette[0] = high;
    palette[1] = low;
    if (high > low)
    {
        for (int step = 1; step <= 6; ++step)
            palette[1 + step] = ((7 - step) * high + step * low) / 7;
    }
    else
    {
        for (int step = 2; step < 8; ++step)
            palette[step] = high;
    }

    std::uint64_t indices = 0;
    for (int index = 0; index < 16; ++index)
    {
        int best = 0;
        int bestError = 1 << 30;
        for (int candidate = 0; candidate < 8; ++candidate)
        {
            const int error = std::abs(static_cast<int>(pixels[index][3]) - palette[candidate]);
            if (error < bestError)
            {
                bestError = error;
                best = candidate;
            }
        }
        indices |= static_cast<std::uint64_t>(best) << (3 * index);
    }

    out[0] = static_cast<std::uint8_t>(high);
    out[1] = static_cast<std::uint8_t>(low);
    for (int byte = 0; byte < 6; ++byte)
        out[2 + byte] = static_cast<std::uint8_t>((indices >> (8 * byte)) & 0xFFu);
}
} // namespace detail

// Compresses a tightly packed RGBA8 image. The size does not have to be a multiple of four.
[[nodiscard]] inline std::vector<std::uint8_t> Compress(Format format, std::span<const std::uint8_t> rgba, int width,
                                                         int height)
{
    std::vector<std::uint8_t> output(CompressedSize(format, width, height));
    std::uint8_t* out = output.data();
    std::array<std::array<std::uint8_t, 4>, 16> pixels{};
    for (int blockY = 0; blockY < BlocksAcross(height); ++blockY)
    {
        for (int blockX = 0; blockX < BlocksAcross(width); ++blockX)
        {
            detail::GatherBlock(rgba, width, height, blockX, blockY, pixels);
            if (format == Format::Bc3)
            {
                detail::EncodeAlphaBlock(pixels, out);
                out += 8;
            }
            detail::EncodeColourBlock(pixels, out);
            out += 8;
        }
    }
    return output;
}

// Decodes one texel back to RGBA (used by the tests and the quality checks).
[[nodiscard]] inline std::array<std::uint8_t, 4> DecodeTexel(Format format, std::span<const std::uint8_t> data,
                                                              int width, int x, int y)
{
    const int blocksAcross = BlocksAcross(width);
    const std::uint8_t* block = data.data() + (static_cast<std::size_t>(y / kBlockSize) * blocksAcross + x / kBlockSize) *
                                                   BytesPerBlock(format);
    const int texel = (y % kBlockSize) * kBlockSize + (x % kBlockSize);

    std::uint8_t alpha = 255;
    if (format == Format::Bc3)
    {
        const int high = block[0];
        const int low = block[1];
        std::uint64_t indices = 0;
        for (int byte = 0; byte < 6; ++byte)
            indices |= static_cast<std::uint64_t>(block[2 + byte]) << (8 * byte);
        const int index = static_cast<int>((indices >> (3 * texel)) & 7u);
        int palette[8];
        palette[0] = high;
        palette[1] = low;
        if (high > low)
        {
            for (int step = 1; step <= 6; ++step)
                palette[1 + step] = ((7 - step) * high + step * low) / 7;
        }
        else
        {
            for (int step = 1; step <= 4; ++step)
                palette[1 + step] = ((5 - step) * high + step * low) / 5;
            palette[6] = 0;
            palette[7] = 255;
        }
        alpha = static_cast<std::uint8_t>(palette[index]);
        block += 8;
    }

    const std::uint16_t endpoint0 = static_cast<std::uint16_t>(block[0] | (block[1] << 8));
    const std::uint16_t endpoint1 = static_cast<std::uint16_t>(block[2] | (block[3] << 8));
    const detail::Rgb colour0 = detail::Unpack565(endpoint0);
    const detail::Rgb colour1 = detail::Unpack565(endpoint1);
    const std::uint32_t indices = static_cast<std::uint32_t>(block[4]) | (static_cast<std::uint32_t>(block[5]) << 8) |
                                  (static_cast<std::uint32_t>(block[6]) << 16) | (static_cast<std::uint32_t>(block[7]) << 24);
    const int index = static_cast<int>((indices >> (2 * texel)) & 3u);
    detail::Rgb colour{};
    switch (index)
    {
    case 0:
        colour = colour0;
        break;
    case 1:
        colour = colour1;
        break;
    case 2:
        colour = {(2 * colour0.r + colour1.r) / 3, (2 * colour0.g + colour1.g) / 3, (2 * colour0.b + colour1.b) / 3};
        break;
    default:
        colour = {(colour0.r + 2 * colour1.r) / 3, (colour0.g + 2 * colour1.g) / 3, (colour0.b + 2 * colour1.b) / 3};
        break;
    }
    return {static_cast<std::uint8_t>(colour.r), static_cast<std::uint8_t>(colour.g), static_cast<std::uint8_t>(colour.b),
            alpha};
}

// Halves an RGBA8 image with a 2x2 box filter (odd sizes keep the last row/column).
[[nodiscard]] inline std::vector<std::uint8_t> Downsample(std::span<const std::uint8_t> rgba, int width, int height,
                                                           int& outWidth, int& outHeight)
{
    outWidth = std::max(width / 2, 1);
    outHeight = std::max(height / 2, 1);
    std::vector<std::uint8_t> output(static_cast<std::size_t>(outWidth) * outHeight * kBytesPerPixelRgba);
    for (int y = 0; y < outHeight; ++y)
    {
        const int y0 = std::min(y * 2, height - 1);
        const int y1 = std::min(y * 2 + 1, height - 1);
        for (int x = 0; x < outWidth; ++x)
        {
            const int x0 = std::min(x * 2, width - 1);
            const int x1 = std::min(x * 2 + 1, width - 1);
            for (int channel = 0; channel < 4; ++channel)
            {
                const auto sample = [&](int sx, int sy)
                { return static_cast<int>(rgba[(static_cast<std::size_t>(sy) * width + sx) * kBytesPerPixelRgba + channel]); };
                output[(static_cast<std::size_t>(y) * outWidth + x) * kBytesPerPixelRgba + channel] =
                    static_cast<std::uint8_t>((sample(x0, y0) + sample(x1, y0) + sample(x0, y1) + sample(x1, y1) + 2) / 4);
            }
        }
    }
    return output;
}

struct Level
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> data;
};

// Level 0 and, when requested, every smaller level down to 4x4 (the smallest size that is still a whole block).
[[nodiscard]] inline std::vector<Level> CompressChain(Format format, std::span<const std::uint8_t> rgba, int width,
                                                       int height, bool withMips)
{
    std::vector<Level> levels;
    levels.push_back({width, height, Compress(format, rgba, width, height)});
    if (!withMips)
        return levels;

    std::vector<std::uint8_t> current(rgba.begin(), rgba.end());
    int currentWidth = width;
    int currentHeight = height;
    while (currentWidth / 2 >= kBlockSize && currentHeight / 2 >= kBlockSize)
    {
        int nextWidth = 0;
        int nextHeight = 0;
        current = Downsample(current, currentWidth, currentHeight, nextWidth, nextHeight);
        currentWidth = nextWidth;
        currentHeight = nextHeight;
        levels.push_back({currentWidth, currentHeight, Compress(format, current, currentWidth, currentHeight)});
    }
    return levels;
}
} // namespace Render::BlockCompression
