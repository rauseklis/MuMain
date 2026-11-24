#include <doctest.h>

#include "UI/NewUI/HUD/ExpHuntBarLayout.h"

namespace Bar = UI::HUD::ExpHuntBar;

TEST_CASE("exp hunt bar: the three pieces tile the bar with no gap or overlap")
{
    for (const int width : {Bar::kMinWidth, 140, 211, 400})
    {
        const Bar::Rect left = Bar::LeftPiece();
        const Bar::Rect middle = Bar::MiddlePiece(width);
        const Bar::Rect right = Bar::RightPiece(width);
        CHECK(left.x == 0);
        CHECK(middle.x == left.x + left.width);
        CHECK(right.x == middle.x + middle.width);
        CHECK(right.x + right.width == width);
        CHECK(middle.width > 0);
    }
}

TEST_CASE("exp hunt bar: any text fits between the ends with the padding free on both sides")
{
    for (const int textWidth : {0, 40, 120, 143, 230, 400})
    {
        const int width = Bar::BarWidth(textWidth);
        const Bar::Rect box = Bar::TextBox(width);
        CHECK(box.width >= textWidth);
        CHECK(box.x - Bar::LeftPiece().width == Bar::kTextPadding);
        CHECK(Bar::RightPiece(width).x - (box.x + box.width) == Bar::kTextPadding);
    }
}

TEST_CASE("exp hunt bar: the bar never gets narrower than the minimum and grows with the text")
{
    CHECK(Bar::BarWidth(0) == Bar::kMinWidth);
    int previous = Bar::BarWidth(0);
    for (int textWidth = 0; textWidth <= 400; ++textWidth)
    {
        const int width = Bar::BarWidth(textWidth);
        CHECK(width >= previous);
        previous = width;
    }
}

TEST_CASE("exp hunt bar: the bar is smaller than the original 220 x 20 one")
{
    // 143 logical pixels is the width of the longest line ("Experience Hunter: 9,999,999,999 / min") at 1080p.
    CHECK(Bar::BarWidth(143) < 220);
    CHECK(Bar::kHeight < 20);
    CHECK(Bar::kHitHeight <= Bar::kHeight);
}

TEST_CASE("exp hunt bar: atlas pieces are in the texture and keep two pixels of gutter apart")
{
    const Bar::Rect pieces[] = {Bar::kLeftSource, Bar::kRightSource, Bar::kMiddleSource};
    for (const Bar::Rect& piece : pieces)
    {
        CHECK(piece.x >= 2);
        CHECK(piece.y >= 2);
        CHECK(piece.x + piece.width + 2 <= Bar::kAtlasSize);
        CHECK(piece.y + piece.height + 2 <= Bar::kAtlasSize);
    }
    CHECK(Bar::kRightSource.x - (Bar::kLeftSource.x + Bar::kLeftSource.width) >= 4);
    CHECK(Bar::kMiddleSource.x - (Bar::kRightSource.x + Bar::kRightSource.width) >= 4);
}

TEST_CASE("exp hunt bar: pieces snapped to whole screen pixels keep neighbours touching at every scale")
{
    namespace Snap = UI::HUD::TopLeftBar;
    for (const float scale : {1.0f, 1.5f, 2.25f, 3.0f})
    {
        for (const int x : {0, 7, 101, 424})
        {
            const int width = 150;
            const Bar::Rect left{x, 0, Bar::kCapWidth, Bar::kHeight};
            const Bar::Rect middle{x + Bar::kCapWidth, 0, width - 2 * Bar::kCapWidth, Bar::kHeight};
            const Snap::SnappedRect a = Snap::SnapRect(left, scale, scale, 0.0f, 0.0f);
            const Snap::SnappedRect b = Snap::SnapRect(middle, scale, scale, 0.0f, 0.0f);
            CHECK(a.x + a.width == doctest::Approx(b.x));
            CHECK(a.width * scale == doctest::Approx(std::round(a.width * scale)));
        }
    }
}
