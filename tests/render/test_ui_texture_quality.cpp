#include <doctest.h>

#include "Render/Textures/UiTextureQuality.h"

#include <array>

TEST_CASE("only interface assets receive smooth 2D treatment [render][ui_texture_quality]")
{
    using Render::UiTextureQuality::IsInterfaceTexturePath;

    CHECK(IsInterfaceTexturePath(L"Data\\Interface\\FontTest.tga"));
    CHECK(IsInterfaceTexturePath(L"C:/MU/data/interface/newui_option_top.tga"));

    CHECK_FALSE(IsInterfaceTexturePath(L"Data\\Effect\\Damage1.jpg"));
    CHECK_FALSE(IsInterfaceTexturePath(L"Data\\Skill\\HellGate.tga"));
    CHECK_FALSE(IsInterfaceTexturePath(L"unclassified.tga"));
}

TEST_CASE("transparent UI borders inherit visible edge colour without changing alpha [render][ui_texture_quality]")
{
    std::array<std::uint8_t, 12> pixels{
        0, 0, 0, 0,
        120, 80, 40, 255,
        0, 0, 0, 0,
    };

    Render::UiTextureQuality::BleedTransparentRgb(pixels, 3, 1);

    CHECK(pixels == std::array<std::uint8_t, 12>{
                        120, 80, 40, 0,
                        120, 80, 40, 255,
                        120, 80, 40, 0,
                    });
}

TEST_CASE("transparent UI bleed rejects malformed dimensions [render][ui_texture_quality]")
{
    std::array<std::uint8_t, 4> pixel{1, 2, 3, 0};
    Render::UiTextureQuality::BleedTransparentRgb(pixel, 2, 1);
    CHECK(pixel == std::array<std::uint8_t, 4>{1, 2, 3, 0});
}
