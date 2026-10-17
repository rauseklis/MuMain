#pragma once

class BMD;
class OBJECT;

namespace Render::Items::RoyalFlames
{
void Emit(BMD* model, OBJECT* owner, int modelType, int level, float alpha,
          int selection, int renderFlags, bool translate, float (*bones)[3][4]);
}
