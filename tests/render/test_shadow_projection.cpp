#include <cmath>

#include <doctest.h>

#include "Render/Models/ShadowProjection.h"

using Render::Models::CalcShadowHorizontalShear;

// Background: investigated 2026-09-30 as part of the Kalima "shadow blob"
// report (docs/AUDIT.md). This exercises the exact shear formula
// CalcShadowPosition() (ZzzBMD.cpp) applies to every shadow-mesh vertex, in
// isolation, so its behavior for realistic and edge-case inputs is pinned
// down by a test rather than by re-deriving it by hand each time.

TEST_CASE("shadow shear matches the hand-derived example [render][shadow]")
{
    // Kalima-fight-shaped example worked through in the investigation: a
    // vertex 150 units above the casting object's own origin (a plausible
    // wing height) and 50 units to its side, with the normal (non-Battle-
    // Castle) constants.
    const float relativeX = 50.f;
    const float relativeZ = 150.f;
    const float sx = 2000.f;
    const float sy = 4000.f;

    const float shift = CalcShadowHorizontalShear(relativeX, relativeZ, sx, sy);

    // shift = 150 * (50 + 2000) / (150 - 4000) = 307500 / -3850
    CHECK(shift == doctest::Approx(307500.f / -3850.f));
    CHECK(shift == doctest::Approx(-79.87f).epsilon(0.001));
}

TEST_CASE("shadow shear stays bounded for realistic character/monster/wing heights [render][shadow]")
{
    const float sx = 2000.f;
    const float sy = 4000.f;

    // Sweep every relative height from flat ground up through a tall winged
    // model (character/monster meshes in this game top out well under 1000
    // local units above their own origin) and every plausible side offset.
    // None of these should ever produce a shift whose magnitude rivals or
    // exceeds a Kalima-arena-sized distance (a few thousand units) -- if a
    // future model change makes this fail, that model's highest vertex is
    // approaching the sy asymptote below and the shadow WILL visibly blow up
    // in-game, not just in this test.
    for (float relativeZ = 0.f; relativeZ <= 900.f; relativeZ += 25.f)
    {
        for (float relativeX = -500.f; relativeX <= 500.f; relativeX += 50.f)
        {
            const float shift = CalcShadowHorizontalShear(relativeX, relativeZ, sx, sy);
            CAPTURE(relativeZ);
            CAPTURE(relativeX);
            CHECK(std::fabs(shift) < 2000.f);
        }
    }
}

TEST_CASE("shadow shear diverges as the relative height approaches the sy asymptote [render][shadow]")
{
    // This is not "the bug" (no real mesh gets anywhere near sy=4000 above
    // its own origin -- Kalima's own terrain-height data was independently
    // verified flat and clean, see docs/AUDIT.md) -- it documents the known
    // shape of the formula so a future change that lets relativeZ grow this
    // large is a deliberate, understood risk rather than a surprise blob.
    const float sx = 2000.f;
    const float sy = 4000.f;

    const float farFromAsymptote = std::fabs(CalcShadowHorizontalShear(0.f, 200.f, sx, sy));
    const float nearAsymptote = std::fabs(CalcShadowHorizontalShear(0.f, 3990.f, sx, sy));

    CHECK(nearAsymptote > farFromAsymptote * 100.f);
}

TEST_CASE("shadow shear is zero for a vertex sitting exactly at the origin [render][shadow]")
{
    CHECK(CalcShadowHorizontalShear(0.f, 0.f, 2000.f, 4000.f) == doctest::Approx(0.f));
}
