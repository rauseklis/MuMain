# Royal Mace of King — inventory flame preview: handoff

Instructions for a Claude session that continues this work. Read `AGENTS.md` and
`docs/CODING_RULES.md` first; they apply to every change.

## Goal

The Royal Mace of King (item 2:23) shows fire around its head when equipped at +7 or
higher. The same fire must also show in inventory, hover and picked-up item previews.
The normal Mace of King (2:22) never has flames.

## Why previews had no flames

In-world flames are particles created by `RoyalFlames::Emit`
(`src/source/Render/Items/RoyalMaceFlames.cpp`), called from `RenderLinkObject` in
`src/source/Engine/Object/ZzzCharacter.cpp`. Inventory icons are drawn by
`RenderObjectScreen` in `src/source/Engine/Object/ZzzInventory.cpp`, a separate pass that
never runs the particle system, so no particle could appear there.

## What was done (branch `claude/wizardly-wright-e0x487`)

- `RoyalFlames::RenderPreview(model, modelType, level, translate, bones)` draws the fire
  texture (`BITMAP_TRUE_FIRE`) directly as sprites at the same head-mesh emitter vertices.
  It replays the in-world particle: 20-tick life, rises 1 unit per tick, scale shrinks
  0.02 per tick, brightness is life / 25. Five overlapping sprites per emitter.
- It is called at the end of `RenderObjectScreen`, after `RenderPartObject`.
- The profile (emitter count and scale per level) is shared with the world path via
  `RoyalMaceFlameProfile.h`: none below +7, then 1, 2 and 3 emitters from +7, +9 and +13.
- Size and rise are multiplied by the model's `BodyScale`, because previews are drawn
  smaller than the in-world weapon.

## Status: not yet verified

Nothing has been compiled or seen in game. The container used so far was Linux without the
Windows x86 MSVC toolchain. The first job is to build and look.

Build (VS x86 Developer PowerShell, `VCPKG_ROOT` set, from the repo root):

```powershell
$env:CMAKE_BUILD_PARALLEL_LEVEL = '2'
cmake --preset windows-x86 -DBUILD_TESTING=ON
cmake --build out/build/windows-x86 --config Release
ctest --test-dir out/build/windows-x86 -C Release -E bundled_font_assets --output-on-failure
```

## Manual checks

Use a Royal Mace at +0, +6, +7, +9, +13, +15 and a normal Mace of King.

1. Flames show in the inventory, on hover (item rotating) and while held on the cursor.
2. Nothing below +7; one, two and three flames at +7, +9 and +13; larger toward +15.
3. The normal Mace of King and other maces show no flames.
4. Flames sit at the mace head, are not hidden behind the model, and do not overflow the
   slot or draw over neighbouring items or windows.
5. Vault, multi-vault, trade and shop previews: they go through the same function, so
   check they behave and nothing is left in a bad render state afterward.
6. Ground items and in-world equipped flames are unchanged.

## Likely adjustments

All tuning constants are at the top of `RoyalMaceFlames.cpp`.

- Too small or large: change how `BodyScale` is applied to `size` in `RenderPreview`.
- Clipped by depth: sprites are drawn with depth state as `RenderPartObject` left it.
  Look at the item depth bands (`Preview::ItemDepthFraction`, `PickedItemDepthFraction`)
  before adding any depth-state change, and restore whatever state is changed.
- Flame rises the wrong way for a tilted item: the rise is along world Z. Use the camera
  up vector if it looks wrong.
- Flicker speed: `MillisecondsPerTick`.

## Rules to keep

- Do not add flames to anything except item 2:23 at +7 or higher.
- Keep the profile in `RoyalMaceFlameProfile.h` the single source for counts and scale;
  `tests/render/test_royal_mace_flames.cpp` covers it. Update it if the rules change.
- Do not touch drop settings, accounts or databases.
- Do not create a PR unless the user asks.

## Server side (read only, `rauseklis/OpenMU`)

Reviewed by reading code at commit `d1f63e0`; the .NET tests were not run. It matches the
release handover:

- Damage 110–125 (Mace) and 150–170 (Royal); weapon speed 30 and 40.
- 1x3 one-handed, for DK, MG and DL class lines; drops disabled.
- +4 min and max damage per upgrade level, through +15.
- STR and AGI coefficients 480/334 and 968/675; additional damage +4 adds 5 STR.
- Configuration updates 123 (`AddKingMaces`) and 124 (`KingMaceRequirements`) in
  `src/Persistence/Initialization/Updates/UpdateVersion.cs`.

Files: `Persistence/Initialization/VersionSeasonSix/Items/KingMaces.cs`,
`KingMaceRequirements.cs`, `GameLogic/ItemExtensions.cs`, `tests/MUnique.OpenMU.Tests/KingMacesTest.cs`.
To verify, run those tests with .NET 10 on a machine that has it.
