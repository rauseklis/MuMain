#include <array>

#include <doctest.h>

#include "Render/Models/BmdPolygonTopology.h"

TEST_CASE("BMD triangles remain one renderer triangle [render][bmd]")
{
    CHECK(Render::Models::GetBmdTriangleCornerCount(3) == 3);

    std::array<int, 3> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i)
        corners[i] = Render::Models::BmdPolygonTriangleCorners[i];

    CHECK(corners == std::array<int, 3>{0, 1, 2});
}

TEST_CASE("BMD quads become two renderer triangles [render][bmd]")
{
    CHECK(Render::Models::GetBmdTriangleCornerCount(4) == 6);
    CHECK(Render::Models::BmdPolygonTriangleCorners == std::array<int, 6>{0, 1, 2, 0, 2, 3});
}

TEST_CASE("invalid BMD polygon sizes emit no renderer vertices [render][bmd]")
{
    CHECK(Render::Models::GetBmdTriangleCornerCount(0) == 0);
    CHECK(Render::Models::GetBmdTriangleCornerCount(2) == 0);
    CHECK(Render::Models::GetBmdTriangleCornerCount(5) == 0);
}
