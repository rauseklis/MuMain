#include <doctest.h>

#include <array>
#include <span>

#include "UI/NewUI/Inventory/ItemDataParser.h"

namespace
{
    constexpr BYTE HasOptionFlag = 0x01;
    constexpr BYTE HasExcellentFlag = 0x08;
    constexpr BYTE HasAncientFlag = 0x10;
    constexpr BYTE HasHarmonyFlag = 0x20;
    constexpr BYTE HasGuardianFlag = 0x40;
    constexpr BYTE HasSocketsFlag = 0x80;

    // Group 0, number 1, level 5, durability 10, followed by the option flags byte.
    std::array<BYTE, 5> Header(BYTE flags)
    {
        return { 0x00, 0x01, 5, 10, flags };
    }
}

TEST_CASE("guardian flag sets HasGuardianOption [ui][item_parse]")
{
    const auto data = Header(HasGuardianFlag);
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK(params.IsValid);
    CHECK(params.HasGuardianOption);
}

TEST_CASE("item without guardian flag keeps HasGuardianOption false [ui][item_parse]")
{
    const auto data = Header(0x00);
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK(params.IsValid);
    CHECK_FALSE(params.HasGuardianOption);
}

TEST_CASE("guardian flag consumes no extra bytes alongside harmony and sockets [ui][item_parse]")
{
    // Flags: Harmony | Guardian | Sockets. Harmony byte, socket header (bonus 1, count 2), two sockets.
    const std::array<BYTE, 9> data = { 0x00, 0x01, 5, 10, HasHarmonyFlag | HasGuardianFlag | HasSocketsFlag,
                                       0x21, 0x12, 0x07, 0x08 };
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK(params.IsValid);
    CHECK(params.HasGuardianOption);
    CHECK(params.HasHarmonyOption);
    CHECK(params.HarmonyOptionType == 2);
    CHECK(params.HarmonyOptionLevel == 1);
    CHECK(params.SocketBonusOption == 1);
    CHECK(params.SocketCount == 2);
    CHECK(params.SocketOptions[0] == 0x07);
    CHECK(params.SocketOptions[1] == 0x08);
}

TEST_CASE("option, excellent and ancient fields decode unchanged [ui][item_parse]")
{
    // Flags: Option | Excellent | Ancient. Option byte (type 3, level 2), excellent 0x0A, ancient (bonus 4, discriminator 3).
    const std::array<BYTE, 8> data = { 0x00, 0x01, 5, 10, HasOptionFlag | HasExcellentFlag | HasAncientFlag,
                                       0x32, 0x0A, 0x43 };
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK(params.IsValid);
    CHECK(params.OptionType == 3);
    CHECK(params.OptionLevel == 2);
    CHECK(params.ExcellentFlags == 0x0A);
    CHECK(params.AncientDiscriminator == 3);
    CHECK(params.AncientBonusOption == 4);
    CHECK_FALSE(params.HasGuardianOption);
}

TEST_CASE("data shorter than the header is rejected [ui][item_parse]")
{
    const std::array<BYTE, 4> data = { 0x00, 0x01, 5, 10 };
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK_FALSE(params.IsValid);
}

TEST_CASE("truncated option byte is rejected without reading past the buffer [ui][item_parse]")
{
    const auto data = Header(HasOptionFlag);
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK_FALSE(params.IsValid);
}

TEST_CASE("truncated harmony byte is rejected [ui][item_parse]")
{
    const auto data = Header(HasHarmonyFlag);
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK_FALSE(params.IsValid);
}

TEST_CASE("socket count above MAX_SOCKETS is rejected instead of overflowing [ui][item_parse]")
{
    // Socket header declares 15 sockets; the buffer provides 15 socket bytes, but the struct only holds MAX_SOCKETS.
    std::array<BYTE, 22> data{};
    data[0] = 0x00;
    data[1] = 0x01;
    data[2] = 5;
    data[3] = 10;
    data[4] = HasSocketsFlag;
    data[5] = 0x0F;
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK_FALSE(params.IsValid);
}

TEST_CASE("socket list shorter than its declared count is rejected [ui][item_parse]")
{
    const std::array<BYTE, 7> data = { 0x00, 0x01, 5, 10, HasSocketsFlag, 0x03, 0x07 };
    const auto params = ParseItemData(std::span<const BYTE>(data));

    CHECK_FALSE(params.IsValid);
}
