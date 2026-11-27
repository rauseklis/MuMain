#pragma once

// The dark skeletons (monsters 1111-1112): the Death Bone skeleton drawn 20% bigger and much darker, with sparks under
// its feet while it walks. Dark Skeleton carries a Gorgon Staff, Dark Reaper a Rune Blade. The server decides which
// skill their attacks show (Evil Spirit, Hellfire).

struct CHARACTER;
struct OBJECT;

/// Creates the dark skeleton for the monster number, or returns nullptr when it is not one of them.
CHARACTER* CreateDarkSkeleton(EMonsterType type, int key, int positionX, int positionY);

/// Whether the monster number is one of the dark skeletons.
bool IsDarkSkeleton(int monsterIndex);

/// Darkens the light a dark skeleton is drawn with; it still follows the light of the terrain (day, night, torches).
void DarkenDarkSkeletonLight(vec3_t light);

/// Creates the sparks under the feet of a walking dark skeleton. Does nothing for other characters.
void CreateDarkSkeletonFootSparks(CHARACTER* c, OBJECT* o);
