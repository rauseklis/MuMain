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
