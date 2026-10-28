# Skill visual follow-up design

## Evidence and causes

- Scroll of Flame renders `BITMAP_FLAME_VOID` through the terrain decal path. The terrain renderer samples beyond the texture and clamps its non-black JPEG edge pixels, producing the large coloured square visible in the supplied screenshot.
- Flame has `MinimumNumberOfHitsPerTarget = 0`, `MaximumNumberOfHitsPerTarget = 2`, and a `0.5^distance` pre-attack probability. Both rounds are therefore optional even for targets inside the configured 1.5-tile radius. A zero-damage server result is displayed as `Miss`, which confirms selection but not damage.
- Fire Burst uses `CreateEffectFpsChecked` for a trail segment that was deterministic at the legacy 25 Hz rate. Bernoulli sampling keeps only the average count and leaves visible random gaps.
- Twisting Slash uses the same angle for its orbit and render-time weapon spin. Its legacy 150/180-unit orbit is also visually too large for the current client camera.

## Changes

### Client

1. Render Scroll of Flame's ground footprint with the proven radial light texture and violet light, while retaining `BITMAP_FLAME_VOID` for its particles.
2. Drive Fire Burst trail creation from a per-effect legacy-tick accumulator and create due trail segments directly instead of randomly discarding them.
3. Store Twisting Slash's orbit phase separately from the weapon model angle and reduce the non-spear/spear radii by 25 percent (150/180 to 112.5/135 world units).

### Server

1. Set Flame and its master replacements to at least one guaranteed attack round per target.
2. Add a mandatory configuration update for existing databases and make fresh Season 6 initialization match it.

## Verification

- Add failing source-contract and timing tests before the client changes.
- Add failing update and fresh-initialization tests before the server changes.
- Run focused tests, a Windows x64 production build, and a throwaway in-memory server startup.
- Merge server first and verify the live deployment, then merge and install the client build.

