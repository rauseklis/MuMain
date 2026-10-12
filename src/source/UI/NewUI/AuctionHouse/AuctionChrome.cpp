#include "stdafx.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionModel.h"
#include "UI/NewUI/NewUISystem.h"
#include "Engine/Object/ZzzInventory.h"
#include "Network/Server/SocketSystem.h"

namespace UI::AuctionChrome
{
// Native size of newui_auction_item_slot, authored at twice the slot's logical size so it stays crisp.
constexpr float ITEM_SLOT_TEXTURE_SIZE = 54.f;

static bool IsAuctionJewelItem(const ITEM* item)
{
    return item != nullptr && (item->Type == ITEM_JEWEL_OF_BLESS || item->Type == ITEM_JEWEL_OF_SOUL
        || item->Type == ITEM_JEWEL_OF_CHAOS || item->Type == ITEM_JEWEL_OF_GUARDIAN
        || item->Type == ITEM_JEWEL_OF_LIFE || item->Type == ITEM_JEWEL_OF_CREATION
        || item->Type == ITEM_JEWEL_OF_HARMONY);
}

DWORD GetAuctionItemRarityColor(const ITEM* item)
{
    if (item == nullptr)
    {
        return 0xFF7A7A7Au;
    }
    else if (IsAuctionJewelItem(item))
    {
        return 0xFFFFCC1Au;
    }
    else if (item->AncientDiscriminator > 0)
    {
        return 0xFF00FF00u;
    }
    else if (g_SocketItemMgr.IsSocketItem(item))
    {
        return 0xFFB366FFu;
    }
    else if (item->ExcellentFlags > 0)
    {
        return 0xFF1AFF80u;
    }
    else if (item->Level >= 7)
    {
        return 0xFFFFCC1Au;
    }
    else if (item->SpecialNum > 0)
    {
        return 0xFF80B3FFu;
    }

    return 0xFF8A8A8Au;
}

void SetBrowseItemNameColor(const ITEM* item)
{
    const DWORD color = GetAuctionItemRarityColor(item);
    g_pRenderText->SetTextColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, color >> 24);
}

static DWORD BlendRgb(DWORD from, DWORD to, float toShare)
{
    const auto mix = [toShare](DWORD a, DWORD b)
    {
        return static_cast<DWORD>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * toShare);
    };
    return 0xFF000000u | (mix((from >> 16) & 0xFFu, (to >> 16) & 0xFFu) << 16)
        | (mix((from >> 8) & 0xFFu, (to >> 8) & 0xFFu) << 8) | mix(from & 0xFFu, to & 0xFFu);
}

// The slot texture's ring is a neutral steel, tinted at draw time. The tint leans toward the item's rarity
// colour without adopting it fully, so the list stays calm and a rarity reads as an accent, not a neon frame.
constexpr DWORD ITEM_SLOT_NEUTRAL_RING = 0xFF78808Au;
constexpr float ITEM_SLOT_RARITY_SHARE = 0.55f;
constexpr float ITEM_SLOT_HOVER_BRIGHTEN = 0.28f;

DWORD GetAuctionItemSlotTint(const ITEM* item, bool hovered)
{
    const DWORD tint = BlendRgb(ITEM_SLOT_NEUTRAL_RING, GetAuctionItemRarityColor(item), ITEM_SLOT_RARITY_SHARE);
    return hovered ? BlendRgb(tint, 0xFFFFFFFFu, ITEM_SLOT_HOVER_BRIGHTEN) : tint;
}

// A rounded, anti-aliased container drawn from one native-size texture (ring, recessed dark fill, faint top
// highlight). A texture rather than coloured quads: flat quads cannot be rounded or anti-aliased, so the
// previous pixel-notched version always read as a blocky outline.
void RenderAuctionItemSlot(int x, int y, int size, DWORD tint)
{
    RenderImageStretch(SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_ITEM_SLOT, (float)x, (float)y, (float)size,
        (float)size, 0.f, 0.f, ITEM_SLOT_TEXTURE_SIZE, ITEM_SLOT_TEXTURE_SIZE,
        RGBA((tint >> 16) & 0xFFu, (tint >> 8) & 0xFFu, tint & 0xFFu, 255));
}

// The button and field textures are neutral steel authored at two texels per logical pixel; their caps
// are the 4px corner radius plus the ring and bevel. State is carried by the draw-time tint, so one
// texture serves normal, hover, pressed, locked, active and focused.
constexpr float CHROME_TEXELS_PER_PIXEL = 2.f;
constexpr float CHROME_CAP_TEXELS = 12.f;
constexpr float CHROME_BUTTON_TEXTURE_WIDTH = 80.f;
constexpr float CHROME_FIELD_TEXTURE_WIDTH = 96.f;
constexpr float CHROME_TEXTURE_HEIGHT = 48.f;

static UI::NineSlice::Texture MakeChromeTexture(int imageIndex, float width)
{
    UI::NineSlice::Texture texture;
    texture.imageIndex = imageIndex;
    texture.width = width;
    texture.height = CHROME_TEXTURE_HEIGHT;
    texture.texelsPerPixel = CHROME_TEXELS_PER_PIXEL;
    texture.insets = { CHROME_CAP_TEXELS, CHROME_CAP_TEXELS, CHROME_CAP_TEXELS, CHROME_CAP_TEXELS };
    return texture;
}

const UI::NineSlice::ButtonSkin& AuctionButtonSkin()
{
    static const UI::NineSlice::ButtonSkin skin{
        MakeChromeTexture(SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_BUTTON, CHROME_BUTTON_TEXTURE_WIDTH),
        { RGBA(172, 178, 190, 255), RGBA(236, 241, 250, 255), RGBA(112, 116, 126, 255), RGBA(88, 90, 96, 255) } };
    return skin;
}

// For recessed fields (inputs, combos, wallet panels). "pressed" is the open/focused state, a gold ring.
const UI::NineSlice::ButtonSkin& AuctionFieldSkin()
{
    static const UI::NineSlice::ButtonSkin skin{
        MakeChromeTexture(SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_FIELD, CHROME_FIELD_TEXTURE_WIDTH),
        { RGBA(142, 148, 160, 255), RGBA(200, 206, 218, 255), RGBA(232, 198, 112, 255), RGBA(142, 148, 160, 255) } };
    return skin;
}

const UI::NineSlice::ButtonSkin& AuctionPanelSkin()
{
    static const UI::NineSlice::ButtonSkin skin{
        MakeChromeTexture(SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_FIELD, CHROME_FIELD_TEXTURE_WIDTH),
        { RGBA(150, 128, 78, 255), RGBA(150, 128, 78, 255), RGBA(150, 128, 78, 255), RGBA(150, 128, 78, 255) } };
    return skin;
}

// Flat dark cards for table rows and the table's own container: steel at rest, gold when pointed at.
const UI::NineSlice::ButtonSkin& AuctionRowSkin()
{
    static const UI::NineSlice::ButtonSkin skin{
        MakeChromeTexture(SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_ROW, CHROME_FIELD_TEXTURE_WIDTH),
        { RGBA(68, 74, 86, 255), RGBA(224, 192, 108, 255), RGBA(68, 74, 86, 255), RGBA(68, 74, 86, 255) } };
    return skin;
}

// Short names used when a price component has no icon to carry its identity.
const wchar_t* PriceKindName(AuctionHouse::PriceKind kind)
{
    static const wchar_t* const names[] = { L"Zen", L"Chaos", L"Bless", L"Soul", L"Life", L"Creation", L"Guardian",
        L"Harmony", L"Str", L"Agi", L"Vit", L"Ene", L"Cmd" };
    return names[static_cast<size_t>(kind)];
}
} // namespace UI::AuctionChrome
