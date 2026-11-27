#pragma once

// The dark skeletons (monsters 1111-1113): the Death Bone skeleton drawn bigger and much darker, with sparks under
// its feet while it walks and flames on its body, which dies in sparks and glitter like the Kalima monsters. Dark
// Skeleton (Gorgon Staff) and Dark Reaper (Rune Blade) are 20% bigger, Dark Mage (Skull Staff) 10%. The server
// decides which skill their attacks show (Evil Spirit, Hellfire, Flame).

struct CHARACTER;
struct OBJECT;

/// Creates the dark skeleton for the monster number, or returns nullptr when it is not one of them.
CHARACTER* CreateDarkSkeleton(EMonsterType type, int key, int positionX, int positionY);

/// Whether the monster number is one of the dark skeletons.
bool IsDarkSkeleton(int monsterIndex);

/// Darkens the light a dark skeleton is drawn with; it still follows the light of the terrain (day, night, torches).
void DarkenDarkSkeletonLight(vec3_t light);

/// Creates the sparks under the feet of a walking dark skeleton which has them. Does nothing for other characters.
void CreateDarkSkeletonFootSparks(CHARACTER* c, OBJECT* o);

/// Creates the flames on the body of a living dark skeleton, and the sparks and glitter of a dying one (like the
/// Kalima monsters). Does nothing for other characters.
void CreateDarkSkeletonFlames(CHARACTER* c, OBJECT* o);
