#include "stdafx.h"
#include "Render/Items/RoyalMaceFlames.h"
#include "Render/Items/RoyalMaceFlameProfile.h"
#include "Core/Globals/_enum.h"
#include "Core/Utilities/_GlobalFunctions.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Effects/ZzzEffect.h"

namespace Render::Items::RoyalFlames
{
namespace
{
constexpr int HeadMesh = 0;
constexpr int ExpectedHeadVertices = 72;
constexpr int VertexStride = ExpectedHeadVertices / 3;
constexpr int FireSubtype = 9;
constexpr float MinimumVisibleAlpha = 0.1f;
}

void Emit(BMD* model, OBJECT* owner, int modelType, int level, float alpha,
          int selection, int renderFlags, bool translate, float (*bones)[3][4])
{
    const Profile profile = ForItem(modelType - MODEL_ITEM, MAX_ITEM_INDEX, level);
    if (profile.emitters == 0 || owner == nullptr || model == nullptr || bones == nullptr ||
        owner->Type != MODEL_PLAYER || owner->Kind != KIND_PLAYER || owner->EnableShadow ||
        alpha < MinimumVisibleAlpha || selection != 0 || (renderFlags & RENDER_WAVE))
        return;
    if (g_isCharacterBuff(owner, eBuff_Cloaking))
        return;
    if (model->NumMeshs <= HeadMesh || model->Meshs[HeadMesh].NumVertices != ExpectedHeadVertices)
        return;

    vec3_t flameLight = {1.f, 0.35f, 0.12f};
    for (int emitter = 0; emitter < profile.emitters; ++emitter)
    {
        vec3_t position;
        model->SkinVertex(HeadMesh, emitter * VertexStride, bones, translate, 0.f, position);
        CreateParticleFpsChecked(BITMAP_TRUE_FIRE, position, owner->Angle, flameLight,
                                FireSubtype, profile.scale, owner);
    }
}
}
