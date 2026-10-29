#include <doctest.h>

#include "Render/Models/BoneIndexRepair.h"

TEST_CASE("bone indices inside the bone table are kept [render][bone_index_repair]")
{
    CHECK(Render::Models::RepairBoneIndex(0, 5) == 0);
    CHECK(Render::Models::RepairBoneIndex(4, 5) == 4);
    CHECK(Render::Models::IsBoneIndexValid(4, 5));
}

TEST_CASE("bone indices outside the bone table are attached to the root bone [render][bone_index_repair]")
{
    CHECK(Render::Models::RepairBoneIndex(5, 5) == Render::Models::RootBoneIndex);
    CHECK(Render::Models::RepairBoneIndex(-1, 5) == Render::Models::RootBoneIndex);
    CHECK(Render::Models::RepairBoneIndex(199, 5) == Render::Models::RootBoneIndex);
    CHECK_FALSE(Render::Models::IsBoneIndexValid(5, 5));
    CHECK_FALSE(Render::Models::IsBoneIndexValid(0, 0));
}
