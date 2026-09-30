#pragma once

// Pure, header-only extraction of the directional-shear math used by
// CalcShadowPosition() (ZzzBMD.cpp) to flatten a character/monster mesh into
// a ground-projected shadow decal. Split out so the shear formula itself can
// be exercised by a standalone unit test (tests/render/test_shadow_projection.cpp)
// without pulling in the renderer, terrain, or any other engine subsystem --
// same rationale as BmdPolygonTopology.h next to it.
//
// Investigated 2026-09-30 as part of the Kalima "shadow blob" report: this
// formula is identical on every map (sx/sy do not depend on gMapManager), and
// Kalima's own terrain-height data was independently verified flat and clean
// (see docs/AUDIT.md), so the formula itself was not expected to be the fault
// -- this header/test exists to make that verifiable by anyone, not just by
// re-reading the derivation.
namespace Render::Models
{
// Computes the horizontal (X) displacement CalcShadowPosition() applies to a
// single mesh vertex, given that vertex's position relative to the casting
// object's origin (relativeX, relativeZ) and the two directional-shear
// constants the caller passes to CalcShadowPosition/RenderBodyShadow (sx is
// 2000 normally, 2500 in Battle Castle; sy is always 4000).
//
// The formula divides by (relativeZ - sy): as relativeZ approaches sy the
// result diverges towards +/-infinity. For every real character/monster/prop
// mesh, relativeZ (height above the object's own origin) stays far below sy,
// so this is a latent property of the formula rather than a bug by itself --
// but it means a future model whose highest vertex sits within a few hundred
// units of sy would produce an extreme, "blown up" shadow shape. See the
// "asymptote" test case for the concrete bound.
[[nodiscard]] constexpr float CalcShadowHorizontalShear(float relativeX, float relativeZ, float sx, float sy)
{
    return relativeZ * (relativeX + sx) / (relativeZ - sy);
}
}
