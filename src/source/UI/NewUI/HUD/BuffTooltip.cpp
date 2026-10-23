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
        constexpr float CornerSize = 18.f;
        constexpr float AtlasWidth = 128.f;
        constexpr float AtlasHeight = 32.f;
        constexpr float AtlasCornerSize = 24.f;
        constexpr float AtlasRuleX = 28.f;
        constexpr float AtlasRuleWidth = 96.f;
        constexpr float AtlasRuleHeight = 4.f;
        constexpr float RuleHeight = 4.f;
        // Border strips, in atlas pixels (see tools/ui_hd/tooltip_ornaments.py), and their thickness on screen.
        constexpr float AtlasStripX = 28.f;
        constexpr float AtlasTopStripY = 6.f;
        constexpr float AtlasLeftStripY = 10.f;
        constexpr float AtlasStripLong = 4.f;
        constexpr float AtlasStripThin = 2.75f;
        constexpr float AtlasInset = 0.125f;
        constexpr float BorderThickness = AtlasStripThin * CornerSize / AtlasCornerSize;
        constexpr float HighlightBands = 6.f;

        constexpr unsigned int ShadowOuter = 0x30000000;
        constexpr unsigned int ShadowInner = 0x50000000;
        constexpr unsigned int BodyColor = 0xF2100D16;
        constexpr unsigned int HighlightColor = 0x0CFFE2A0;

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
            RenderColorQuadARGB(x, y, width, height, BodyColor);
            const float bandHeight = height / HighlightBands / 2.f;
            for (int band = 0; band < static_cast<int>(HighlightBands); ++band)
            {
                const unsigned int alpha = (HighlightColor >> 24) * (HighlightBands - band) / HighlightBands;
                RenderColorQuadARGB(x, y + band * bandHeight, width, bandHeight,
                    (alpha << 24) | (HighlightColor & 0x00FFFFFF));
            }
        }

        // Draws the atlas rectangle (in atlas pixels) mirrored horizontally and/or vertically.
        void RenderAtlas(int texture, float x, float y, float width, float height,
            float sourceX, float sourceY, float sourceWidth, float sourceHeight, bool flipX, bool flipY)
        {
            float u = sourceX / AtlasWidth;
            float v = sourceY / AtlasHeight;
            float uWidth = sourceWidth / AtlasWidth;
            float vHeight = sourceHeight / AtlasHeight;
            if (flipX)
            {
                u += uWidth;
                uWidth = -uWidth;
            }
            if (flipY)
            {
                v += vHeight;
                vHeight = -vHeight;
            }
            RenderBitmap(texture, x, y, width, height, u, v, uWidth, vHeight);
        }

        void RenderBorder(int texture, float x, float y, float width, float height)
        {
            const float runWidth = width - CornerSize * 2;
            const float runHeight = height - CornerSize * 2;
            const float topX = AtlasStripX + AtlasInset;
            const float longRun = AtlasStripLong - AtlasInset * 2;
            const float thin = AtlasStripThin - AtlasInset;
            RenderAtlas(texture, x + CornerSize, y, runWidth, BorderThickness,
                topX, AtlasTopStripY + AtlasInset, longRun, thin, false, false);
            RenderAtlas(texture, x + CornerSize, y + height - BorderThickness, runWidth, BorderThickness,
                topX, AtlasTopStripY + AtlasInset, longRun, thin, false, true);
            RenderAtlas(texture, x, y + CornerSize, BorderThickness, runHeight,
                topX, AtlasLeftStripY + AtlasInset, thin, longRun, false, false);
            RenderAtlas(texture, x + width - BorderThickness, y + CornerSize, BorderThickness, runHeight,
                topX, AtlasLeftStripY + AtlasInset, thin, longRun, true, false);
        }

        void RenderCorners(int texture, float x, float y, float width, float height)
        {
            const float right = x + width - CornerSize;
            const float bottom = y + height - CornerSize;
            RenderAtlas(texture, x, y, CornerSize, CornerSize, 0, 0, AtlasCornerSize, AtlasCornerSize, false, false);
            RenderAtlas(texture, right, y, CornerSize, CornerSize, 0, 0, AtlasCornerSize, AtlasCornerSize, true, false);
            RenderAtlas(texture, x, bottom, CornerSize, CornerSize, 0, 0, AtlasCornerSize, AtlasCornerSize, false, true);
            RenderAtlas(texture, right, bottom, CornerSize, CornerSize, 0, 0, AtlasCornerSize, AtlasCornerSize, true, true);
        }

        void RenderRule(int texture, float x, float y, float width)
        {
            const float inset = PaddingX - 2.f;
            RenderAtlas(texture, x + inset, y - RuleHeight / 2.f, width - inset * 2, RuleHeight,
                AtlasRuleX, 0, AtlasRuleWidth, AtlasRuleHeight, false, false);
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

    void Render(int anchorX, int anchorY, int count, int ornamentTexture)
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
        EndRenderColor();
        RenderBorder(ornamentTexture, x, y, width, height);
        RenderCorners(ornamentTexture, x, y, width, height);
        float lineY = y + PaddingY;
        for (int line = 0; line < count; ++line)
        {
            if (IsDivider(line))
            {
                RenderRule(ornamentTexture, x, lineY + LineHeight(line) / 2.f, width);
            }
            lineY += LineHeight(line);
        }
        RenderTipTextList(static_cast<int>(x + width / 2), static_cast<int>(y + PaddingY),
            count, 0, RT3_SORT_CENTER, STRP_NONE, FALSE);
        g_pRenderText->SetFont(g_hFont);
    }
}
