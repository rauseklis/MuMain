#pragma once

#include "UI/NewUI/Widgets/NineSlice.h"

typedef struct tagITEM ITEM;

#include "UI/NewUI/AuctionHouse/AuctionModel.h"

// The look the Auction House shares between its window and its panels: the nine-slice skins, the rounded item slot
// and the colours an item's rarity gives its name and slot ring. Pure drawing helpers with no window state.
namespace UI::AuctionChrome
{
// A raised button, a recessed field, the dim-gold wallet panel and a flat table row card. State is carried by the
// tint, so one texture serves normal, hover, pressed, locked, active and focused.
const NineSlice::ButtonSkin& AuctionButtonSkin();
const NineSlice::ButtonSkin& AuctionFieldSkin();
const NineSlice::ButtonSkin& AuctionPanelSkin();
const NineSlice::ButtonSkin& AuctionRowSkin();

// ARGB colour for an item's rarity (ancient, socket, excellent, +7 and up, option, plain).
DWORD GetAuctionItemRarityColor(const ITEM* item);
// Sets the text colour to the item's rarity colour.
void SetBrowseItemNameColor(const ITEM* item);
// The ring tint of an item's slot: neutral steel leaning toward the rarity colour, brightened when hovered.
DWORD GetAuctionItemSlotTint(const ITEM* item, bool hovered);
// A rounded item slot drawn from one native-size texture, tinted.
void RenderAuctionItemSlot(int x, int y, int size, DWORD tint);

// The Auction House window frame (stone background, ornate top, side and bottom bands) at any size, for windows that
// share its look. width and height are in UI logical pixels; the top and bottom caps keep their authored size.
void RenderWindowFrame(float x, float y, float width, float height);

// Short name of a price kind, for places with no icon to carry its identity.
const wchar_t* PriceKindName(AuctionHouse::PriceKind kind);

// A price spelled out in words, for places with room for a sentence rather than chips: "4 Bless, 2 Soul".
std::wstring PriceSummaryText(AuctionCurrencyMode currency, const AuctionHouse::AuctionAmount& amount);
} // namespace UI::AuctionChrome
