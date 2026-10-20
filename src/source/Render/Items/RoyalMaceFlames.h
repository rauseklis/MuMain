#pragma once

class BMD;
class OBJECT;

namespace Render::Items::RoyalFlames
{
void Emit(BMD* model, OBJECT* owner, int modelType, int level, float alpha,
          int selection, int renderFlags, bool translate, float (*bones)[3][4]);

// Draws the flames straight into an inventory/hover/picked-item preview, which never runs the particle pass.
// Call after the weapon has been transformed with `bones`.
void RenderPreview(BMD* model, int modelType, int level, bool translate, float (*bones)[3][4]);
}
