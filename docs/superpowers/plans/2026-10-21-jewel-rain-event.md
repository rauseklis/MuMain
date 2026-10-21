# Jewel Rain Event Implementation Plan

**Status: done** - `rauseklis/OpenMU` PR #7. Kept as the record of how it was built.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A scheduled, fully configurable server event that announces a countdown and rains batches of items onto Devias town.

**Architecture:** A pure, clock-driven state machine (`JewelRainScheduler`) turns "now" into actions (countdown, start, drop step N, end). A thin plug-in (`JewelRainPlugIn`, hooked to the existing per-second `IPeriodicTaskPlugIn`) executes those actions: golden messages to all players and ground-item drops on the map. Config is an `ISupportCustomConfiguration` class so it appears in the Admin Panel.

**Tech Stack:** C# / .NET 10, NUnit, OpenMU `GameLogic` plug-in system.

**Spec:** `client/docs/superpowers/specs/2026-10-21-jewel-rain-event-design.md`

**Work location:** server repo worktree `mu-online-openmu/server/.worktrees/jewel-rain` (branch `feature/jewel-rain`). All paths below are relative to its root.

## Global Constraints

- Days Wed/Fri/Sun, start 21:00, server time zone (Latvia), countdown 5,4,3,2,1 minutes.
- Map Devias = 2. Step interval 30 s; end announcement 30 s after last drop; first step drops at start.
- Default steps: Chaos (group 12, number 15), Soul (14/14), Bless (14/13), Creation (14/22), Life (14/16): 15 pieces each, as 15 separate ground items. (Chaos is group 12, not 14; the ids were checked against the item initialization in Task 6.)
- `owners: null` on `DroppedItem` (free for all). Messages: `MessageType.GoldenCenter`.
- Code style per surrounding code: StyleCop file headers, XML docs on public members, `ConfigureAwait(false)`, `[PlugIn]` + `[Guid]` + `[Display]`.

## Review Focus

- Server restarts inside the countdown window: past marks are skipped, never replayed.
- Server stalls past the start by more than 5 s: the run is skipped as missed, not started late.
- Countdown that crosses midnight (start 00:02): still announced correctly.
- A start time arrives while a run is active: skipped, no double start.
- Empty step list or unknown map/item: event does not run, logs a warning, never throws.
- Config edited during a run: the run keeps its snapshot; the next run uses the new values.

---

### Task 1: Configuration model

**Files:**
- Create: `src/GameLogic/PlugIns/JewelRain/JewelRainConfiguration.cs`
- Create: `src/GameLogic/PlugIns/JewelRain/JewelRainStep.cs`
- Test: `tests/MUnique.OpenMU.Tests/JewelRain/JewelRainConfigurationTest.cs`

**Interfaces:**
- Produces: `JewelRainStep { byte Group; short Number; byte Level; ushort Amount }`.
- Produces: `JewelRainConfiguration` with `bool Enabled`, `IList<DayOfWeek> Days`, `IList<TimeOnly> StartTimes`, `IList<int> CountdownMinutes`, `ushort MapNumber`, `byte AreaX1, AreaY1, AreaX2, AreaY2`, `TimeSpan StepInterval`, `TimeSpan EndDelay`, `IList<JewelRainStep> Steps`, `LocalizedString CountdownMessage` (with `{minutes}`), `LocalizedString StartMessage`, `LocalizedString EndMessage`, and `static JewelRainConfiguration Default`.

- [ ] **Step 1: Failing test** `DefaultMatchesTheDesign`: asserts Days == [Wednesday, Friday, Sunday], StartTimes == [21:00], CountdownMinutes == [5,4,3,2,1], MapNumber == 2, StepInterval == 30 s, EndDelay == 30 s, Steps.Count == 5 and every step `Amount == 15`.
- [ ] **Step 2: Run** `dotnet test tests/MUnique.OpenMU.Tests --filter JewelRainConfigurationTest` → fails to compile.
- [ ] **Step 3: Implement** both classes; `Default` returns the values above; message defaults: countdown "Jewel Rain starts in {minutes} minute(s) in Devias!", start "Jewel Rain has begun in Devias! Run for the jewels!", end "Jewel Rain has ended - no more drops.". `[Display(Name=..., Order=...)]` on every property (plain strings, like `PeriodicInvasionConfiguration`).
- [ ] **Step 4: Run** the filter → PASS.
- [ ] **Step 5: Commit** `feat(jewel-rain): configuration model`.

### Task 2: Scheduler state machine (pure logic)

**Files:**
- Create: `src/GameLogic/PlugIns/JewelRain/JewelRainScheduler.cs` (also holds `JewelRainAction`, `JewelRainActionKind`)
- Test: `tests/MUnique.OpenMU.Tests/JewelRain/JewelRainSchedulerTest.cs`

**Interfaces:**
- Consumes: `JewelRainConfiguration` (Task 1).
- Produces: `enum JewelRainActionKind { Countdown, Start, Drop, End, Missed }`; `readonly record struct JewelRainAction(JewelRainActionKind Kind, int Value)` (Value = minutes for Countdown, step index for Drop, 0 otherwise); `sealed class JewelRainScheduler { IReadOnlyList<JewelRainAction> Advance(JewelRainConfiguration config, DateTime nowLocal); void RequestForceStart(); bool IsRunning { get; } }`. `nowLocal` is the server-time-zone wall clock.

Rules `Advance` implements: when idle, the next start `S` is the earliest configured (day, time) with `S > now − 5 s`, looking today and the next 7 days. The run arms at `S − max(CountdownMinutes)`. A countdown mark for minute `m` fires once when `now ≥ S − m`; it is emitted only if `now − (S − m) ≤ 5 s`, otherwise silently consumed. Start fires when `now ≥ S`: emitted if `now − S ≤ 5 s`, otherwise `Missed` and the run is dropped. Step `i` is due at `S + i × StepInterval` and emitted in order. `End` is due at `S + (n−1) × StepInterval + EndDelay`, then the scheduler is idle and never re-arms for a start ≤ the finished `S`. `n`, interval and end delay are snapshotted at Start. Force start: next `Advance` runs a run with `S = now`, no countdown. Empty `Steps`, `!Enabled`, or empty `Days`/`StartTimes` → no actions.

- [ ] **Step 1: Failing tests**, one per Review Focus line plus the happy path, all using a Wednesday 2026-10-21 and `Default`:
  - `FullRunProducesCountdownStartFiveDropsAndEnd`: ticking every second from 20:54:00 to 21:03:00 yields exactly Countdown 5,4,3,2,1 at 20:55:00…20:59:00, Start at 21:00:00, Drop 0..4 at 21:00:00, 21:00:30, 21:01:00, 21:01:30, 21:02:00, End at 21:02:30, and nothing after.
  - `RestartInsideCountdownSkipsPassedMarks`: first tick at 20:57:30 → only Countdown 2 at 20:58:00 and 1 at 20:59:00 appear (no 5, 4, 3).
  - `LateStartIsMissed`: first tick at 21:00:10 → `Missed`, no Start/Drop.
  - `CountdownCrossesMidnight`: config start 00:02 on Thursday (Days incl. Thursday) → Countdown 5 at Wednesday 23:57:00.
  - `OverlappingStartIsSkipped`: StartTimes 21:00 and 21:01 → one Start only; the second start is not emitted.
  - `EmptyStepsDoNothing`, `DisabledDoesNothing`, `WrongDayDoesNothing` (Tuesday 21:00).
  - `ConfigEditDuringRunKeepsSnapshot`: after Start, change `StepInterval` to 5 s → drops still 30 s apart.
  - `ForceStartRunsImmediately`: `RequestForceStart()` then `Advance` → Start + Drop 0 at once, no countdown.
- [ ] **Step 2: Run** `--filter JewelRainSchedulerTest` → fails to compile.
- [ ] **Step 3: Implement** the types and rules above. Keep it free of game objects.
- [ ] **Step 4: Run** the filter → all PASS.
- [ ] **Step 5: Commit** `feat(jewel-rain): scheduler state machine`.

### Task 3: Drop-area and item helpers

**Files:**
- Create: `src/GameLogic/PlugIns/JewelRain/JewelRainDropHelper.cs`
- Test: `tests/MUnique.OpenMU.Tests/JewelRain/JewelRainDropHelperTest.cs`

**Interfaces:**
- Consumes: `GameMapTerrain.WalkMap`, `JewelRainConfiguration` area fields, `JewelRainStep`.
- Produces: `static Point? TryGetDropCell(GameMapTerrain terrain, JewelRainConfiguration config)` — a random cell inside the rectangle where `WalkMap` is true (safezone allowed), `null` if there is none; `static Item? TryCreateItem(IGameContext context, JewelRainStep step)` — looks up `Configuration.Items` by `Group`/`Number`, returns a `TemporaryItem` (`Definition`, `Level`, `Durability = IsStackable() ? 1 : Definition.Durability`) or `null` if unknown.

- [ ] **Step 1: Failing tests:** `PicksOnlyWalkableCellsInsideRectangle` (terrain from a byte[] with a walkable 3×3 block inside a blocked map; 100 draws all inside that block), `ReturnsNullWhenRectangleHasNoWalkableCell`, `UnknownItemReturnsNull`.
- [ ] **Step 2: Run** → fail. **Step 3: Implement.** **Step 4: Run** → PASS.
- [ ] **Step 5: Commit** `feat(jewel-rain): drop cell and item helpers`.

### Task 4: Plug-in

**Files:**
- Create: `src/GameLogic/PlugIns/JewelRain/JewelRainPlugIn.cs`

**Interfaces:**
- Consumes: Tasks 1-3.
- Produces: `[PlugIn] [Guid("7B3E1F42-5C9A-4D68-9E21-0A6F4C8D3B15")] public sealed class JewelRainPlugIn : IPeriodicTaskPlugIn, ISupportCustomConfiguration<JewelRainConfiguration>, ISupportDefaultCustomConfiguration` with `public static void RequestStart(IGameContext gameContext)` for the GM command; `ForceStart()` marks all contexts.

Behaviour: per `GameContext` a `JewelRainScheduler` (static `ConcurrentDictionary`). `ExecuteTaskAsync` computes `nowLocal` from `gameContext.ServerTimeZone`, calls `Advance`, then executes actions in order: `Countdown` / `Start` / `End` → `ForEachPlayerAsync` + `IShowMessagePlugIn.ShowMessageAsync(text, MessageType.GoldenCenter)` using `GetTranslation(player.Culture)` and `{minutes}`; `Drop(i)` → get the map with `GetMapAsync(config.MapNumber, false)` (skip if null), then `step.Amount` times: helper cell, helper item, `new DroppedItem(item, cell, map, null, null)` and `map.AddAsync`; `Missed` → `LogWarning`. Unknown item or no cell → `LogWarning`, continue. All exceptions caught and logged; never rethrow.

- [ ] **Step 1:** Implement the class as described.
- [ ] **Step 2: Build** `dotnet build src/GameLogic` → success, no warnings in new files.
- [ ] **Step 3: Commit** `feat(jewel-rain): periodic plug-in`.

### Task 5: GM start command

**Files:**
- Create: `src/GameLogic/PlugIns/ChatCommands/JewelRainChatCommandPlugIn.cs`

**Interfaces:**
- Produces: `/jewelrain` (GameMaster only) → `JewelRainPlugIn.RequestStart(player.GameContext)` and a blue confirmation message. Implements `IChatCommandPlugIn` like `StartMiniGameEventChatCommandPlugInBase`.

- [ ] **Step 1:** Implement with `[PlugIn] [Guid(...)] [Display(Name = "...")]`.
- [ ] **Step 2: Build** `dotnet build src/GameLogic`.
- [ ] **Step 3: Commit** `feat(jewel-rain): /jewelrain GM command`.

### Task 6: Verify and set real defaults

- [ ] **Step 1:** Run the whole test project: `dotnet test tests/MUnique.OpenMU.Tests` → all new tests pass; no previously passing test fails (note pre-existing failures separately).
- [ ] **Step 2:** From the shipped Devias terrain data, find the town's safezone bounds and the real Chaos/Soul/Bless/Creation/Life group+number from the item initialization; update `JewelRainConfiguration.Default` and its test to those values.
- [ ] **Step 3:** Commit `feat(jewel-rain): real Devias area and jewel ids`. Report anything that needs an in-game check (safezone drops, final rectangle).
