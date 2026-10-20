#include "stdafx.h"
#include "Render/Items/RoyalMaceFlames.h"
#include "Render/Items/RoyalMaceFlameProfile.h"
#include "Core/Globals/_enum.h"
#include "Core/Utilities/_GlobalFunctions.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include <algorithm>
#include <cmath>

namespace Render::Items::RoyalFlames
{
namespace
{
constexpr int HeadMesh = 0;
constexpr int ExpectedHeadVertices = 72;
constexpr int VertexStride = ExpectedHeadVertices / 3;
constexpr int FireSubtype = 9;
constexpr float MinimumVisibleAlpha = 0.1f;

// Inventory previews have no particle pass, so the flame is drawn as sprites that replay the in-world
// particle: a FireSubtype particle lives 20 ticks, rises 1 unit per tick, and fades as life / 25.
constexpr int PreviewSpritesPerEmitter = 5;
constexpr float ParticleLifeTicks = 20.f;
constexpr float ParticleFadeDivisor = 25.f;
constexpr float RisePerTick = 1.f;
constexpr float ScaleLossPerTick = 0.02f;
constexpr double MillisecondsPerTick = 40.0;

bool HasUsableHeadMesh(const BMD* model)
{
    return model != nullptr && model->NumMeshs > HeadMesh &&
           model->Meshs[HeadMesh].NumVertices == ExpectedHeadVertices;
}

void EmitterPosition(const BMD* model, int emitter, bool translate, float (*bones)[3][4], vec3_t position)
{
    model->SkinVertex(HeadMesh, emitter * VertexStride, bones, translate, 0.f, position);
}
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
    if (!HasUsableHeadMesh(model))
        return;

    vec3_t flameLight = {1.f, 0.35f, 0.12f};
    for (int emitter = 0; emitter < profile.emitters; ++emitter)
    {
        vec3_t position;
        EmitterPosition(model, emitter, translate, bones, position);
        CreateParticleFpsChecked(BITMAP_TRUE_FIRE, position, owner->Angle, flameLight,
                                FireSubtype, profile.scale, owner);
    }
}

void RenderPreview(BMD* model, int modelType, int level, bool translate, float (*bones)[3][4])
{
    const Profile profile = ForItem(modelType - MODEL_ITEM, MAX_ITEM_INDEX, level);
    if (profile.emitters == 0 || bones == nullptr || !HasUsableHeadMesh(model))
        return;

    const double ticks = WorldTime / MillisecondsPerTick;
    const BITMAP_t* bitmap = Bitmaps.GetTexture(BITMAP_TRUE_FIRE);

    BeginSprite();
    EnableAlphaBlend();
    for (int emitter = 0; emitter < profile.emitters; ++emitter)
    {
        vec3_t origin;
        EmitterPosition(model, emitter, translate, bones, origin);
        for (int sprite = 0; sprite < PreviewSpritesPerEmitter; ++sprite)
        {
            const float phase = static_cast<float>(sprite) / PreviewSpritesPerEmitter;
            const float age = std::fmod(static_cast<float>(ticks) + phase * ParticleLifeTicks, ParticleLifeTicks);
            const float fade = (ParticleLifeTicks - age) / ParticleFadeDivisor;
            const float particleScale = std::max(0.f, profile.scale - age * ScaleLossPerTick);
            const float size = bitmap->Width * particleScale * model->BodyScale;
            vec3_t position = {origin[0], origin[1], origin[2] + age * RisePerTick * model->BodyScale};
            vec3_t light = {fade, fade, fade};
            RenderSprite(BITMAP_TRUE_FIRE, position, size, size, light);
        }
    }
    EndSprite();
    DisableAlphaBlend();
}
}
