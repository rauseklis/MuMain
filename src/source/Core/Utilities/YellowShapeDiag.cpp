#include "stdafx.h"
#include "YellowShapeDiag.h"

#include <array>

#include "Core/Input/KeyState.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"
#include "UI/NewUI/NewUISystem.h"

namespace Diag::YellowShape
{
namespace
{
    constexpr int SWITCH_COUNT = 6;
    constexpr float ANNOUNCE_SECONDS = 10.0f;

    constexpr std::array<const wchar_t*, SWITCH_COUNT> NAMES = {
        L"weapon trail creation", L"weapon trail drawing", L"effect models", L"joints", L"GPU skinning", L"sprites and particles"};

    std::array<bool, SWITCH_COUNT> g_off{};
    std::array<bool, SWITCH_COUNT> g_wasDown{};
}

void Update()
{
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
}

bool WeaponBlurCreationOff() { return g_off[0]; }
bool BlurRenderOff() { return g_off[1]; }
bool EffectsOff() { return g_off[2]; }
bool JointsOff() { return g_off[3]; }
bool GpuSkinningOff() { return g_off[4]; }
bool SpritesAndParticlesOff() { return g_off[5]; }
}
