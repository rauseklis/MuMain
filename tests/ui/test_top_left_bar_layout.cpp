#include <doctest.h>

#include "UI/NewUI/HUD/TopLeftBarLayout.h"

namespace Bar = UI::HUD::TopLeftBar;

TEST_CASE("top-left bar: the buttons sit inside the right piece and do not touch the text box")
{
    const Bar::Button buttons[] = {Bar::Button::Settings, Bar::Button::MuHelper, Bar::Button::ExperienceHunter};

    int previousRight = Bar::kTextBox.x + Bar::kTextBox.width;
    for (const Bar::Button button : buttons)
    {
        const Bar::Rect rect = Bar::ButtonRect(button);
        CHECK(rect.x >= previousRight);
        CHECK(rect.x >= Bar::kRightPiece.x);
        CHECK(rect.x + rect.width <= Bar::kRightPiece.x + Bar::kRightPiece.width);
        CHECK(rect.y >= 0);
        CHECK(rect.y + rect.height <= Bar::kRightPiece.height);
        previousRight = rect.x + rect.width;
    }
}

TEST_CASE("top-left bar: the bar is clearly narrower than the old 217 pixel bar and the pieces add up")
{
    constexpr int oldWidth = 217;
    CHECK(Bar::kFrameWidth * 5 <= oldWidth * 4);
    CHECK(Bar::kMiddlePiece.x == Bar::kLeftPiece.x + Bar::kLeftPiece.width);
    CHECK(Bar::kRightPiece.x == Bar::kMiddlePiece.x + Bar::kMiddlePiece.width);
}

TEST_CASE("top-left bar: the drawn bar and the clickable bar are the same shape")
{
    CHECK(Bar::BarContains(0, 0));
    CHECK(Bar::BarContains(Bar::kFrameWidth - 1, Bar::kRightHeight - 1));
    CHECK_FALSE(Bar::BarContains(Bar::kFrameWidth, 0));
    CHECK_FALSE(Bar::BarContains(0, Bar::kFrameHeight));
    // the right end is shorter than the left end and the middle
    CHECK(Bar::BarContains(Bar::kLeftWidth, Bar::kFrameHeight - 1));
    CHECK_FALSE(Bar::BarContains(Bar::kFrameWidth - 1, Bar::kRightHeight));
}

TEST_CASE("top-left bar: a click is inside a button exactly where the button is drawn")
{
    const Bar::Rect rect = Bar::ButtonRect(Bar::Button::MuHelper);
    CHECK(Bar::Contains(rect, rect.x, rect.y));
    CHECK(Bar::Contains(rect, rect.x + rect.width - 1, rect.y + rect.height - 1));
    CHECK_FALSE(Bar::Contains(rect, rect.x - 1, rect.y));
    CHECK_FALSE(Bar::Contains(rect, rect.x + rect.width, rect.y));
    CHECK_FALSE(Bar::Contains(rect, rect.x, rect.y + rect.height));
}

TEST_CASE("top-left bar: every face of the atlas lies inside it and the faces do not overlap")
{
    const Bar::Face faces[] = {Bar::Face::Settings, Bar::Face::MuHelperStart, Bar::Face::MuHelperStop,
                               Bar::Face::ExperienceOff, Bar::Face::ExperienceOn};
    const Bar::ButtonState states[] = {Bar::ButtonState::Up, Bar::ButtonState::Over, Bar::ButtonState::Down};

    for (const Bar::Face face : faces)
    {
        for (const Bar::ButtonState state : states)
        {
            const Bar::Rect rect = Bar::FaceRect(face, state);
            CHECK(rect.x >= 0);
            CHECK(rect.y >= Bar::kRightSource.y + Bar::kRightSource.height);
            CHECK(rect.x + rect.width <= Bar::kAtlasSize);
            CHECK(rect.y + rect.height <= Bar::kAtlasSize);
            CHECK(Bar::kFaceStrideX - rect.width >= 2); // a gutter against the neighbouring face
            CHECK(Bar::kFaceStrideY - rect.height >= 2);
        }
    }
}

TEST_CASE("top-left bar: snapped edges land on whole screen pixels at awkward scales")
{
    const float rates[] = {1.875f, 2.25f, 1.0f, 2.0f, 1.3333f};
    for (const float rate : rates)
    {
        const Bar::SnappedRect snapped =
            Bar::SnapRect(Bar::Rect{2, 2, Bar::kFrameWidth, Bar::kFrameHeight}, rate, rate, 0.0f, 0.0f);
        const float left = snapped.x * rate;
        const float right = (snapped.x + snapped.width) * rate;
        const float top = snapped.y * rate;
        const float bottom = (snapped.y + snapped.height) * rate;
        CHECK(left == doctest::Approx(std::round(left)).epsilon(1e-4));
        CHECK(right == doctest::Approx(std::round(right)).epsilon(1e-4));
        CHECK(top == doctest::Approx(std::round(top)).epsilon(1e-4));
        CHECK(bottom == doctest::Approx(std::round(bottom)).epsilon(1e-4));
    }
}

TEST_CASE("top-left bar: neighbouring rectangles share their edge after snapping")
{
    const float rate = 1.875f;
    const Bar::SnappedRect first = Bar::SnapRect(Bar::Rect{10, 0, 7, 5}, rate, rate, 0.0f, 0.0f);
    const Bar::SnappedRect second = Bar::SnapRect(Bar::Rect{17, 0, 7, 5}, rate, rate, 0.0f, 0.0f);
    CHECK(first.x + first.width == doctest::Approx(second.x));
}

TEST_CASE("top-left bar: snapping respects the screen offset")
{
    const float edge = Bar::SnapEdge(10.0f, 1.875f, 0.4f);
    const float screen = edge * 1.875f + 0.4f;
    CHECK(screen == doctest::Approx(std::round(screen)).epsilon(1e-4));
}
