#include "stdafx.h"
#include "Core/Text/TextLineWrap.h"
#include "UI/NewUI/HUD/Notices.h"

#include "UI/Legacy/UIControls.h"        // g_pRenderText, RT3_WRITE_CENTER
#include "App/Platform/Windows/Winmain.h"    // g_hFontBold
#include "Render/Textures/ZzzOpenglUtil.h" // EnableAlphaTest
#include "UI/NewUI/NewUISystem.h"        // g_pNewUISystem
#include "Engine/AI/ZzzAI.h"             // FPS_ANIMATION_FACTOR
#include "Engine/Object/ZzzInterface.h"  // CutText

namespace
{
// At most four lines, so the stack ends above the kill notice (UI::KillNotice, y = 72).
constexpr int MAX_NOTICE = 4;
// The stack hangs from the top centre of the screen. The old spot, mid-screen at y = 300, sat on top of every open
// window (Auction House, shop, inventory, ...). Windows leave the strip above them free (the Auction House starts at
// y = 25 of its panel) and the docked windows sit at the sides, so the first lines fit between and above them; the
// plate keeps the text readable where a longer stack reaches a window title.
constexpr int NOTICE_CENTER_X = 320;
constexpr int NOTICE_TOP_Y = 2;
constexpr int NOTICE_LINE_HEIGHT = 13;
constexpr BYTE NOTICE_PLATE_ALPHA = 200;
// Ticks of the original 25 Hz game loop (12 seconds) until the oldest line is pushed up and off.
constexpr float NOTICE_LIFETIME = 300.f;
constexpr int NOTICE_TEXT_MAX = 256;

struct Notice
{
    wchar_t Text[NOTICE_TEXT_MAX];
    int LifeTime;
    BYTE Color;
};

    int    s_count = 0;
    // A float: counting down by FPS_ANIMATION_FACTOR (below 1 at a high frame rate) into an int truncated every step
    // to a whole 1, so the lines left the screen 2.4 times as fast at 60 fps and 6 times as fast at 144 fps.
    float  s_time = NOTICE_LIFETIME;
    float  s_blinkPhase = 0.f;
    Notice s_notices[MAX_NOTICE];

    // Shift the buffer up by one when it is full so the newest line fits.
    void Scroll()
    {
        if (s_count > MAX_NOTICE - 1)
        {
            s_count = MAX_NOTICE - 1;
            for (int i = 1; i < MAX_NOTICE; i++)
            {
                s_notices[i - 1].Color = s_notices[i].Color;
                wcscpy(s_notices[i - 1].Text, s_notices[i].Text);
            }
        }
    }
}

namespace UI::Notices
{
    void CopyText(wchar_t (&destination)[NOTICE_TEXT_MAX], const wchar_t* source)
    {
        wcsncpy_s(destination, source, _TRUNCATE);
    }

    void Clear()
    {
        memset(s_notices, 0, sizeof(s_notices));
    }

    void Create(const wchar_t* text, int color)
    {
        g_pRenderText->SetFont(g_hFontBold);
        const SIZE size = g_pRenderText->MeasureText(text, lstrlen(text));

        Scroll();
        s_notices[s_count].Color = color;
        if (size.cx < NOTICE_TEXT_MAX)
        {
            CopyText(s_notices[s_count++].Text, text);
        }
        else
        {
            wchar_t topText[NOTICE_TEXT_MAX] = { 0 };
            wchar_t bottomText[NOTICE_TEXT_MAX] = { 0 };
            CutText(text, topText, bottomText, NOTICE_TEXT_MAX);
            CopyText(s_notices[s_count++].Text, topText);
            Scroll();
            s_notices[s_count].Color = color;
            CopyText(s_notices[s_count++].Text, bottomText);
        }
        s_time = NOTICE_LIFETIME;
    }

    void Move()
    {
        s_time -= FPS_ANIMATION_FACTOR;
        if (s_time <= 0)
        {
            s_time = NOTICE_LIFETIME;
            Create(L"", 0);
        }
    }

    void Render()
    {
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
        if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == true)
            return;
#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM

        EnableAlphaTest();

        g_pRenderText->SetFont(g_hFontBold);

        for (int i = 0; i < MAX_NOTICE; i++)
        {
            Notice* n = &s_notices[i];
            if (n->Color == 0)
            {
                g_pRenderText->SetBgColor(0, 0, 0, NOTICE_PLATE_ALPHA);
                if ((int)s_blinkPhase % 10 < 5)
                {
                    g_pRenderText->SetTextColor(255, 200, 80, 128);
                }
                else
                {
                    g_pRenderText->SetTextColor(255, 200, 80, 255);
                }
            }
            else
            {
                g_pRenderText->SetTextColor(100, 255, 200, 255);
                g_pRenderText->SetBgColor(0, 0, 0, NOTICE_PLATE_ALPHA);
            }

            g_pRenderText->RenderText(NOTICE_CENTER_X, NOTICE_TOP_Y + i * NOTICE_LINE_HEIGHT, n->Text, 0, 0,
                                      RT3_WRITE_CENTER);
        }

        s_blinkPhase += FPS_ANIMATION_FACTOR;
    }
}
