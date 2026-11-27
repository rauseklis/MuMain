#include "Render/Terrain/MapDarkness.h"

#include <algorithm>

#include "Core/Time/FrameTiming.h"

namespace Render::Terrain::Darkness
{
namespace
{
constexpr int NoWorld = -1;
constexpr float NormalLightFactor = 1.f;

// How much light is left during the invasion: 0.35 = 35 % of the normal light.
constexpr float DarkLightFactor = 0.35f;

// The fade from normal to dark (or back) takes about three seconds:
// 75 frames at the reference animation speed of 25 frames per second.
constexpr float FadeFrames = 75.f;
constexpr float FadeStepPerFrame = (NormalLightFactor - DarkLightFactor) / FadeFrames;

bool isActive = false;
int darkWorld = NoWorld;
float lightFactor = NormalLightFactor;
} // namespace

void SetActive(bool active, int world)
{
    isActive = active;
    darkWorld = active ? world : NoWorld;
}

void Reset()
{
    SetActive(false, NoWorld);
    lightFactor = NormalLightFactor;
}

float UpdateLightFactor(int currentWorld)
{
    if (isActive && currentWorld != darkWorld)
    {
        // The hero left the dark map; the server sends the state again on re-entry while the event runs.
        Reset();
    }

    const float target = isActive ? DarkLightFactor : NormalLightFactor;
    const float step = FadeStepPerFrame * FPS_ANIMATION_FACTOR;
    lightFactor = lightFactor > target ? std::max(target, lightFactor - step) : std::min(target, lightFactor + step);
    return lightFactor;
}
} // namespace Render::Terrain::Darkness
