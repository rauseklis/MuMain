# Skill Engine Repair Design

## Problem

The first FPS skill patch verified helper functions and source patterns, but it did not verify complete rendered skill sequences. Several effects still disappear, lose choreography, become asymmetric, or generate invalid state above the legacy 25 Hz update rate.

The reported failures are Fire Slash, Twisting Slash, Fire Burst, Scroll of Flame, Evil Spirit, Rageful Blow, and Ice Storm. The same classes of defect can affect every skill which depends on exact animation frames, exact floating-point lifetimes, probabilistic one-shot creation, or per-render random motion.

## Reference behavior

Use the original `sven-n/MuMain` implementation as the canonical choreography and asset mapping. Use `papontos/mymu-client` only as a comparison for high-FPS adaptations, and `bernatvadell/muonline` as an independent description of a skill's intended shape. Do not replace original choreography merely to make timing easier.

For server-authoritative damage, use `MUnique/OpenMU` and the original client's world-unit radius as references. The legacy Scroll of Flame range is 150 world units, equal to 1.5 map tiles; OpenMU therefore needs a target-area diameter of 3 tiles, not 2.

## Root defect classes

1. One-shot events use `Create*FpsChecked`, so the only visible event can be randomly discarded.
2. Exact float equality or integer lifetime checks can repeat or skip milestones when lifetime advances by a fractional FPS factor.
3. `pow` is used to scale sign changes; a negative base with a fractional exponent produces non-finite state.
4. Random steering is sampled once per rendered frame while damping is not scaled consistently, changing trajectories and symmetry with FPS.
5. Earlier custom edits changed the number, lifetime, and placement of Twisting Slash and Fire Burst children instead of preserving the canonical sequence.
6. Source-pattern tests cannot prove effect count, order, trajectory, finite state, or left/right balance.

## Client design

Introduce small, pure timing/trajectory helpers which can be tested without the renderer:

- milestone crossing for descending lifetimes, with one-shot state stored on the effect;
- reference-tick accumulation for continuous legacy emissions;
- finite Rageful Blow trajectory calculation with an explicit phase transition;
- deterministic cardinal-angle generation and reference-tick steering for Evil Spirit;
- canonical Twisting Slash five-stage emission and canonical Fire Burst trail density/lifetimes.

Restore the original visual sequence first, then make only its time progression frame-rate independent. One-shot child effects must use unconditional creation once their guarded milestone is reached. Continuous particles may remain rate-limited.

Fire Slash must unconditionally create its sword-force joint once. Rageful Blow must preserve the rise, return, catch, and ground-impact sequence without negative-base exponentiation. Evil Spirit must retain four 90-degree directions and the original motion/blend geometry while applying colour independently. Ice Storm remains covered by the same milestone and reference-tick rules.

## Server design

Change Scroll of Flame's `TargetAreaDiameter` from 2 to 3 for new configurations and add an update plug-in for existing databases. The update must be idempotent and limited to Flame and its relevant master-skill variants if they own independent area settings.

Verify the effective 1.5-tile radius with a focused unit/integration test and a throwaway server/database. Never use the local database named `openmu`.

## Tests

Client tests run the pure sequence model at 25, 60, 120, and 240 FPS and assert:

- identical one-shot milestone counts;
- canonical Twisting Slash stage order and count;
- Fire Slash always emits its joint;
- Fire Burst emits a bounded, continuous canonical trail;
- Rageful Blow remains finite and completes both rise and return phases;
- Evil Spirit starts at four cardinal angles and has balanced left/right travel;
- Ice Storm milestones occur once.

Add a structural audit test which rejects probabilistic creation inside known one-shot skill milestones and catalogs remaining exact-frame/lifetime comparisons for follow-up. This test is a guardrail, not a substitute for sequence tests.

Server tests assert the configured Flame diameter and boundary inclusion/exclusion around a 1.5-tile radius.

## Delivery

The client and server changes use separate pull requests. No packet format changes are planned. Test both before merging. Merge the server first, watch the live deployment and container health, then merge the client, build Windows x64, and update the live client folder only after checking its installed-build marker.
