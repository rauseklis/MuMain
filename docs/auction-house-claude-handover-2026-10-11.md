# Auction House handover — 2026-10-11

## Scope and repositories

The Auction House work is split across two repositories and two matching worktrees. Both use branch `codex/auction-house`.

| Area | Worktree | Remote |
| --- | --- | --- |
| Client UI | `client/.worktrees/auction-house` | `https://github.com/rauseklis/MuMain.git` |
| Server browse protocol and persistence | `server/.worktrees/auction-house` | `https://github.com/rauseklis/OpenMU.git` |

Use the test database `openmu_auction_test` only. Do not touch a production database. The server has an unrelated pre-existing modification at `src/Web/Map/wwwroot/js/MUnique.OpenMU.Web.Map.js`; leave it unmodified and out of commits.

## Current state

The browse screen has 50 seeded demo listings across weapons, armour, wings, helpers, jewels/materials and consumables, with levels spread from `+0` through `+15`. The client correctly receives Excellent and Ancient flags from auction rows even when the option configuration is unavailable.

The outstanding visual problem is icon fitting. Screenshots from 2026-10-11 show that most previews are now anchored in the correct cell, but a few 3D mesh silhouettes (notably bows, scythe-like weapons and some wings) still cross the slot outline. Do not try more item-group scale constants as the primary solution; earlier group-based values repeatedly produced inconsistent results.

## Rendering investigation already completed

The ordinary inventory is the authoritative implementation:

- `src/source/UI/NewUI/Inventory/NewUIInventoryCtrl.cpp`, `CNewUIInventoryCtrl::Render3D`, renders each item using `ItemAttribute[Type].Width/Height * 20` logical inventory cells.
- `src/source/Engine/Object/ZzzInventory.cpp`, `RenderItem3D`, resolves the item's authored inventory anchor through `Render::Items::Display::GetInventoryAnchor(Type, Level)`.
- Item display metadata is loaded from `Data/Items/Models` via `src/source/Render/Items/ItemDisplay.cpp` and `ItemModelDatabase`.
- `RenderObjectScreen` applies each model's authored rotation, offset and scale. A fixed UI rectangle alone does **not** resize the model.

The Auction House currently uses `RenderItem3DFittedToSlot` in `ZzzInventory.cpp`. It calculates an inventory-footprint scale from `ItemAttribute` and calls `RenderItem3DScaled` with the original slot rectangle so the authored anchor is not cancelled. The Auction list passes a `0.68f` fill factor at `NewUIAuctionWindow.cpp` in `Render3D`.

This is an improvement over the removed geometric-centre approach, but it is still an estimate: certain meshes extend beyond their declared inventory footprint because their true rendered silhouette and glow exceed their geometry metadata.

## Required deep analysis before changing icon fitting

1. Trace the full model pipeline for a preview: `RenderItem3D` → `RenderObjectScreen` → `RenderPartObject`/`BMD`. Identify where a model's final transformed bounds can be measured after its rotation, animation and item effects have been applied.
2. Implement a reusable compact-preview API that fits the **projected mesh bounds**, not only `ItemAttribute` grid dimensions, into a supplied rectangle with a small configurable inset. It must preserve native item model rotation and anchor behaviour.
3. Validate against a representative matrix: one-handed and two-handed weapons, bows, staves, scythes, helm/armour/pants/gloves/boots, normal and excellent items, ancient items, jewels, every wing type and helpers.
4. Treat clipping only as a final safety guard. A scissor rectangle may be possible through `mu::GetRenderer().SetScissor`, but it must use the correct logical-to-window conversion and render-command ordering. Do not use it as the visual-fit algorithm: cropped item tips are not acceptable.
5. Avoid global state that changes all item renders. The implementation must be scoped to the supplied compact preview call.

The success criterion is straightforward: every preview remains wholly inside the icon container, has a consistent inset, and stays recognisable. No glow, wing, blade or bow may cross the outline.

## Icon container redesign

The current slot is intentionally minimal and should be redesigned after preview fitting is reliable:

- Rendering function: `RenderRoundedAuctionIconSlot` in `src/source/UI/NewUI/AuctionHouse/NewUIAuctionWindow.cpp`.
- Current logical size: 27 px (`BROWSE_ICON_CELL_SIZE`); inner preview: 23 px (`BROWSE_ICON_SIZE`).
- Keep the black fill and a thin rarity-tinted border, but create a professional rounded container: consistent anti-aliased-looking corners at the game's scale, a restrained inner bevel/shadow, and no heavy fluorescent outline.
- The container must not depend on hover state for basic legibility, and every state must work with the black table background.
- Preserve tooltip hit-testing: only the icon container should trigger the item tooltip.

## Relevant server work

The server change is in the matching server worktree:

- `src/Persistence/AuctionHouse/AuctionSearchTypes.cs`: `AuctionListingRow.OptionFlags`.
- `src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs`: projects `OptionFlags` and includes linked item sets when loading listed items.
- `src/GameServer/MessageHandler/AuctionHouse/AuctionBrowseHandlerPlugIn.cs`: applies the immutable listing option snapshot to extended serialized item bytes. This keeps Excellent and Ancient visual flags intact if live option configuration is absent or changes.

The server project has been built successfully with:

```powershell
dotnet build src\GameServer\MUnique.OpenMU.GameServer.csproj -c Release --no-restore --verbosity:quiet -m:1
```

The client build needs the Visual Studio developer command prompt because a plain PowerShell build does not provide the Windows SDK headers:

```powershell
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build out\build\windows-x86 --config Debug --target Main
```

Always stop `Main.exe` before linking a new client build; Windows locks the executable while it is running.

## Suggested first action for the next engineer

Read `AGENTS.md` and `docs/CODING_RULES.md`, then inspect the files named above without changing layout constants. Produce a short design note explaining where final mesh bounds are available and how the compact-preview API will measure, scale and position them. Only then implement the renderer and verify it visually with screenshots of both auction browse pages.
