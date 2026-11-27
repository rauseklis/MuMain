#pragma once

// The dark skeletons (monsters 1111-1113): the Death Bone skeleton drawn bigger and much darker, in the ember look of
// the Kalima monsters: a glowing head, ember sparks under its feet while it walks, and ember sparks bursting out of
// its body when it dies. Dark Skeleton (Gorgon Staff) and Dark Reaper (Rune Blade) are 20% bigger, Dark Mage (Skull
// Staff) 10%. The server decides which skill their attacks show (Evil Spirit, Hellfire, Flame).

struct CHARACTER;
struct OBJECT;

/// Creates the dark skeleton for the monster number, or returns nullptr when it is not one of them.
CHARACTER* CreateDarkSkeleton(EMonsterType type, int key, int positionX, int positionY);

/// Whether the monster number is one of the dark skeletons.
bool IsDarkSkeleton(int monsterIndex);

/// Darkens the light a dark skeleton is drawn with; it still follows the light of the terrain (day, night, torches).
void DarkenDarkSkeletonLight(vec3_t light);

/// Draws the ember glow in the head of a living dark skeleton. Does nothing for other characters.
void RenderDarkSkeletonGlow(CHARACTER* c, OBJECT* o);

/// Creates the ember sparks under the feet of a walking dark skeleton which has them. Does nothing for other characters.
void CreateDarkSkeletonFootSparks(CHARACTER* c, OBJECT* o);

/// Creates the ember sparks a dying dark skeleton bursts into once it has faded to half, like the Kalima monsters.
/// Does nothing for other characters.
void CreateDarkSkeletonDeathEmbers(CHARACTER* c, OBJECT* o);
