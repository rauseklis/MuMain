#pragma once

// The experience bar under the bottom interface: the maths of what it shows and how the fill moves.
// The bar fills from the experience needed for the current level (lower) to the experience needed for the next
// level (upper). All values are 64 bit; the ratio is computed in double (exact up to 2^53) so large values neither
// overflow nor jump.
namespace UI::Hud::ExperienceBar
{
inline constexpr int kSegments = 10;
// The fill closes 99 % of the remaining distance in about 0.55 s.
inline constexpr double kFillTauSeconds = 0.12;
// Below this remaining distance (a third of a logical pixel on the 620 wide bar) the fill snaps to its target.
inline constexpr double kSnapDistance = 0.0005;

struct Range
{
    long long lower = 0;
    long long upper = 0;
};

// Experience at which a normal level starts (the experience the previous level needed).
long long NormalLevelStartExperience(long long level);
// Experience at which a master level starts.
long long MasterLevelStartExperience(long long masterLevel);

// An upper bound below the lower bound (data not received yet) is treated as an empty range.
Range MakeRange(long long lower, long long next);

// Fraction of the range that is done, 0.0 to 1.0. An empty range gives 0.
double Ratio(long long current, Range range);
// Which tenth of the range the experience is in, 0 to 9. A finished range gives 9.
int Segment(long long current, Range range);
// Width of the fill in the same unit as fullWidth. Never below 0 and never above fullWidth.
float FillWidth(double ratio, float fullWidth);
// Moves the displayed ratio towards the target. A target below the displayed ratio (level up, experience lost) is
// taken at once; a higher target is approached smoothly.
double Advance(double displayed, double target, double seconds);
} // namespace UI::Hud::ExperienceBar
