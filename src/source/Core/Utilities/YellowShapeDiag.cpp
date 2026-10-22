#include "stdafx.h"
#include "YellowShapeDiag.h"

#include <array>
#include <map>
#include <string>
#include <vector>

#include "Core/Input/KeyState.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"
#include "UI/NewUI/NewUISystem.h"

extern double WorldTime;

namespace Diag::YellowShape
{
namespace
{
    constexpr int SWITCH_COUNT = 6;
    constexpr float ANNOUNCE_SECONDS = 10.0f;

    constexpr std::array<const wchar_t*, SWITCH_COUNT> NAMES = {
        L"weapon trail creation", L"weapon trail drawing", L"effect models", L"joints", L"GPU skinning", L"sprites and particles"};

    constexpr int MATERIAL_COUNT = 4;
    constexpr std::array<int, MATERIAL_COUNT> MATERIAL_KEYS = {'Z', 'X', 'C', 'V'};
    constexpr std::array<const wchar_t*, MATERIAL_COUNT> MATERIAL_NAMES = {L"GPU skinning of textured meshes", L"GPU skinning of flat colour meshes",
                                                                           L"GPU skinning of bright meshes", L"GPU skinning of environment mapped meshes"};
    std::array<bool, MATERIAL_COUNT> g_materialOff{};
    std::array<bool, MATERIAL_COUNT> g_materialWasDown{};

    // The models seen with GPU skinning during the last two seconds (pointer and name), and the one switched off.
    std::map<const void*, std::string> g_seenNow;
    std::map<const void*, std::string> g_seenBefore;
    double g_seenWindowStart = 0.0;
    const void* g_modelOff = nullptr;
    bool g_modelKeyWasDown = false;

    std::array<bool, SWITCH_COUNT> g_off{};
    std::array<bool, SWITCH_COUNT> g_wasDown{};
}

void Update()
{
    const bool modelKeyDown = Core::Input::IsKeyDown(VK_CONTROL) && Core::Input::IsKeyDown(VK_SHIFT) && Core::Input::IsKeyDown('B');
    if (modelKeyDown && !g_modelKeyWasDown && !g_seenBefore.empty())
    {
        // The next model after the one which is switched off now; after the last one all are on again.
        auto next = g_modelOff == nullptr ? g_seenBefore.begin() : g_seenBefore.upper_bound(g_modelOff);
        g_modelOff = next == g_seenBefore.end() ? nullptr : next->first;
        wchar_t text[160];
        if (g_modelOff == nullptr)
        {
            swprintf(text, std::size(text), L"[diag] model GPU skinning: all on");
        }
        else
        {
            swprintf(text, std::size(text), L"[diag] GPU skinning OFF for model %hs (%d of %d)", next->second.c_str(),
                     static_cast<int>(std::distance(g_seenBefore.begin(), next)) + 1, static_cast<int>(g_seenBefore.size()));
        }

        SEASON3B::ScopedMessageLifetime visible(ANNOUNCE_SECONDS);
        g_pSystemLogBox->AddText(text, SEASON3B::TYPE_SYSTEM_MESSAGE);
    }

    g_modelKeyWasDown = modelKeyDown;
    if (WorldTime - g_seenWindowStart > 2000.0)
    {
        g_seenWindowStart = WorldTime;
        if (g_modelOff == nullptr)
        {
            g_seenBefore = g_seenNow;
        }

        g_seenNow.clear();
    }

    const bool modifiers = Core::Input::IsKeyDown(VK_CONTROL) && Core::Input::IsKeyDown(VK_SHIFT);
    for (int i = 0; i < SWITCH_COUNT; ++i)
    {
        const bool down = modifiers && Core::Input::IsKeyDown(VK_F1 + i);
        if (down && !g_wasDown[i])
        {
            g_off[i] = !g_off[i];
            wchar_t text[96];
            swprintf(text, std::size(text), L"[diag] %ls: %ls", NAMES[i], g_off[i] ? L"OFF" : L"on");
            SEASON3B::ScopedMessageLifetime visible(ANNOUNCE_SECONDS);
            g_pSystemLogBox->AddText(text, SEASON3B::TYPE_SYSTEM_MESSAGE);
        }

        g_wasDown[i] = down;
    }

    for (int i = 0; i < MATERIAL_COUNT; ++i)
    {
        const bool down = modifiers && Core::Input::IsKeyDown(MATERIAL_KEYS[i]);
        if (down && !g_materialWasDown[i])
        {
            g_materialOff[i] = !g_materialOff[i];
            wchar_t text[96];
            swprintf(text, std::size(text), L"[diag] %ls: %ls", MATERIAL_NAMES[i], g_materialOff[i] ? L"OFF" : L"on");
            SEASON3B::ScopedMessageLifetime visible(ANNOUNCE_SECONDS);
            g_pSystemLogBox->AddText(text, SEASON3B::TYPE_SYSTEM_MESSAGE);
        }

        g_materialWasDown[i] = down;
    }
}

void NoteGpuSkinnedModel(const void* model, const char* name)
{
    g_seenNow.try_emplace(model, name);
}

bool GpuSkinningOffForModel(const void* model) { return model != nullptr && model == g_modelOff; }

bool GpuSkinningOffForMaterial(int material) { return material >= 0 && material < MATERIAL_COUNT && g_materialOff[material]; }

bool WeaponBlurCreationOff() { return g_off[0]; }
bool BlurRenderOff() { return g_off[1]; }
bool EffectsOff() { return g_off[2]; }
bool JointsOff() { return g_off[3]; }
bool GpuSkinningOff() { return g_off[4]; }
bool SpritesAndParticlesOff() { return g_off[5]; }
}
