#pragma once

// Darkens the terrain light of the current map while the server reports a darkening map event
// (the Dark Skeleton invasion). The light fades smoothly down and back up again; the darkness
// belongs to the map on which it was switched on and ends as soon as the hero leaves that map.
namespace Render::Terrain::Darkness
{
    // The map event number the server sends for the Dark Skeleton invasion.
    constexpr unsigned char DarkSkeletonInvasionEvent = 10;

    // Switches the darkness on or off for the given map.
    void SetActive(bool active, int world);

    // Switches the darkness off immediately, without fading (e.g. when entering the game).
    void Reset();

    // Advances the fade by one frame and returns the factor (1 = normal light) which the
    // terrain light of the given current map is multiplied with.
    float UpdateLightFactor(int currentWorld);
}
