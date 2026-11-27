# Dark Skeleton Invasion

A daily invasion event on the Inferno 500x server (game server 0). The Ember 5x server does not run it.

## What players see

- At **20:00** (server time zone, `SystemConfiguration.TimeZoneId`) a golden message announces the invasion in **Devias**.
- **10 Dark Mages, 10 Dark Skeletons and 2 Dark Reapers** appear at random places all over the map.
- **The map gets dark:** within about three seconds the light of the ground, the buildings and the characters fades to
  35 % of normal. Spell and torch lights stay bright. Leaving Devias brings the normal light back at once; coming back while
  the invasion runs makes it dark again.
- Killing a Dark Reaper is announced to everybody with the killer's name.
- The invasion ends after **15 minutes**, or as soon as **both Dark Reapers are killed**. Then the remaining monsters
  disappear, the light comes back and an end message is shown.

## Game masters

- `/startdark` starts the invasion right away (about 3 seconds after the command). It refuses when the invasion is already
  running or on a server where it is not enabled.

## Settings (admin panel, plug-in "Dark Skeleton Invasion")

| Setting | Default | Meaning |
|---|---|---|
| Timetable | 20:00 | Start times of the day; add more times for more runs |
| Task Duration | 15 minutes | Longest run |
| Monster Spawns | 1113 ×10, 1111 ×10, 1112 ×2, map 2 (Devias), spawn on all maps | Which monsters, how many, on which maps; add a map number to spawn on more maps |
| Game Server Ids | 0 | The servers which run the event (empty = all) |
| End When Killed (Monster Ids) | 1112 | The event ends early when all spawned monsters of these numbers are dead |

The darkness itself (35 %, three-second fade) is part of the client (`Render/Terrain/MapDarkness.cpp`). The server tells the
client with the map event message (code `0x0B`), event number **10**, which this project added for the invasion.
