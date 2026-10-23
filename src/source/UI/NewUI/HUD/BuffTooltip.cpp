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
        constexpr int PaddingX = 12;
        constexpr int PaddingY = 9;
        constexpr int MinimumWidth = 90;
        constexpr float DividerHeight = 1.f;
        constexpr float HalfLineFactor = 0.55f;
        constexpr float LineFactor = 1.1f;
        constexpr float CornerSize = 3.f;
        constexpr float HighlightBands = 6.f;

        constexpr unsigned int ShadowOuter = 0x30000000;
        constexpr unsigned int ShadowInner = 0x50000000;
        constexpr unsigned int BodyColor = 0xF2100D16;
        constexpr unsigned int HighlightColor = 0x0CFFE2A0;
        constexpr unsigned int BorderOuter = 0xFF6B5326;
        constexpr unsigned int BorderInner = 0x55E6C477;
        constexpr unsigned int CornerColor = 0xFFE6C477;
        constexpr unsigned int DividerEdge = 0x22E6C477;
        constexpr unsigned int DividerCentre = 0xA0E6C477;

        bool IsDivider(int line)
        {
            return TextList[line][0] == L'\n';
        }

        float LineHeight(int line)
        {
            g_pRenderText->SetFont(TextBold[line] ? g_hFontBold : g_hFont);
            if (IsDivider(line))
            {
                return g_pRenderText->MeasureText(L"Q", 1).cy * HalfLineFactor;
            }
            return g_pRenderText->MeasureText(TextList[line], lstrlen(TextList[line])).cy * LineFactor;
        }

        void RenderBox(float x, float y, float width, float height)
        {
            RenderColorQuadARGB(x + 2, y + 3, width, height, ShadowOuter);
            RenderColorQuadARGB(x + 1, y + 2, width, height, ShadowInner);
            RenderColorQuadARGB(x, y, width, height, BorderOuter);
            RenderColorQuadARGB(x + 1, y + 1, width - 2, height - 2, BodyColor);
            const float bandHeight = (height - 2) / HighlightBands / 2.f;
            for (int band = 0; band < static_cast<int>(HighlightBands); ++band)
            {
                const unsigned int alpha = (HighlightColor >> 24) * (HighlightBands - band) / HighlightBands;
                RenderColorQuadARGB(x + 1, y + 1 + band * bandHeight, width - 2, bandHeight,
                    (alpha << 24) | (HighlightColor & 0x00FFFFFF));
            }
            RenderColorQuadARGB(x + 2, y + 2, width - 4, 1, BorderInner);
            RenderColorQuadARGB(x + 2, y + height - 3, width - 4, 1, BorderInner);
            RenderColorQuadARGB(x + 2, y + 2, 1, height - 4, BorderInner);
            RenderColorQuadARGB(x + width - 3, y + 2, 1, height - 4, BorderInner);
            RenderColorQuadARGB(x, y, CornerSize, CornerSize, CornerColor);
            RenderColorQuadARGB(x + width - CornerSize, y, CornerSize, CornerSize, CornerColor);
            RenderColorQuadARGB(x, y + height - CornerSize, CornerSize, CornerSize, CornerColor);
            RenderColorQuadARGB(x + width - CornerSize, y + height - CornerSize, CornerSize, CornerSize, CornerColor);
        }

        void RenderDivider(float x, float y, float width)
        {
            const float inset = PaddingX - 2.f;
            const float third = (width - inset * 2) / 3.f;
            RenderColorQuadARGB(x + inset, y, third, DividerHeight, DividerEdge);
            RenderColorQuadARGB(x + inset + third, y, third, DividerHeight, DividerCentre);
            RenderColorQuadARGB(x + inset + third * 2, y, third, DividerHeight, DividerEdge);
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

    void AppendDivider(int& count)
    {
        if (count >= MaximumLines)
        {
            return;
        }
        Core::Text::CopyWideString(TextList[count], MaximumLineCharacters, L"\n");
        TextListColor[count] = TEXT_COLOR_WHITE;
        TextBold[count] = false;
        ++count;
    }

    void Render(int anchorX, int anchorY, int count)
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
            if (!IsDivider(line))
            {
                textWidth = std::max(textWidth, static_cast<float>(g_pRenderText->MeasureText(TextList[line], lstrlen(TextList[line])).cx));
            }
            textHeight += LineHeight(line);
        }
        const float width = std::max(static_cast<float>(MinimumWidth), textWidth + PaddingX * 2);
        const float height = textHeight + PaddingY * 2;
        const float x = std::clamp(anchorX - width / 2, 0.f, std::max(0.f, REFERENCE_WIDTH - width));
        const float y = std::clamp(static_cast<float>(anchorY), 0.f, std::max(0.f, REFERENCE_HEIGHT - height));
        EnableAlphaTest();
        RenderBox(x, y, width, height);
        float lineY = y + PaddingY;
        for (int line = 0; line < count; ++line)
        {
            if (IsDivider(line))
            {
                RenderDivider(x, lineY + LineHeight(line) / 2.f, width);
            }
            lineY += LineHeight(line);
        }
        EndRenderColor();
        RenderTipTextList(static_cast<int>(x + width / 2), static_cast<int>(y + PaddingY),
            count, 0, RT3_SORT_CENTER, STRP_NONE, FALSE);
        g_pRenderText->SetFont(g_hFont);
    }
}
