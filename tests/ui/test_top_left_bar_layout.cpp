#include <doctest.h>

#include "UI/NewUI/HUD/TopLeftBarLayout.h"

namespace Bar = UI::HUD::TopLeftBar;

TEST_CASE("top-left bar: the buttons sit inside the frame without touching each other or the readout")
{
    const Bar::Rect frame{0, 0, Bar::kFrameWidth, Bar::kFrameHeight};
    const Bar::Button buttons[] = {Bar::Button::Settings, Bar::Button::MuHelper, Bar::Button::ExperienceHunter};

    int previousRight = Bar::kCoordinateRow.x + Bar::kCoordinateRow.width;
    for (const Bar::Button button : buttons)
    {
        const Bar::Rect rect = Bar::ButtonRect(button);
        CHECK(rect.x > previousRight);
        CHECK(rect.x + rect.width < frame.width);
        CHECK(rect.y >= 0);
        CHECK(rect.y + rect.height < frame.height);
        previousRight = rect.x + rect.width;
    }
}

TEST_CASE("top-left bar: the bar is at most about half as wide as the old 217 pixel bar")
{
    constexpr int oldWidth = 217;
    CHECK(Bar::kFrameWidth * 2 <= oldWidth + 2 * Bar::kMargin + 4);
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
            CHECK(rect.y >= Bar::kFrameHeight);
            CHECK(rect.x + rect.width <= Bar::kAtlasSize);
            CHECK(rect.y + rect.height <= Bar::kAtlasSize);
            CHECK(Bar::kFaceCell - rect.width >= 2); // an empty gutter against the neighbouring face
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
