// LorenciaMarketFountain.cpp: the animated water of the Loren Market fountain shown in Lorencia.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "World/GameMaps/LorenciaMarketFountain.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Effects/ZzzEffect.h"
#include "World/MapInfra/MapManager.h"

namespace World::Lorencia::MarketFountain
{
namespace
{
    constexpr float WATER_SURFACE_ANIMATION_SPEED = 0.2f;
    constexpr int HIDDEN_MESH_ALL = -2;
    constexpr float WATER_SURFACE_FLOW_AMOUNT = 0.05f;
    constexpr float WATER_SURFACE_FLOW_U_SPEED = 0.0007f;
    constexpr float WATER_SURFACE_FLOW_V_SPEED = 0.001f;
    constexpr float WATER_SURFACE_SCROLL_SPEED = 0.001f;
    constexpr int WATER_SURFACE_SCROLL_PERIOD = 10000;
    constexpr float WATER_SURFACE_LIGHT = 0.3f;
    constexpr int MESH_SPRAY = 0;
    constexpr int MESH_FALLING_WATER = 1;
    constexpr int PARTICLE_SPLASH_TYPE = 0;
    constexpr int PARTICLE_FALL_TYPE = 8;

    bool IsInLorencia()
    {
        return gMapManager.WorldActive == WD_0LORENCIA;
    }

    bool IsEmitter(const OBJECT* o)
    {
        return o->Type == MODEL_MARKET_FOUNTAIN_WATER01 || o->Type == MODEL_MARKET_FOUNTAIN_WATER02;
    }
} // namespace

void Move(OBJECT* o)
{
    if (!IsInLorencia())
        return;

    if (o->Type == MODEL_MARKET_FOUNTAIN_BASE)
    {
        o->Velocity = WATER_SURFACE_ANIMATION_SPEED;
        return;
    }

    if (IsEmitter(o))
        o->HiddenMesh = HIDDEN_MESH_ALL;
}

bool RenderObjectMesh(OBJECT* o, BMD* b)
{
    if (!IsInLorencia() || o->Type != MODEL_MARKET_FOUNTAIN_BASE)
        return false;

    const float flowU = sinf(WorldTime * WATER_SURFACE_FLOW_U_SPEED) * WATER_SURFACE_FLOW_AMOUNT;
    const float flowV = sinf(WorldTime * WATER_SURFACE_FLOW_V_SPEED) * WATER_SURFACE_FLOW_AMOUNT;
    const float scrollV = -(int)WorldTime % WATER_SURFACE_SCROLL_PERIOD * WATER_SURFACE_SCROLL_SPEED;

    b->BeginRender(o->Alpha);
    constexpr int STONE_MESHES[] = { 2, 3, 4 };
    for (const int mesh : STONE_MESHES)
        b->RenderMesh(mesh, RENDER_TEXTURE, o->Alpha, o->BlendMesh, o->BlendMeshLight);
    b->RenderMesh(MESH_FALLING_WATER, RENDER_TEXTURE | RENDER_BRIGHT, o->Alpha, MESH_FALLING_WATER,
                  WATER_SURFACE_LIGHT, o->BlendMeshTexCoordU, scrollV);
    b->RenderMesh(MESH_SPRAY, RENDER_TEXTURE | RENDER_BRIGHT, o->Alpha, MESH_SPRAY, WATER_SURFACE_LIGHT, flowU,
                  flowV);
    b->EndRender();
    return true;
}

void RenderVisual(OBJECT* o)
{
    if (!IsInLorencia() || !IsEmitter(o))
        return;

    vec3_t light;
    Vector(1.f, 1.f, 1.f, light);
    if (o->Type == MODEL_MARKET_FOUNTAIN_WATER01)
    {
        Vector(0.f, 0.f, 0.f, light);
        CreateParticleFpsChecked(BITMAP_WATERFALL_5, o->Position, o->Angle, light, PARTICLE_SPLASH_TYPE);
        return;
    }

    CreateParticleFpsChecked(BITMAP_WATERFALL_3, o->Position, o->Angle, light, PARTICLE_FALL_TYPE, o->Scale);
}
} // namespace World::Lorencia::MarketFountain
