#include "stdafx.h"
#include "KillNotice.h"

#include <algorithm>
#include <chrono>
#include <string>

#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/Combat/DuelMgr.h"
#include "I18N/All.h"
#include "Render/Textures/ZzzOpenglUtil.h" // BeginBitmap/EndBitmap
#include "Scenes/SceneCore.h"
#include "UI/Legacy/UIControls.h"             // g_pRenderText, RT3_SORT_CENTER
#include "App/Platform/Windows/Winmain.h"     // g_hFontBold

extern CDuelMgr g_DuelMgr;

namespace UI::KillNotice
{
namespace
{
    using Clock = std::chrono::steady_clock;

    constexpr float STAY_AFTER_MOVING_SECONDS = 30.0f;
    constexpr float FADE_SECONDS = 3.0f;

    // Position and size in the 640x480 reference space of the 2D helpers.
    constexpr float NOTICE_Y = 72.0f;
    constexpr float NOTICE_WIDTH = 360.0f;

    // Deep red background and pale gold letters: readable on dark and bright maps.
    constexpr BYTE BG_R = 90;
    constexpr BYTE BG_G = 10;
    constexpr BYTE BG_B = 20;
    constexpr BYTE BG_ALPHA = 225;
    constexpr BYTE TEXT_R = 255;
    constexpr BYTE TEXT_G = 226;
    constexpr BYTE TEXT_B = 150;

    struct State
    {
        bool active = false;
        bool movedSinceDeath = false;
        Clock::time_point movedAt;
        std::wstring text;
    };

    State g_state;

    // The hero is back on its feet and has started to move again.
    bool HeroIsMoving()
    {
        return Hero != nullptr && Hero->Dead <= 0.0f && Hero->Movement != 0;
    }

    float Opacity()
    {
        if (!g_state.movedSinceDeath)
        {
            return 1.0f;
        }

        const float seconds = std::chrono::duration<float>(Clock::now() - g_state.movedAt).count();
        return std::clamp(1.0f - (seconds - STAY_AFTER_MOVING_SECONDS) / FADE_SECONDS, 0.0f, 1.0f);
    }

    void UpdateTiming()
    {
        if (!g_state.movedSinceDeath && HeroIsMoving())
        {
            g_state.movedSinceDeath = true;
            g_state.movedAt = Clock::now();
        }

        if (g_state.movedSinceDeath && Opacity() <= 0.0f)
        {
            g_state = State{};
        }
    }

    void DrawNotice(float opacity)
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(BG_R, BG_G, BG_B, static_cast<BYTE>(BG_ALPHA * opacity));
        g_pRenderText->SetTextColor(TEXT_R, TEXT_G, TEXT_B, static_cast<BYTE>(255 * opacity));
        g_pRenderText->RenderText(static_cast<int>((REFERENCE_WIDTH - NOTICE_WIDTH) / 2.0f), static_cast<int>(NOTICE_Y),
                                  g_state.text.c_str(), static_cast<int>(NOTICE_WIDTH), 0, RT3_SORT_CENTER);
    }
}

void OnHeroDied(int killerKey)
{
    if (g_DuelMgr.IsDuelEnabled())
    {
        return;
    }

    const int index = FindCharacterIndex(killerKey);
    if (index >= MAX_CHARACTERS_CLIENT)
    {
        return;
    }

    const CHARACTER& killer = CharactersClient[index];
    if (&killer == Hero || killer.Object.Kind != KIND_PLAYER)
    {
        return;
    }

    wchar_t text[128];
    mu_swprintf(text, I18N::Game::YouWereKilledByPlayer, killer.ID);
    g_state = State{};
    g_state.active = true;
    g_state.text = text;
}

void Render()
{
    if (!g_state.active)
    {
        return;
    }

    if (SceneFlag != MAIN_SCENE)
    {
        return;
    }

    UpdateTiming();
    if (!g_state.active)
    {
        return;
    }

    BeginBitmap();
    DrawNotice(Opacity());
    EndBitmap();
}
}
