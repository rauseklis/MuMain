#include <doctest.h>

#include "Render/Textures/UiHdTextures.h"

#include <array>
#include <vector>

using namespace Render::UiHdTextures;

TEST_CASE("HD scale is an integer multiple on both axes [render][ui_hd]")
{
    CHECK(ComputeHdScale(190, 64, 380, 128) == 2);
    CHECK(ComputeHdScale(190, 64, 760, 256) == 4);
    CHECK(ComputeHdScale(1, 14, 4, 56) == 4);
}

TEST_CASE("HD scale rejects anything that is not an exact multiple [render][ui_hd]")
{
    CHECK_FALSE(ComputeHdScale(190, 64, 381, 128).has_value());
    CHECK_FALSE(ComputeHdScale(190, 64, 380, 130).has_value());
    CHECK_FALSE(ComputeHdScale(190, 64, 760, 128).has_value());
    CHECK_FALSE(ComputeHdScale(190, 64, 190, 64).has_value());
    CHECK_FALSE(ComputeHdScale(190, 64, 950, 320).has_value());
    CHECK_FALSE(ComputeHdScale(0, 64, 380, 128).has_value());
    CHECK_FALSE(ComputeHdScale(190, 64, 0, 128).has_value());
    CHECK_FALSE(ComputeHdScale(-190, 64, 380, 128).has_value());
}

TEST_CASE("HD setting parses off, auto and explicit scales [render][ui_hd]")
{
    CHECK(ParseSetting(L"off") == Setting::Off);
    CHECK(ParseSetting(L"OFF") == Setting::Off);
    CHECK(ParseSetting(L"auto") == Setting::Auto);
    CHECK(ParseSetting(L"2") == Setting::Scale2);
    CHECK(ParseSetting(L"4") == Setting::Scale4);
    CHECK(ParseSetting(L"") == Setting::Auto);
    CHECK(ParseSetting(L"banana") == Setting::Auto);
}

TEST_CASE("HD setting selects the scales to try, best first [render][ui_hd]")
{
    CHECK(CandidateScales(Setting::Off).empty());
    CHECK(CandidateScales(Setting::Scale2) == std::vector<int>{2});
    CHECK(CandidateScales(Setting::Scale4) == std::vector<int>{4, 2});
    CHECK(CandidateScales(Setting::Auto) == std::vector<int>{4, 2});
}

TEST_CASE("HD path mirrors the original under the tier folder [render][ui_hd]")
{
    CHECK(HdPath(L"Data\\Interface\\newui_item_back01.tga", 4) == L"Data\\Interface@4x\\newui_item_back01.png");
    CHECK(HdPath(L"Data/Interface/partCharge1/newui_menu03.jpg", 2) == L"Data/Interface@2x/partCharge1/newui_menu03.png");
    CHECK(HdPath(L"C:\\MU\\data\\interface\\a.tga", 2) == L"C:\\MU\\data\\Interface@2x\\a.png");
    CHECK(HdPath(L"Data\\Interface\\noextension", 2) == L"Data\\Interface@2x\\noextension.png");
}

TEST_CASE("HD path also works for the Skill and Item folders [render][ui_hd]")
{
    CHECK(HdPath(L"Data\\Skill\\dkhead2.jpg", 4) == L"Data\\Skill@4x\\dkhead2.png");
    CHECK(HdPath(L"Data/Item/fenril01.tga", 2) == L"Data/Item@2x/fenril01.png");
    CHECK(IsHdEligiblePath(L"Data\\Item\\fenril02.tga"));
    CHECK(IsHdEligiblePath(L"Data\\Monster\\wolf.jpg"));
    CHECK(IsHdEligiblePath(L"Data\\Player\\skin.jpg"));
    CHECK(HdPath(L"Data\\NPC\\guard.tga", 2) == L"Data\\NPC@2x\\guard.png");
    CHECK_FALSE(IsHdEligiblePath(L"Data\\Effect\\spark.jpg"));
    CHECK(IsHdEligiblePath(L"Data\\World1\\tile.jpg"));
    CHECK(HdPath(L"Data\\World12\\TileGrass01.jpg", 2) == L"Data\\World12@2x\\TileGrass01.png");
    CHECK(HdPath(L"Data\\Object57\\wall.tga", 4) == L"Data\\Object57@4x\\wall.png");
    CHECK_FALSE(IsHdEligiblePath(L"Data\\Worldmap\\tile.jpg"));
    CHECK_FALSE(IsHdEligiblePath(L"Data\\Objects\\tile.jpg"));
    CHECK_FALSE(IsHdEligiblePath(L"Data\\World\\tile.jpg"));
}

TEST_CASE("HD path keeps the extension for textures that share a name [render][ui_hd]")
{
    CHECK(HdPathWithExtension(L"Data\\World1\\TileGrass01.jpg", 2) == L"Data\\World1@2x\\TileGrass01.jpg.png");
    CHECK(HdPathWithExtension(L"Data\\World1\\TileGrass01.TGA", 4) == L"Data\\World1@4x\\TileGrass01.tga.png");
    CHECK(HdPathWithExtension(L"Data\\Effect\\spark.jpg", 2).empty());
    CHECK(HdPathWithExtension(L"Data\\World1\\noextension", 2).empty());
}

TEST_CASE("HD path is empty for textures outside the interface folder [render][ui_hd]")
{
    CHECK(HdPath(L"Data\\Effect\\Damage1.jpg", 2).empty());
    CHECK(HdPath(L"unclassified.tga", 2).empty());
}

TEST_CASE("padding keeps the content at the top-left and the padding transparent [render][ui_hd]")
{
    const std::array<std::uint8_t, 8> content{1, 2, 3, 255, 5, 6, 7, 255};
    const auto padded = PadToTexture(content, 2, 1, 4, 2, false);

    REQUIRE(padded.size() == 4u * 2u * 4u);
    CHECK(padded[0] == 1);
    CHECK(padded[3] == 255);
    CHECK(padded[4] == 5);
    CHECK(padded[8] == 0);
    CHECK(padded[11] == 0);
    CHECK(padded[16] == 0);
}

TEST_CASE("padding of an originally opaque texture is opaque black [render][ui_hd]")
{
    const std::array<std::uint8_t, 4> content{9, 9, 9, 255};
    const auto padded = PadToTexture(content, 1, 1, 2, 1, true);

    REQUIRE(padded.size() == 8u);
    CHECK(padded[4] == 0);
    CHECK(padded[5] == 0);
    CHECK(padded[6] == 0);
    CHECK(padded[7] == 255);
}
