# Auction House handover, 2026-10-13

Branches: client `codex/auction-house` (MuMain), server `codex/auction-house` (OpenMU), both worktrees under
`mu-online-openmu/{client,server}/.worktrees/auction-house`. Nothing is pushed; every change below is a local commit.
Database: `openmu_auction_test` only (never `openmu`).

## What this session changed

### Client (`src/source/UI/NewUI/AuctionHouse`, `Widgets`, `Render/Items`)

- **Item previews** are fitted from the skinned model's measured bounds (`Render/Items/ItemPreviewBounds`), at the depth
  `RenderObjectScreen` really draws at, with the grid estimate as a guarded fallback. `ItemPreview:` lines in
  `Debug/MuError.log` give the per-model numbers and a per-mesh breakdown.
- **Chrome**: `Widgets/NineSlice` plus four textures (`newui_auction_item_slot/button/field/row.OZT`, authored at 2x with a
  separate alpha mask). `NineSlice::Render` enables alpha itself: `RenderColorQuadARGB` switches alpha test off, so a
  caret quad used to turn later controls' rounded corners square at the blink rate. Shared skins and rarity colours live
  in `AuctionChrome.{h,cpp}`.
- **Browse / My Listings / Mailbox** share a rounded table container with row cards; the slot overlaps each card. Prices
  are chips (icon + amount, up to five over two lines) laid out by `LayoutPriceCell`, used by both the 2D and 3D passes.
  Fruit Basket no longer appears anywhere in the UI (resx key replaced by "Fruits").
- **Wallet cards** (jewels in a 4-column grid, fruits in a row, Zen) with counts that shorten (`12.3k`, `1.5M`) to fit.
- **Sell tab** (`NewUIAuctionSellPanel`): item card, Auction / Buy Now modes, price rows (auction: one starting price and
  optional buyout; buy now: up to five distinct currencies with add/remove), duration, note, the fee the server charges
  today, Create/Clear. An auction is sent with today's create packet (Zen/jewel scalar, or a stat fruit as a one-fruit
  basket). **Buy Now is built but Create is disabled** (`BUY_NOW_SUPPORTED = false` in `NewUIAuctionWindow.cpp`) until
  the server protocol carries mixed prices.
- **Detail views** (listing, Mailbox collection) are stacks of cards; bid/buyout/cancel failures show specific messages
  (`DescribeOperationResult`).
- Pure, unit-tested logic is in `AuctionModel` (`tests/ui`): compact counts, wallet grid, price components, chip packing,
  sell draft validation and create-field mapping, result messages. 73 cases pass.

### Server (`src/Persistence/AuctionHouse`, `src/GameLogic/AuctionHouse`)

Step 1 of `docs/superpowers/specs/2026-10-12-auction-mixed-price-design.md` (in the monorepo root, uncommitted there):
`PriceKind` (13 wire values), `PriceComponent`, `PriceLimits`, `PriceRules` (buy-now vs auction, up to five distinct
components, only single-component listings are biddable), `SaleFee` / `SaleFeePolicy` (flat fee held at listing,
consumed by a sale, refunded otherwise; default 100,000 Zen, WCoin reserved). No schema or packet change, so the
running game is unaffected. 67 new tests pass (235 auction tests pass, 125 PostgreSQL tests skipped without
`AUCTION_TEST_CONNECTION_STRING`).

**`AuctionHouse.ini`** is created next to `MUnique.OpenMU.Startup.exe` from the current settings, lists every setting with
a description, is re-read within seconds of a save (no restart), logs and skips bad lines, and wins over the admin-panel
configuration. The test server's file has `Enabled=false` (that is what the database stored): set `Enabled=true` to allow
Sell/Bid/Buyout/Cancel/Collect.

## Decisions recorded (owner, 2026-10-12)

Buy-now listings take up to five mixed currencies and no bids; only single-component listings are biddable. Flat sale
fee, paid by the seller only, held at listing and refunded if unsold (100,000 Zen now, WCoin later); no percentage fees.
Listing fee removed. Mixed listings are hidden when a price filter or price sort is active.

## Not done / next

1. Server steps 2-3: database migration (`AuctionListingPrice` rows, fee state), the 52-byte `AuctionPrice` packet and
   handlers, the held fee. Then flip `BUY_NOW_SUPPORTED`, show the flat fee in the Sell tab, extend the Mailbox claim to
   grouped multi-component proceeds.
2. None of the new screens (Sell, detail cards, Mailbox cards) has been looked at in-game yet; expect spacing fixes.
3. Ancient Helm model 4795 draws a tiny crest plus dark head/hair meshes; the fit is of the whole bounds.
4. New Sell/detail strings are English literals; add them to the 11 `Game.*.resx` files.
5. Bid sends the minimum next bid only; no typed bid amount yet.
