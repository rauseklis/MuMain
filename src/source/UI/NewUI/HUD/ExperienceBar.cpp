#include "UI/NewUI/HUD/ExperienceBar.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr long long kNormalCurveFactor = 10;
constexpr long long kOverLevelFactor = 1000;
constexpr long long kNormalLevelCap = 255;
constexpr long long kMasterLevelBase = 400;
constexpr long long kMasterCurveOffset = 3892250000LL;
constexpr long long kCurveLevelOffset = 9;

long long CurveAt(long long level, long long factor)
{
    return (kCurveLevelOffset + level) * level * level * factor;
}
} // namespace

long long UI::Hud::ExperienceBar::NormalLevelStartExperience(long long level)
{
    const long long prior = level - 1;
    if (prior <= 0)
        return 0;

    long long experience = CurveAt(prior, kNormalCurveFactor);
    if (prior > kNormalLevelCap)
        experience += CurveAt(prior - kNormalLevelCap, kOverLevelFactor);
    return experience;
}

long long UI::Hud::ExperienceBar::MasterLevelStartExperience(long long masterLevel)
{
    const long long total = masterLevel + kMasterLevelBase;
    const long long over = total - kNormalLevelCap;
    const long long curve = CurveAt(total, kNormalCurveFactor) + CurveAt(over, kOverLevelFactor);
    return (curve - kMasterCurveOffset) / 2;
}

UI::Hud::ExperienceBar::Range UI::Hud::ExperienceBar::MakeRange(long long lower, long long next)
{
    return {lower, std::max(lower, next)};
}

double UI::Hud::ExperienceBar::Ratio(long long current, Range range)
{
    const long long span = range.upper - range.lower;
    if (span <= 0)
        return 0.0;

    const long long done = std::clamp(current, range.lower, range.upper) - range.lower;
    return std::clamp(static_cast<double>(done) / static_cast<double>(span), 0.0, 1.0);
}

int UI::Hud::ExperienceBar::Segment(long long current, Range range)
{
    const long long span = range.upper - range.lower;
    if (span <= 0)
        return 0;

    const long long done = std::clamp(current, range.lower, range.upper) - range.lower;
    if (done >= span)
        return kSegments - 1;

    int segment = 0;
    if (done <= std::numeric_limits<long long>::max() / kSegments)
        segment = static_cast<int>(done * kSegments / span);
    else
        segment = static_cast<int>(static_cast<double>(done) / static_cast<double>(span) * kSegments);
    return std::clamp(segment, 0, kSegments - 1);
}

float UI::Hud::ExperienceBar::FillWidth(double ratio, float fullWidth)
{
    if (!(ratio > 0.0))
        return 0.0f;
    return static_cast<float>(std::min(ratio, 1.0) * static_cast<double>(fullWidth));
}

double UI::Hud::ExperienceBar::Advance(double displayed, double target, double seconds)
{
    const double goal = std::isfinite(target) ? std::clamp(target, 0.0, 1.0) : 0.0;
    if (!std::isfinite(displayed) || goal <= displayed)
        return goal;
    if (!(seconds > 0.0))
        return displayed;

    const double next = displayed + (goal - displayed) * (1.0 - std::exp(-seconds / kFillTauSeconds));
    return (goal - next < kSnapDistance) ? goal : next;
}
