#pragma once

// The eleven Arena Warrior monsters (1100-1110). Each is a player-model humanoid dressed in armour and
// weapons that fit its name; there is no dedicated monster model.

struct CHARACTER;

/// Creates the Arena Warrior for the monster number, or returns nullptr when it is not one of them.
CHARACTER* CreateArenaWarrior(EMonsterType type, int key, int positionX, int positionY);
