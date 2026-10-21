# Jewel Rain event — design

Server-side feature for the OpenMU game server (`mu-online-openmu/server`). No
client changes.

**Status: implemented** in `rauseklis/OpenMU` PR #7 (merge commit `8eaf431`),
tested in game together with the Jewel Bank. The sections below are the design as
agreed; "What was built" at the end records where the build differs from it.

## Intent

A recurring, simple "drop party": at set times, a countdown announces an event,
then a series of item batches rain onto Devias town and players race to pick
them up. It exists to give the community a regular, exciting reason to gather in
one place. Inspired by the "Lluvia de Joyas" event in the MSPro server package
(`Data/Custom/Events/CustomEventDrop.txt`), but rebuilt so that everything is
configurable instead of one hard-coded row per drop.

## Behaviour (as decided)

- **Schedule:** Wednesday, Friday and Sunday at 21:00, Eastern Europe time
  (Latvia, `Europe/Riga` / Windows `FLE Standard Time`, daylight saving
  respected so it stays 21:00 local all year).
- **Countdown:** golden on-screen announcements to all online players at 5, 4,
  3, 2 and 1 minute before the start, then a start announcement at 21:00.
- **Where:** Devias (map 2), in the town area.
- **Drops:** the event has an ordered list of steps. Each step is one item type
  and an amount. Default list:
  1. 15 Jewel of Chaos
  2. 15 Jewel of Soul
  3. 15 Jewel of Bless
  4. 15 Jewel of Creation
  5. 15 Jewel of Life
- **Rhythm:** step 1 drops the moment the event starts; each following step
  drops 30 seconds after the previous one. The 15 pieces of a step land at
  random reachable spots around the town, so players have to run for them.
- **End:** there is no fixed duration. The event ends when the last step has
  dropped. An end announcement ("Jewel Rain has ended") goes out 30 seconds
  after the last drop. With the 5 default steps the whole thing runs 2:30
  after the start. Adding or removing a step lengthens or shortens it.
- **Pick-up:** drops are free for everyone from the first second (no owner).
  Unclaimed items disappear on the map's normal ground-item timeout.

## Configuration (all editable without code changes)

Edited in the Admin Panel as the plug-in's custom configuration, like the
existing invasion events.

| Setting | Default |
|---|---|
| Enabled | yes |
| Days of week | Wed, Fri, Sun |
| Start time(s) of day | 21:00 |
| Countdown announcements (minutes before start) | 5, 4, 3, 2, 1 |
| Map | Devias (2) |
| Drop area (rectangle on that map) | Devias town: x 178-244, y 6-74 (from the shipped terrain file, confirmed in game) |
| Interval between steps | 30 s |
| Delay from last drop to end announcement | 30 s |
| Steps: item (group + number), level, amount | the five jewels above |
| Messages: countdown, start, end (localized text, `{minutes}` placeholder) | English defaults |

Any item the game knows can be a step, not just jewels and fruits. Fruits are
added by adding steps. The item list is read when an event starts; edits made
during a running event apply to the next one.

## Goals

- Reliable start at the scheduled local time, with the countdown on the right
  minutes.
- Fully data-driven: schedule, area, timing and items can all change from the
  Admin Panel.
- Fair: everyone on the map can pick up from the instant items land.

## Non-goals

- No ranking, scoreboards, per-player rewards or winners.
- No random item pools, weights or probabilities (steps are an explicit,
  ordered list).
- No client changes or new client UI.
- No several simultaneous events; one Jewel Rain configuration.
- No resuming an event interrupted by a server restart, and no catch-up if the
  server was down at 21:00.

## Anti-goals (a failure even if it technically works)

- Items landing on unreachable cells (walls, void) or being untouchable.
- The event starting twice, or a new one starting while one is running.
- Countdown messages fired late, repeated, or out of order.
- Hard-coding the time zone, the days or the items in code.

## Constraints and what the server already gives us

Found by reading `server/src/GameLogic`:

- A per-second periodic-task hook exists (`IPeriodicTaskPlugIn`, driven by
  `GameContext`'s 1-second timer) and a server time zone
  (`GameContext.ServerTimeZone`, set from the system configuration).
- The existing `PeriodicTaskBasePlugIn` is **not enough as is**: its timetable
  has times of day only (no weekday), a single pre-start delay (no 5-to-1
  countdown), and a fixed `TaskDuration` (ours is derived from the step
  count). The new plug-in is its own small state machine on the same hook
  rather than a subclass.
- Golden on-screen messages go through `IShowMessagePlugIn` with
  `MessageType.GoldenCenter`; `ForEachPlayerAsync` reaches everyone on the
  game server (see `BaseInvasionPlugIn`).
- Ground items are `DroppedItem(item, position, map, dropper, owners)`;
  `owners: null` means anyone can pick up. Random walkable cells come from
  `map.Terrain.RandomWalkableCoordinate` and an item factory pattern exists in
  `ItemChatCommandPlugIn`.
- Plug-in settings use `ISupportCustomConfiguration<T>` and `[Display]`
  attributes to appear in the Admin Panel.

## Decided by me (the author), flagged for review

- New plug-in `JewelRainPlugIn` plus a `JewelRainConfiguration` class; state is
  kept per `GameContext`.
- The countdown is computed from the scheduled start time on each tick, and
  each minute mark is announced once. If the server starts late inside the
  countdown window, marks already passed are skipped, not replayed.
- The server timezone is set via the existing system time-zone setting; the
  plug-in only reads it. Setup note: the live server must be set to Latvia time.
- A GM chat command to start/stop the event on demand, for testing, in the
  style of the existing `Start*EventChatCommandPlugIn` commands.
- Messages go to every online player on the game server, wherever they are.
- If the configured map or the item list is invalid or empty, the event logs
  a warning and does not run.
- The schedule/steps logic is kept free of game objects so it can be unit
  tested with a fake clock.

## Calls made after the builder check (author's calls, flagged for review)

- **Town and safezone:** drops are meant to land inside Devias town, safezone
  included (the original MSPro event dropped inside towns too). The plan starts
  with a short in-game spike: find the town's bounds from the terrain data,
  confirm items can land and be picked up there, and set the default rectangle
  from that. If safezone cells reject drops, the rectangle moves to the walkable
  area around the town and this document is updated.
- **Amount means separate pieces:** "15" is 15 separate ground items, each
  picked up on its own, even for items that stack in a bag.
- **Overlap:** if a start time arrives while an event is running, it is skipped
  and a warning is logged. Start times are evaluated as instants in the server
  time zone, so a countdown that crosses midnight still works.
- **Several game servers:** each game server that hosts the configured map runs
  its own copy, and messages go to the players on that server.
- **Admin Panel:** the invasion events already edit a list of nested objects
  (`PeriodicInvasionConfiguration.Mobs`), so the step list uses the same
  mechanism. No new editor is expected.
- **End announcement:** stays 30 s after the last drop even though ground items
  last longer; the message means "no more drops", not "all items gone". Its text
  says so.
- **Messages:** plain text per message in the configuration, with the existing
  `LocalizedString` type so a translation can be added later.

## Left to the builder

- Exact class/file layout and resource-string handling.
- How to pick drop cells (rectangle plus walkable/non-safezone checks) and the
  fallback when a cell can't be found.
- Test structure, and how the in-game verification of the default town
  rectangle is done.

## What was built

- Code: `src/GameLogic/PlugIns/JewelRain/` (`JewelRainConfiguration`,
  `JewelRainStep`, `JewelRainScheduler`, `JewelRainDropHelper`,
  `JewelRainPlugIn`) and `PlugIns/ChatCommands/JewelRainChatCommandPlugIn.cs`.
  Tests: `tests/MUnique.OpenMU.Tests/JewelRain/` (15 tests).
- Item ids: Jewel of Chaos is group 12 number 15; Soul 14/14, Bless 14/13,
  Creation 14/22, Life 14/16. (The design said "group 14" for all; Chaos is the
  exception.)
- Scheduler rules as built: a scheduled start noticed more than 5 s late is
  skipped and logged; a countdown mark is only shown if it is noticed within 5 s
  of its time; the first start time looks back one minute, so a start missed by a
  restart is reported rather than silently dropped; a run keeps the settings it
  had when it armed (the countdown window).
- `/jewelrain` (game master) starts an event immediately, without countdown.
- Safezone drops work: nothing in the pickup or drop code rejects them, and it
  was verified in game. Pickups made through the Jewel Bank auto-deposit work too
  (the Jewel Bank banks plain jewels, fruits and packed jewels from the floor).

## Deployment

1. Rebuild and restart the game server from `master`. The plug-in settings are
   created automatically on the first start (default: Wed/Fri/Sun 21:00, Devias,
   the five jewels, 15 each); no database migration is needed for Jewel Rain.
   Edit them in the Admin Panel under "Jewel Rain Event".
2. Set the server time zone to Latvia (`FLE Standard Time`) in the system
   configuration; the schedule uses the server time zone, and the default of UTC
   would run the event at 21:00 UTC instead.
3. No client change is needed for Jewel Rain. The Jewel Bank needs the matching
   client and server (see the Jewel Bank design); the production client in
   `C:\Games\Emberion Client` is an older build without the Jewel Bank window.
4. Connect server ports for a test server must stay within 44288-44543: the
   client treats every other port as an encrypted game connection and sends
   garbage to a plain connect server.
