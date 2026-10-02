#include "doctest.h"

#include "Render/Effects/TailScaling.h"

#include <cmath>

using Render::Effects::TailScaling::ComputeMaxTails;
using Render::Effects::TailScaling::MinReliableFps;

TEST_CASE("tail scaling inflates MaxTails for real high-FPS play")
{
    bool pending = true;
    // 240 FPS steady state: FPS_ANIMATION_FACTOR = 25/240 ~= 0.1042.
    const int result = ComputeMaxTails(30, 240.0, 25.0f / 240.0f, 200, pending);

    CHECK_FALSE(pending);
    CHECK(result == 200); // 30 / 0.1042 ~= 288, clamped to the 200 cap.
}

TEST_CASE("tail scaling leaves MaxTails at its base value at the legacy 25 FPS cadence")
{
    bool pending = true;
    const int result = ComputeMaxTails(30, 25.0, 1.0f, 200, pending);

    CHECK_FALSE(pending);
    CHECK(result == 30);
}

TEST_CASE("tail scaling defers instead of baking in a stall's instantaneous FPS")
{
    bool pending = false;
    // The real repro: a map-load stall read back as 3.3 FPS for one frame.
    const int result = ComputeMaxTails(30, 3.3, 1.0f, 200, pending);

    CHECK(pending);
    CHECK(result == 30); // Unscaled base value, not a corrupted tiny number.
}

TEST_CASE("tail scaling treats the reliable-FPS floor as inclusive")
{
    bool pendingAtFloor = true;
    const int atFloor = ComputeMaxTails(30, Render::Effects::TailScaling::MinReliableFps, 1.0f, 200, pendingAtFloor);
    CHECK_FALSE(pendingAtFloor);
    CHECK(atFloor == 30);

    bool pendingJustBelow = false;
    const int justBelow =
        ComputeMaxTails(30, std::nextafter(MinReliableFps, 0.0), 1.0f, 200, pendingJustBelow);
    CHECK(pendingJustBelow);
    CHECK(justBelow == 30);
}

TEST_CASE("a joint created during a stall recovers once a reliable frame arrives")
{
    // CreateJoint's call, on the bad frame.
    bool pending = false;
    int maxTails = ComputeMaxTails(30, 3.3, 1.0f, 200, pending);
    CHECK(pending);
    CHECK(maxTails == 30);

    // MoveJoint's retry, one frame later, once the client has recovered to its real steady state.
    maxTails = ComputeMaxTails(maxTails, 240.0, 25.0f / 240.0f, 200, pending);
    CHECK_FALSE(pending);
    CHECK(maxTails == 200);
}

TEST_CASE("a joint created during a stall keeps retrying across multiple bad frames")
{
    bool pending = false;
    int maxTails = ComputeMaxTails(30, 3.3, 1.0f, 200, pending);
    CHECK(pending);

    // The stall is still ongoing one frame later.
    maxTails = ComputeMaxTails(maxTails, 5.0, 1.0f, 200, pending);
    CHECK(pending);
    CHECK(maxTails == 30); // Still the unscaled base, not corrupted further.

    // Recovers on the first genuinely reliable frame.
    maxTails = ComputeMaxTails(maxTails, 203.4, 25.0f / 203.4f, 200, pending);
    CHECK_FALSE(pending);
    CHECK(maxTails > 150); // Comfortably back in the dense-weave range.
}

namespace
{
// FPS_ANIMATION_FACTOR as CalcFPS() computes it: clamp(25 / FPS, 0, 1).
float FactorForFps(double fps)
{
    const double ratio = 25.0 / fps;
    return static_cast<float>(ratio > 1.0 ? 1.0 : ratio);
}
} // namespace

TEST_CASE("a pending joint ratchets up through the real post-load recovery ramp and converges to the cap")
{
    using namespace Render::Effects::TailScaling;

    // Real data from a live session: created at 1.3 FPS, then the FPS counter climbs 29.2, 60.5,
    // 108.4 and settles at ~250. Applying the scaling once on the first "reliable" frame (29.2)
    // baked in 35; the ratchet must instead end at the cap.
    const int base = 30;
    int maxTails = base;
    const double ramp[] = {1.3, 29.2, 60.5, 108.4, 250.0};

    int previous = maxTails;
    for (const double fps : ramp)
    {
        maxTails = RatchetMaxTails(maxTails, base, FactorForFps(fps), 200);
        CHECK(maxTails >= previous); // never decreases
        previous = maxTails;
    }

    CHECK(maxTails == 200);
}

TEST_CASE("the ratchet never lowers MaxTails when FPS dips again mid-recovery")
{
    using namespace Render::Effects::TailScaling;

    int maxTails = 30;
    maxTails = RatchetMaxTails(maxTails, 30, FactorForFps(108.4), 200);
    const int reached = maxTails;
    CHECK(reached > 100);

    maxTails = RatchetMaxTails(maxTails, 30, FactorForFps(2.7), 200); // a hitch
    CHECK(maxTails == reached);
    maxTails = RatchetMaxTails(maxTails, 30, FactorForFps(60.5), 200);
    CHECK(maxTails == reached);
}

TEST_CASE("a player who genuinely sits at a low frame rate converges to their real value, not the cap")
{
    using namespace Render::Effects::TailScaling;

    int maxTails = 30;
    for (int frame = 0; frame < 10; ++frame)
    {
        maxTails = RatchetMaxTails(maxTails, 30, FactorForFps(25.0), 200);
    }
    CHECK(maxTails == 30);

    for (int frame = 0; frame < 10; ++frame)
    {
        maxTails = RatchetMaxTails(maxTails, 30, FactorForFps(60.0), 200);
    }
    CHECK(maxTails == 72); // 30 / (25/60)
}

TEST_CASE("the ratchet matches the creation-time formula in steady state at and above the cap's FPS")
{
    using namespace Render::Effects::TailScaling;

    // The formula reaches the 200 cap at ~167 FPS; creation-time scaling and the ratchet agree there
    // and above, and creation on a reliable frame is not pending, so nothing changes for normal joints.
    for (const double fps : {167.0, 203.4, 250.0, 500.0})
    {
        bool pending = true;
        const int atCreation = ComputeMaxTails(30, fps, FactorForFps(fps), 200, pending);
        CHECK_FALSE(pending);
        CHECK(atCreation == 200);
        CHECK(RatchetMaxTails(30, 30, FactorForFps(fps), 200) == atCreation);
    }
}

TEST_CASE("recovery finishes at the cap or when the bounded window elapses")
{
    using namespace Render::Effects::TailScaling;

    CHECK(IsRecoveryDone(200, 200, 100.0, 2100.0));         // reached the cap early
    CHECK_FALSE(IsRecoveryDone(130, 200, 100.0, 2100.0));   // still ramping, window open
    CHECK(IsRecoveryDone(130, 200, 2100.0, 2100.0));        // window elapsed, keep what it reached
    CHECK(RecoveryWindowMs > 0.0);
}
