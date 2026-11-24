#include <doctest.h>

#include "UI/NewUI/HUD/ExperienceBar.h"

namespace EB = UI::Hud::ExperienceBar;

TEST_CASE("experience bar ratio at the edges of a level [ui][experience_bar]")
{
    const EB::Range range = EB::MakeRange(1000, 2000);
    CHECK(EB::Ratio(1000, range) == doctest::Approx(0.0));
    CHECK(EB::Ratio(1001, range) == doctest::Approx(0.001));
    CHECK(EB::Ratio(1999, range) == doctest::Approx(0.999));
    CHECK(EB::Ratio(2000, range) == doctest::Approx(1.0));
    CHECK(EB::Ratio(500, range) == doctest::Approx(0.0));
    CHECK(EB::Ratio(5000, range) == doctest::Approx(1.0));
}

TEST_CASE("experience bar uses 64 bit values above 2^31 [ui][experience_bar]")
{
    // A real character: level 400, EXP 3822148080 / 3892250000. 3822148080 is exactly where level 400 starts.
    const long long lower = EB::NormalLevelStartExperience(400);
    CHECK(lower == 3822148080LL);
    const EB::Range range = EB::MakeRange(lower, 3892250000LL);
    CHECK(EB::Ratio(3822148080LL, range) == doctest::Approx(0.0));
    const long long gained = 3857199040LL;
    const double expected = static_cast<double>(gained - lower) / static_cast<double>(3892250000LL - lower);
    CHECK(EB::Ratio(gained, range) == doctest::Approx(expected));
    CHECK(EB::Ratio(gained, range) == doctest::Approx(0.5).epsilon(0.001));
    CHECK(EB::FillWidth(EB::Ratio(gained, range), 620.0f) == doctest::Approx(expected * 620.0));

    const EB::Range huge = EB::MakeRange(4000000000000LL, 4100000000000LL);
    CHECK(EB::Ratio(4050000000000LL, huge) == doctest::Approx(0.5));
    CHECK(EB::Segment(4050000000000LL, huge) == 5);
}

TEST_CASE("experience bar fill is monotonic with no jumps or stalls [ui][experience_bar]")
{
    const EB::Range range = EB::MakeRange(3000000000LL, 3900000000LL);
    constexpr long long kStep = 1000003;
    const double maxJump = static_cast<double>(kStep) / static_cast<double>(range.upper - range.lower) + 1e-12;
    double previous = 0.0;
    for (long long experience = range.lower; experience <= range.upper; experience += kStep)
    {
        const double ratio = EB::Ratio(experience, range);
        CHECK(ratio >= previous);
        CHECK(ratio - previous <= maxJump);
        previous = ratio;
    }
}

TEST_CASE("experience bar segments follow the tenths of the range [ui][experience_bar]")
{
    const EB::Range range = EB::MakeRange(0, 1000);
    CHECK(EB::Segment(0, range) == 0);
    CHECK(EB::Segment(99, range) == 0);
    CHECK(EB::Segment(100, range) == 1);
    CHECK(EB::Segment(899, range) == 8);
    CHECK(EB::Segment(900, range) == 9);
    CHECK(EB::Segment(999, range) == 9);
    CHECK(EB::Segment(1000, range) == 9);
    CHECK(EB::Segment(-5, range) == 0);
}

TEST_CASE("experience bar tenth boundaries line up with the fill [ui][experience_bar]")
{
    const EB::Range range = EB::MakeRange(0, 1000);
    for (int tenth = 1; tenth < EB::kSegments; ++tenth)
    {
        const long long experience = tenth * 100;
        CHECK(EB::Segment(experience, range) == tenth);
        CHECK(EB::FillWidth(EB::Ratio(experience, range), 620.0f) == doctest::Approx(tenth * 62.0));
    }
}

TEST_CASE("experience bar degenerate ranges give an empty bar [ui][experience_bar]")
{
    CHECK(EB::Ratio(5, EB::MakeRange(100, 100)) == doctest::Approx(0.0));
    CHECK(EB::Ratio(5, EB::MakeRange(100, 0)) == doctest::Approx(0.0));
    CHECK(EB::Segment(5, EB::MakeRange(100, 0)) == 0);
}

TEST_CASE("experience bar fill width is clamped [ui][experience_bar]")
{
    CHECK(EB::FillWidth(-1.0, 620.0f) == 0.0f);
    CHECK(EB::FillWidth(0.0, 620.0f) == 0.0f);
    CHECK(EB::FillWidth(0.5, 620.0f) == doctest::Approx(310.0));
    CHECK(EB::FillWidth(1.0, 620.0f) == doctest::Approx(620.0));
    CHECK(EB::FillWidth(7.0, 620.0f) == doctest::Approx(620.0));
}

TEST_CASE("experience level starts match the server curve [ui][experience_bar]")
{
    CHECK(EB::NormalLevelStartExperience(1) == 0);
    CHECK(EB::NormalLevelStartExperience(2) == 100);
    CHECK(EB::NormalLevelStartExperience(3) == 10 * 2 * 2 * 11);
    // Past level 255 the extra curve starts: prior level 256 adds (9 + 1) * 1 * 1 * 1000.
    CHECK(EB::NormalLevelStartExperience(257) == (9LL + 256) * 256 * 256 * 10 + (9LL + 1) * 1 * 1 * 1000);
    CHECK(EB::NormalLevelStartExperience(400) > 3000000000LL);
    CHECK(EB::NormalLevelStartExperience(400) < 3892250000LL);
}

TEST_CASE("master experience start grows with the master level [ui][experience_bar]")
{
    CHECK(EB::MasterLevelStartExperience(1) > EB::MasterLevelStartExperience(0));
    CHECK(EB::MasterLevelStartExperience(200) > EB::MasterLevelStartExperience(199));
    CHECK(EB::MasterLevelStartExperience(400) > EB::MasterLevelStartExperience(399));
}

TEST_CASE("experience bar fill glides up and snaps down [ui][experience_bar]")
{
    double shown = 0.2;
    const double target = 0.6;
    double previous = shown;
    for (int frame = 0; frame < 120 && shown < target; ++frame)
    {
        shown = EB::Advance(shown, target, 1.0 / 60.0);
        CHECK(shown >= previous);
        CHECK(shown <= target);
        previous = shown;
    }
    CHECK(shown == doctest::Approx(target));

    // Not instant: one frame moves only part of the way.
    CHECK(EB::Advance(0.2, 0.6, 1.0 / 60.0) < 0.3);
    // Level up (ratio falls back) and lost experience are taken at once.
    CHECK(EB::Advance(0.97, 0.03, 1.0 / 60.0) == doctest::Approx(0.03));
    // No time passed: nothing moves.
    CHECK(EB::Advance(0.2, 0.6, 0.0) == doctest::Approx(0.2));
    // Bad input never produces a value outside 0..1.
    CHECK(EB::Advance(0.5, 2.0, 10.0) == doctest::Approx(1.0));
}
