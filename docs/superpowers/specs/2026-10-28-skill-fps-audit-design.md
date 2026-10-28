# Skill animation and high-FPS audit — design

Client-side investigation and repair for every playable character skill, with
the five reported regressions handled first: Twisting Slash, Fire Slash, Fire
Burst, Scroll of Flame, and Ice Storm.

## Intent

Restore the Season 6 skill presentation and gameplay footprint at every
supported frame rate. The original 25 FPS result is the behavioural baseline;
higher frame rates may add temporal smoothness, but must not change animation
duration, effect coverage, spawn count per second, hit timing, targeting, or
the apparent relationship between an effect and its damage area.

This is an audit, not a visual redesign. Existing intentional art changes such
as Scroll of Flame's violet colour remain unless they prevent the original
skill behaviour from being restored.

## Sources of truth

Use these in descending order:

1. The last known-good behaviour reported by the players, reproduced in the
   installed client where possible.
2. This repository before the relevant high-FPS or skill-specific change.
3. The upstream `sven-n/MuMain` Season 5.2/Season 6 implementation.
4. Server skill definitions and range/targeting rules when a client-side range
   request is involved.

Upstream code is evidence for the intended mechanics, not permission to copy a
newer unrelated redesign into this fork.

## Confirmed implementation history

- Twisting Slash creates `MODEL_SKILL_WHEEL1`, which drives the orbiting
  `MODEL_SKILL_WHEEL2` weapon effect. Commit `ef07f1f3` changed the wheel-one
  driver to create one subtype-zero wheel and terminate immediately. Upstream
  instead emits the five staged wheel subtypes over the driver's lifetime.
- Commit `4bce3df6` separated Twisting Slash orbit phase from the rendered
  weapon angle and rewrote Fire Burst's `MODEL_PIER_PART` trail: one trail child
  per update, a capped homing subdivision count, and a fixed child lifetime.
- Commit `7737c0a6` replaced Scroll of Flame's `BITMAP_FLAME` with a separate
  violet `BITMAP_FLAME_VOID` effect path. The new path still contains the same
  nominal client range call (`AttackCharacterRange`, radius 150), so the
  reported hit regression is not yet explained by a simple radius constant.
- Ice Storm's `MODEL_BLIZZARD` movement, gravity, light growth, impact spawn,
  and random emission were converted to high-FPS helpers/factors. Several
  decisions still use integer lifetime equality or per-update random values.
- Fire Slash uses the shared wheel character animation and a
  `BITMAP_SWORD_FORCE`/joint effect. Its attack trigger is tied to character
  animation-frame thresholds, while its effect creation contains an exact
  first-lifetime check.

These are investigation leads. A change is not called a root cause until a
regression test or a controlled 25/high-FPS comparison demonstrates it.

## Required behaviour for the five reported skills

### Twisting Slash

- Preserve the original full circular slash presentation, including its
  staged/fading orbit trail and weapon-dependent radius.
- The orbit completes in the same wall-clock time at 25 FPS and high FPS.
- Repeated casts do not leave gaps, duplicate opaque blades, or alter the
  character attack animation.
- Base, Strengthener, Magic Gladiator, and Mastery variants share the repaired
  behaviour unless their data explicitly differs.

### Fire Slash

- Play the complete wheel attack animation and sword-force arc without early
  truncation or repeated first-frame bursts.
- The effect trigger, sound, and attack request occur once at the intended
  point in the animation at 25 FPS and high FPS.
- Strengthener behaviour remains distinct where its joint subtype differs.

### Fire Burst

- Each of the three projectiles follows its curved route to the target and
  carries a continuous, bounded trail rather than a chopped line, late clump,
  or detached ring.
- Projectile travel time, trail duration, and density are stable in wall-clock
  terms at 25 FPS and high FPS.
- Base, Strengthener, and Mastery variants behave consistently.

### Scroll of Flame

- Keep the violet flame identity while restoring the same target selection,
  area coverage, cadence, and cast location as the pre-colour skill.
- Measure visual radius and client attack radius separately. If server damage
  targeting is authoritative, verify it on an isolated throwaway server and
  database; never infer a server change from visuals alone.
- A colour-only effect substitution must not change owner, packet serial,
  lifetime boundary crossings, skill identifier, or calls to
  `AttackCharacterRange`.

### Ice Storm

- Preserve the original number and distribution of falling ice objects, their
  descent/impact timing, impact effects, sound, and fade.
- Random acceleration/emission must have equivalent distributions per second,
  not per rendered frame.
- Exact lifetime gates must not be skipped when a fractional high-FPS lifetime
  crosses them.

## Full-skill audit scope

Inventory every player-cast skill and master-skill variant across Dark Wizard,
Dark Knight, Fairy Elf, Magic Gladiator, Dark Lord, Summoner, and Rage Fighter.
For each skill, record:

- cast/character action and animation threshold;
- primary effect, child effects, particles, joints, sounds, and terrain light;
- lifetime and fade rules;
- movement, gravity, rotation, scale, and trail sampling;
- random spawn rules and whether they are rate-corrected;
- target selection, range checks, and client attack callbacks;
- 25 FPS result, high-FPS result, automated coverage, and manual status.

Master variants that deliberately reuse a base implementation can share one
row, but their identifiers must be named so they cannot be silently omitted.

## Audit rules

Flag code in a skill path when it has any of these properties:

- per-update movement, fade, rotation, gravity, scale, or animation changes
  without a time factor;
- multiplication by `FPS_ANIMATION_FACTOR` during one-time construction;
- per-update randomness whose probability or magnitude changes with FPS;
- integer modulo/equality gates on a fractional lifetime;
- thresholds that can be crossed between updates without a crossing test;
- child effects spawned inside an FPS-dependent or growing subdivision loop;
- an effect lifetime measured in updates while its motion is time-scaled;
- hit/range callbacks attached to a replacement visual effect without a
  parity test;
- repeated sound or network/gameplay callbacks that should fire once.

Not every flag is a bug. The audit must explain why each retained construct is
safe or cover it with a regression test.

## Test design

Add small pure helpers where necessary so timing can be tested without running
the renderer. Tests compare deterministic simulations at 25, 60, 120, and 240
FPS over the same wall-clock duration.

Required regression coverage:

- event/lifetime threshold crossing fires once even with fractional steps;
- distance, orbit angle, scale, and alpha end within an explicit tolerance;
- rate-controlled emission produces the intended count/distribution per
  reference tick;
- staged effects retain their canonical subtype sequence and lifetimes;
- Scroll of Flame preserves skill, owner, packet serial, attack radius, and
  attack cadence when its texture/effect type changes;
- the skill inventory test fails when a known player skill has no audit row.

For each reported bug, first add a test that fails on current `main`, record the
cause it demonstrates, then implement the smallest repair.

## Manual validation

The owner performs gameplay checks; automation must not control the game
window. Build and install the Windows x64 client with the repository scripts.
Use `$fps 25`, `$fps 60`, `$fps 120`, and `$fps 240` with VSync off where the
display permits it. Record video or side-by-side observations for:

- single casts and continuous casts;
- self and other-player views where relevant;
- stationary, moving, mounted, and high attack-speed cases;
- clustered monsters at the centre and edge of area skills;
- every class's audit checklist.

Any test that needs damage or hit confirmation uses a separately configured
throwaway OpenMU server and disposable database on shifted ports. Visual-only
checks can use the installed live client, but do not automate input or login.

## Delivery

Keep the work reviewable in focused pull requests:

1. audit framework/tests and the five reported skills;
2. remaining shared effect/timing primitives found by the audit;
3. class-grouped skill fixes and audit-matrix completion.

Every pull request states the demonstrated root causes, automated tests,
Windows build result, manual checks completed by the owner, and checks still
outstanding. A client build is installed only after its change is tested and
merged.

## Non-goals

- New skill art, balance changes, damage/range redesign, or server packet
  changes without evidence that the original contract is broken.
- Linux or macOS client work.
- Automated control of the game window or use of the owner's real local server
  or `openmu` database.
- Broad rendering clean-up unrelated to a skill regression.
