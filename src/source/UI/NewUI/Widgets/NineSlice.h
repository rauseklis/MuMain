#pragma once

// Nine-slice rendering for UI chrome that has to fit controls of arbitrary size: the four corners are
// drawn at their authored size, the four edges stretch along one axis only, and the centre stretches in
// both. Stretching a whole sprite instead - as a button sprite used to be for the toolbar fields - blows
// its borders up with its width (a one-pixel frame becomes a thick bar), which is what this avoids.
namespace UI::NineSlice
{
// How much of the source texture is corner/edge, in texels.
struct Insets
{
    float left = 0.f;
    float top = 0.f;
    float right = 0.f;
    float bottom = 0.f;
};

// A texture laid out for nine-slicing. width/height are the used image extents in texels (the engine
// pads textures to a power of two; the padding is not part of the image). texelsPerPixel is how many
// texels the art spends per logical UI pixel: art authored at 2x for crispness at the UI's ~2x device
// scale has 2, legacy 1:1 art has 1.
struct Texture
{
    int imageIndex = 0;
    float width = 0.f;
    float height = 0.f;
    float texelsPerPixel = 1.f;
    Insets insets;
};

// Draws `texture` over the destination rectangle (UI logical pixels), multiplying it by color (an RGBA()
// value). A destination smaller than the corners shrinks them proportionally instead of overlapping.
void Render(const Texture& texture, float x, float y, float width, float height, unsigned int color);

// Per-state tints for a skinned control. The texture itself is neutral; the state is carried by colour.
struct StateTints
{
    unsigned int normal = 0;
    unsigned int hover = 0;
    unsigned int pressed = 0;
    unsigned int locked = 0;
};

struct ButtonSkin
{
    Texture texture;
    StateTints tints;
};
} // namespace UI::NineSlice
