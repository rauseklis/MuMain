#include "stdafx.h"
#include "UI/NewUI/HUD/BuffTooltip.h"

#include "Core/Text/TextLineWrap.h"
#include "Core/Text/WideString.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/Legacy/UIControls.h"
#include "Render/Textures/ZzzTexture.h"

namespace UI::Buffs::Tooltip
{
    namespace
    {
        constexpr int MaximumLines = 28;
        constexpr int MaximumLineCharacters = 100;
        constexpr int MaximumTextWidth = 290;
        constexpr int Padding = 14;
        constexpr float Border = 16.f;
        constexpr float SourceWidth = 352.f;
        constexpr float SourceHeight = 113.f;
        constexpr float TextureWidth = 512.f;
        constexpr float TextureHeight = 128.f;

        void RenderFrame(int texture, float x, float y, float width, float height)
        {
            const float sourceX[] = {0, Border, SourceWidth - Border, SourceWidth};
            const float sourceY[] = {0, Border, SourceHeight - Border, SourceHeight};
            const float targetX[] = {x, x + Border, x + width - Border, x + width};
            const float targetY[] = {y, y + Border, y + height - Border, y + height};
            for (int row = 0; row < 3; ++row)
            {
                for (int column = 0; column < 3; ++column)
                {
                    RenderBitmap(texture, targetX[column], targetY[row],
                        targetX[column + 1] - targetX[column], targetY[row + 1] - targetY[row],
                        sourceX[column] / TextureWidth, sourceY[row] / TextureHeight,
                        (sourceX[column + 1] - sourceX[column]) / TextureWidth,
                        (sourceY[row + 1] - sourceY[row]) / TextureHeight);
                }
            }
        }
    }

    void Append(const std::wstring& text, int color, bool bold, int& count)
    {
        g_pRenderText->SetFont(bold ? g_hFontBold : g_hFont);
        auto displayText = text;
        std::size_t percent = 0;
        while ((percent = displayText.find(L"%%", percent)) != std::wstring::npos)
        {
            displayText.erase(percent, 1);
            ++percent;
        }
        const auto wrapped = WrapTextToWidth(displayText, MaximumTextWidth, [](const wchar_t* value, size_t length)
        {
            return g_pRenderText->MeasureText(value, static_cast<int>(length)).cx;
        });
        for (const auto& line : wrapped)
        {
            if (count >= MaximumLines)
            {
                break;
            }
            Core::Text::CopyWideString(TextList[count], MaximumLineCharacters, line.c_str());
            TextListColor[count] = color;
            TextBold[count] = bold;
            ++count;
        }
    }

    void Render(int anchorX, int anchorY, int count, int frameTexture)
    {
        if (count <= 0)
        {
            return;
        }
        float textWidth = 0;
        float textHeight = 0;
        for (int line = 0; line < count; ++line)
        {
            g_pRenderText->SetFont(TextBold[line] ? g_hFontBold : g_hFont);
            const SIZE measured = g_pRenderText->MeasureText(TextList[line], lstrlen(TextList[line]));
            textWidth = std::max(textWidth, static_cast<float>(measured.cx));
            textHeight += measured.cy * 1.1f;
        }
        const float width = std::max(Border * 2, textWidth + Padding * 2);
        const float height = std::max(Border * 2, textHeight + Padding * 2);
        const float x = std::clamp(anchorX - width / 2, 0.f, std::max(0.f, REFERENCE_WIDTH - width));
        const float y = std::clamp(static_cast<float>(anchorY), 0.f, std::max(0.f, REFERENCE_HEIGHT - height));
        EnableAlphaTest();
        RenderFrame(frameTexture, x, y, width, height);
        RenderTipTextList(static_cast<int>(x + width / 2), static_cast<int>(y + Padding),
            count, 0, RT3_SORT_CENTER, STRP_NONE, FALSE);
        g_pRenderText->SetFont(g_hFont);
    }
}
