# Dark Skeleton Invasion

A daily invasion event on both servers. On the Ember 5x server (game server 1) the monsters have half of their strength.

## What players see

- At **20:00** (server time zone, `SystemConfiguration.TimeZoneId`) a golden message announces the invasion on its main
  map, which is **Devias** for the daily run.
- **10 Dark Mages, 10 Dark Skeletons and 2 Dark Reapers** appear at random places all over the main map, only where players
  can walk to from the town.
- At the same time, extra monsters appear in Kalima, with this count in **each** of the listed Kalimas (Kalima 1 gets none):

  | Kalima | Monsters |
  |---|---|
  | 2, 3 | 5 Dark Mages |
  | 4, 5 | 4 Dark Skeletons |
  | 6, 7 | 3 Dark Reapers |

  They appear where players can walk to from the Kalima entrance, at least 10 tiles away from it. The start message
  doesn't name Kalima.
- On Ember 5x all invasion monsters, Kalima included, have half of their health, damage, defense, attack rate and
  defense rate. Killed invasion monsters don't come back during the run.
- **The maps get dark:** within a few seconds the light of the ground, the buildings and the characters fades to
  35 % of normal on the main map, and to 65 % in Kalima (which is dark already). Spell and torch lights stay bright.
  Leaving the map brings the normal light back at once; coming back while the invasion runs makes it dark again.
- Killing a Dark Reaper of the main map is announced to everybody with the killer's name; the Kalima ones are not.
- The invasion ends after **15 minutes**, or as soon as **both Dark Reapers of the main map are killed** (the Kalima
  Reapers don't count). Then all remaining invasion monsters disappear, Kalima included, and the light comes back.
  The end message is "All Dark Reapers have been defeated! The light returns." when the Reapers were killed, otherwise
  "Dark Skeleton invasion has ended."

## Game masters

- `/startdarkdevias`, `/startdarklorencia`, `/startdarknoria` and `/startdarkelbeland` start the invasion right away
  (about 3 seconds after the command) with that main map, plus the Kalima monsters, only on the server where the command
  is typed. They refuse when the invasion is already running or on a server where it is not enabled.
- On Lorencia, Noria and Elbeland the monsters don't appear closer than 20 tiles to the town, to protect new players.
- `/stopdark` ends the invasion right away, whatever its main map, only on the server where it is typed: the remaining
  monsters disappear, the light comes back and the end message is shown. During the three-second announcement it stops
  before any monster appears.

## Settings (admin panel, plug-in "Dark Skeleton Invasion")

| Setting | Default | Meaning |
|---|---|---|
| Timetable | 20:00 | Start times of the day; add more times for more runs |
| Task Duration | 15 minutes | Longest run |
| Monster Spawns | 1113 ×10, 1111 ×10, 1112 ×2, map 2 (Devias) | The monsters of the main map. Their map is the main map of the daily run (one is picked at random if there are several) |
| Extra Monster Spawns (Kalima) | 1113 ×5 on maps 25, 26; 1111 ×4 on maps 27, 28; 1112 ×3 on maps 29, 36 | Spawn every run, count per map; they don't end the event |
| Town Distance Map Ids | 0, 3, 51 (Lorencia, Noria, Elbeland) | Main maps on which the monsters keep the minimum town distance |
| Minimum Town Distance | 20 | In tiles |
| Minimum Entrance Distance (Extra Maps) | 10 | In tiles, from where players enter an extra map (Kalima) |
| All Bosses Killed Message | "[{mapName}] All Dark Reapers have been defeated! The light returns." | End message when the main map's Reapers were killed |
| Game Server Ids | 0, 1 | The servers which run the event (empty = all) |
| Weaker On Game Server Ids | 1 | Servers on which the event monsters are weaker |
| Weaker Strength (%) | 50 | Health, damage, defense and rates of the monsters on those servers (50 = half) |
| End When Killed (Monster Ids) | 1112 | The event ends early when all spawned monsters of these numbers on the main map are dead |

A live server whose stored settings are older gets the new settings with these defaults automatically; its other
settings are kept.

The darkness itself (35 %, or 65 % in Kalima, with a fade) is part of the client (`Render/Terrain/MapDarkness.cpp`). The
server tells the client with the map event message (code `0x0B`), event number **10**, which this project added for the
invasion.
