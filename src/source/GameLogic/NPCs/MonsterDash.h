#pragma once

#include <algorithm>
#include <cmath>

// The dash of Mega Crust, Alpha Crust and the monsters that move like them. The server walks such a monster tile by
// tile, one every MoveDelay (400 ms for the Crusts), and sends the end of the whole path at once. The client slides
// the monster there. Sliding every path in the same short time made a long path look like a teleport, so the slide
// takes as long as the server's walk, with a minimum so that a single tile still reads as a dash.
namespace GameLogic::MonsterDash
{
    // The original game loop ticks 25 times a second; the server moves a Crust one tile in 400 ms.
    constexpr float TicksPerTile = 10.f;
    constexpr float MinimumTicks = 15.f;
    constexpr float MaximumTicks = 90.f;

    // How long, in ticks of the original game loop, a dash over the given number of tiles lasts.
    inline float DurationTicks(float tiles)
    {
        return std::clamp(tiles * TicksPerTile, MinimumTicks, MaximumTicks);
    }

    // How far along the way the monster is after elapsedTicks of a dash of durationTicks: 0 to 1. It starts fast and
    // settles into its place, which is what makes the movement read as a dash.
    inline float Progress(float elapsedTicks, float durationTicks)
    {
        const float time = std::clamp(elapsedTicks / std::max(durationTicks, 1.f), 0.f, 1.f);
        const float remaining = 1.f - time;
        return 1.f - remaining * remaining;
    }

    // Tiles between two positions, counted along the longer axis because the monster may move diagonally.
    inline float TileDistance(float fromX, float fromY, float toX, float toY)
    {
        return std::max(std::fabs(toX - fromX), std::fabs(toY - fromY));
    }
}
