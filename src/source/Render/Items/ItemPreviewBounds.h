#pragma once

#include <optional>

// Fits an item's true rendered silhouette into a UI rectangle, instead of estimating from its
// inventory grid footprint (ItemAttribute.Width/Height). Grid metadata says how an item tiles in a
// full-size inventory; it does not measure the model's rendered extent, so meshes (or authored
// rotations) that reach beyond their declared footprint - bows, scythes, wings - still cross a
// tight preview cell's border. This skins the model once per (item, level) through the same path the
// real render uses, then projects its vertices onto the UI pass's own screen axes.
namespace Render::Items::Preview
{
// RenderObjectScreen does not draw an item at the far-plane point CameraProjection::ScreenToWorldRay
// returns: it draws it this fraction of the way from the camera to that point. Screen size per world
// unit is inversely proportional to that depth, so the measurement here and the real render must use
// the same fraction - they share these constants so they cannot drift apart.
inline constexpr float ItemDepthFraction = 0.1f;
inline constexpr float PickedItemDepthFraction = 0.07f;

struct Placement
{
    // The RenderItem3DScaled multiplier that fits the silhouette inside the requested area.
    float scale = 1.0f;
    // Added to the slot rectangle's position before drawing, so the silhouette - not the model's
    // authored origin - is centred in the slot.
    float shiftX = 0.0f;
    float shiftY = 0.0f;
};

// Fits itemType/level's projected silhouette into a slotWidth x slotHeight rectangle (UI logical
// pixels), occupying `fill` of the rectangle on its tightest axis (0.1-1.0), centred. Returns
// std::nullopt if the model can't be measured: a handful of seasonal/event items render through
// RenderItem3D's special-cased model switch instead of the ordinary GetInventoryModel/GetDrawnModel
// resolution measured here. Callers should fall back to the grid-based estimate for those.
std::optional<Placement> ComputePlacement(int itemType, int level, float slotWidth, float slotHeight, float fill,
                                          float depthFraction = ItemDepthFraction);
} // namespace Render::Items::Preview
