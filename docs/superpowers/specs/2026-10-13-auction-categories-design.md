# Auction House: sub-categories and the new category rail

Date: 2026-10-13. Branch: `codex/auction-house` (client and server). Status: approved in conversation, written spec awaiting
the owner's review.

## Goal

Players browsing the Auction House can narrow a category to the kind of item they want: under Armor pick Gloves, under
Weapons pick Maces, and so on, including scrolls, jewels, potions and pets. The category rail is redesigned to carry this
without crowding the wallet cards.

Success looks like this:

1. Every category with sub-categories opens a flyout beside the rail. Picking a leaf filters the table to exactly those
   items, across pages.
2. Items stop landing in the wrong category: skill orbs and scrolls (currently under Wings and Miscellaneous) have their
   own category, and bundled jewels sit with the jewels.
3. The rail has no `>` arrow; each row carries a small icon, and the selected row shows the picked leaf's name.
4. The wallet cards, table height and everything else on Browse are unchanged (the wallet may shift down one pixel).

## Non-goals

Per-category listing counts, a class filter (the packet already has `ClassMask`), a hand-editable taxonomy file, and any
change to Sell, My Listings or Mailbox. Existing English-literal UI strings are not localised here (the 11 `Game.*.resx`
files are a separate pass).

## What exists today

- The server classifies a listing once, when it is created (`AuctionCategoryClassifier.Classify(group, number)`), stores
  the result in `AuctionListing.Category` and filters browse on that column. Seven values: Weapon 0, Armor 1, Wing 2,
  PetHelper 3, JewelMaterial 4, Consumable 5, Miscellaneous 6.
- Each listing already stores `ItemGroup` and `ItemNumber`, so no information is lost and old rows can be re-classified.
- The browse request (`AuctionBrowseRequest`, code D5 sub 01, length 104) carries one `Category` byte; 255 means all.
- Weapons and armor split cleanly by item group (0 to 5 and 6 to 11). Groups 12 to 15 do not: in the owner's Item.txt,
  group 12 mixes about 140 wing and cape variants with skill orbs, scrolls, seeds, spheres, boxes and bundled jewels
  (the classifier calls all of it Wing except one jewel), group 15 holds only skill scrolls (classified Miscellaneous),
  and groups 13 and 14 hold pets, rings, pendants, mounts, seals, tickets, potions, elixirs, cards and boxes.
- The client rail (`RenderCategoryRail` in `NewUIAuctionWindow.cpp`) is eight 19-pixel rows with a `>` glyph; the wallet
  cards sit directly under it and end where the table ends.
- The client's packet bindings are generated from the `MUnique.OpenMU.Network.Packets` NuGet package, which for this
  branch is packed by hand into the local feed `C:\Users\rause\.local-nuget\auction-house` (currently 0.9.11, referenced
  by `ClientLibrary/MUnique.Client.Library.csproj`).

## Taxonomy

One table on the server maps `(group, number)` to `(category, sub-category)`. Sub-category values are per category; 255
means "all of this category". Rules are checked in order: exact pairs first, then number lists and ranges, then the
group default. Numbers below come from the owner's Item.txt; a number not listed falls to its group's default, which
keeps a stock Season 6 item set working.

| Category (wire) | Sub-categories (value) and rules |
| --- | --- |
| Weapon (0) | Swords 0 = group 0; Axes 1 = group 1; Maces & Scepters 2 = group 2; Spears 3 = group 3; Bows & Crossbows 4 = group 4 (ammunition included); Staves & Books 5 = group 5 |
| Armor (1) | Helms 0 = group 7; Armors 1 = group 8; Pants 2 = group 9; Gloves 3 = group 10; Boots 4 = group 11; Shields 5 = group 6 |
| Wing (2) | Wings 0 = group 12 default; Capes & Cloaks 1 = group 12 numbers 40, 49, 50, 130, 135, 154, 156, 161, 163, 208, 210, 211, 234, 240, 242, plus the exact pair (13, 30) |
| Pet & Helper (3) | Pets 0 = group 13 numbers 0, 1, 4, 5, 64, 65, 67, 80, 106, 123, 145; Mounts 1 = 2, 3, 37, 400 to 424; Rings 2 = 8, 9, 10, 20 to 24, 39 to 42, 68, 76, 107, 109 to 112, 122; Pendants 3 = 12, 13, 25 to 28, 38, 113 to 115; Eggs 4 = 200 to 276; Seals & Tickets 5 = group 13 default |
| Jewel & Material (4) | Jewels 0 = the existing exact pairs (12,15) (14,13) (14,14) (14,16) (14,22) (14,31) (14,42), plus group 14 numbers 160, 161, 244 to 259; Bundles 1 = group 12 numbers 30, 31, 136 to 143; Refining stones 2 = group 14 numbers 41, 43, 44; Seeds & Spheres 3 = group 12 numbers 60 to 65, 70 to 74, 100 to 129; Fruits 4 = (13, 15) and group 13 numbers 54 to 58 |
| Consumable (5) | Potions 0 = group 14 numbers 0 to 8, 35 to 40, 70, 71, 94, 133; Elixirs & Buffs 1 = group 14 numbers 72 to 82, 97, 98, 140; Boxes & Misc 2 = group 14 default |
| Miscellaneous (6) | No sub-categories. Everything not matched above, including group 12 number 26 and groups 16 and up |
| Scrolls & Orbs (7, new) | Orbs & Crystals 0 = group 12 numbers 7 to 14, 16 to 19, 44 to 47; Scrolls & Parchments 1 = group 12 numbers 21 to 24, 35, 48, and all of group 15 |

Exact pairs and number lists are evaluated before group defaults, so a jewel inside group 12 or 14 is never mistaken for a
wing or a potion. The existing wire values 0 to 6 are unchanged; Scrolls & Orbs is appended as 7.

## Protocol

- `AuctionBrowseRequest` gets one new field, `SubCategory` (Byte), appended after `Name` at index 104, so the length
  becomes 105. 255 means "every sub-category". A sub-category sent with Category 255 is ignored.
- `AuctionCategory` in `CommonEnums.xml` gains `ScrollOrb = 7`.
- Browse and My Listings response rows are unchanged: the client does not display a sub-category.
- Packaging: after the XML change the server project is packed as `0.9.12` into the local feed, the client's
  `OpenMuPacketsVersion` is raised to 0.9.12, and the client bindings (`PacketBindings_ClientToServer.h`,
  `PacketFunctions_ClientToServer.cpp` and the managed `ConnectionManager.*Functions.cs`) are regenerated by the build.
  `SendAuctionBrowseRequest` gains a trailing `BYTE subCategory`. Client and server must be updated together.

## Server

- `AuctionListing` (DataModel) gets `byte SubCategory`; a new EF Core migration adds the column with default 255
  ("unclassified"). The generated persistence classes are regenerated by the build.
- `AuctionCategoryClassifier` returns both values (a small record) from the taxonomy table. `ListItemAsync` stores both
  when a listing is created.
- The browse filter becomes `Category == c` and, when the request names one, `SubCategory == s`
  (`AuctionListingSearch` gains `byte? SubCategory`; `AuctionBrowseHandlerPlugIn` maps 255 to null).
- A new repository method, `ReclassifyListingsAsync(batchSize)`, recomputes `Category` and `SubCategory` from the stored
  `ItemGroup` and `ItemNumber` for rows whose `SubCategory` is 255, in batches. The existing maintenance plug-in
  (`AuctionExpiryPlugIn`) calls it each tick until no rows remain, the same pattern as
  `RepairMissingReturnCollectionsAsync`. It is idempotent and touches only the connected database, which is
  `openmu_auction_test`. Until it finishes, unclassified rows simply do not match a sub-category filter.

## Client

- **Taxonomy mirror.** A pure header `AuctionTaxonomy` (no engine dependencies, unit-tested) lists the eight categories
  and their leaves: wire ids, English labels (literals for now) and the rail icon item. It has no item rules; the server
  owns classification.
- **Rail.** Nine rows (All Categories plus eight) at a 17-pixel pitch, so the rail takes 153 pixels against the
  152 of the current eight rows at 19, and the wallet cards move down by one pixel at most. The `>` is removed. Each row has a 14-pixel icon, a
  representative item drawn with `RenderItem3DFittedToSlot` like the wallet icons: Weapon (0,16) Sword of Destruction,
  Armor (8,8) Brass Armor, Wing (12,5) Wings of Dragon, Pet & Helper (13,4) Dark Horse, Jewel & Material (14,13) Jewel of
  Bless, Consumable (14,3) Large Healing Potion, Scrolls & Orbs (15,3) Scroll of Fire Ball, Miscellaneous (14,11) Box of
  Luck. All Categories has no icon. Each icon is verified to exist in `item.bmd` before it is used. The selected row keeps
  the gold tint, and shows the leaf's name instead of the category's when a leaf is picked; long names are cut with an
  ellipsis by measuring the text.
- **Flyout.** Clicking a category with leaves opens a panel whose left edge meets the rail and whose top aligns with the
  clicked row (moved up if it would run past the table's bottom). Rows use the rail's styling: "All <category>" first, then
  the leaves. The panel is 132 pixels wide, which fits "Bows & Crossbows" and "Elixirs & Buffs". Picking a row selects it,
  closes the flyout, resets to page 1 and requests the page. Clicking anywhere else, or pressing Escape, closes it without
  changing the selection. Clicking a category without leaves (Miscellaneous, All Categories) selects it directly.
- **Reset** clears both the category and the leaf.
- **Interaction rules.** While the flyout is open it owns the mouse (like an open combo): the table rows beneath it are not
  hovered and show no tooltip, and their 3D item icons are skipped, by adding the flyout rectangle to the window's
  `IsCoveredByOpenDropdown`.
- **Request.** `SendBrowseRequest` sends the category byte as before (index 0 maps to 255, otherwise index minus one, with
  Scrolls & Orbs at 7) and the leaf's wire id, or 255.
- **State.** `m_SelectedCategoryIndex` is kept; `m_SelectedSubCategory` (255 = all) is added. Changing the category resets
  the leaf to 255.

## Testing

- Server (xUnit, in `MUnique.OpenMU.Tests/AuctionHouse`): every rule in the table, including exact pairs overriding group
  defaults and unknown numbers falling to the group default; a coverage test over a trimmed `(group, number, name)` fixture
  taken from Item.txt asserting each entry classifies and printing the ones that fall to a default; browse handler maps
  255 to no filter; the repository search and `ReclassifyListingsAsync` tests join the existing PostgreSQL tests (skipped
  without `AUCTION_TEST_CONNECTION_STRING`).
- Client (doctest, `tests/ui`): taxonomy integrity (leaf ids unique per category, labels non-empty, every icon pair
  distinct), the flyout layout (position, clamping to the table's bottom, row hit-testing), and the row-label fitting.
- In game, by the owner: the rail at 9 rows, flyout open and close paths, a leaf filter across two pages, Reset, and that no
  table icon shows through the open flyout.

## Risks

- The packet change needs the repack and version bump to stay in step on both sides; a mismatch makes the server drop
  browse requests (it already rejects packets shorter than the declared length).
- The number lists come from one Item.txt. Items the lists miss land in a group default; the coverage test shows how many.
- Nine rows at 17 pixels is tighter than the current 19. The nine-slice rows still fit their text (8 pixels tall); this is
  checked in game.
