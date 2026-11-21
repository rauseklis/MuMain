#include "doctest.h"

#include "MUHelper/AttackFollowUp.h"
#include "MUHelper/HelperClock.h"

using MUHelper::HelperClock;

TEST_CASE("the helper timer fires faster than the full work runs")
{
    CHECK(HelperClock::TickMs < HelperClock::FullWorkIntervalMs);
}

TEST_CASE("the full work runs on the first tick and then once per interval")
{
    HelperClock clock;
    clock.Start(10'000);

    CHECK(clock.TakeFullWorkDue(10'050));
    CHECK_FALSE(clock.TakeFullWorkDue(10'100));
    CHECK_FALSE(clock.TakeFullWorkDue(10'299));
    CHECK(clock.TakeFullWorkDue(10'300));
    CHECK_FALSE(clock.TakeFullWorkDue(10'350));
}

TEST_CASE("the full work is not sent more often when the timer fires often")
{
    HelperClock clock;
    clock.Start(0);

    int fullWorks = 0;
    for (std::uint64_t now = HelperClock::TickMs; now <= 10'000; now += HelperClock::TickMs)
    {
        fullWorks += clock.TakeFullWorkDue(now) ? 1 : 0;
    }

    // 10 seconds at one full work per 250 ms.
    CHECK(fullWorks == 40);
}

TEST_CASE("helper seconds are real seconds")
{
    HelperClock clock;
    clock.Start(5'000);

    CHECK(clock.SecondsElapsed(5'000) == 0);
    CHECK(clock.SecondsElapsed(5'999) == 0);
    CHECK(clock.SecondsElapsed(6'000) == 1);
    // The old counter needed 12.5 seconds to count 10.
    CHECK(clock.SecondsElapsed(15'000) == 10);
}

TEST_CASE("a restart counts the seconds from the new start")
{
    HelperClock clock;
    clock.Start(0);
    CHECK(clock.SecondsElapsed(30'000) == 30);

    clock.Start(30'000);
    CHECK(clock.SecondsElapsed(30'500) == 0);
    CHECK(clock.TakeFullWorkDue(30'050));
}

TEST_CASE("the next attack follows a hit as soon as the swing ends")
{
    MUHelper::AttackFollowUp followUp;
    followUp.OnAttack(true, true);

    CHECK_FALSE(followUp.IsDue(true));
    CHECK_FALSE(followUp.IsDue(true));
    CHECK(followUp.IsDue(false));
}

TEST_CASE("a swing that starts on a later tick is followed up too")
{
    MUHelper::AttackFollowUp followUp;
    followUp.OnAttack(true, false);

    CHECK_FALSE(followUp.IsDue(true));
    CHECK(followUp.IsDue(false));
}

TEST_CASE("a hit that shows no swing waits for the full work")
{
    // Otherwise such a cast would be sent on every 50 ms tick.
    MUHelper::AttackFollowUp followUp;
    followUp.OnAttack(true, false);

    CHECK_FALSE(followUp.IsDue(false));
    CHECK_FALSE(followUp.IsDue(false));
}

TEST_CASE("without a hit the next attack waits for the full work")
{
    MUHelper::AttackFollowUp followUp;
    CHECK_FALSE(followUp.IsDue(false));

    // Walking to the target, a refused skill or no target.
    followUp.OnAttack(false, false);
    CHECK_FALSE(followUp.IsDue(true));
    CHECK_FALSE(followUp.IsDue(false));
}

TEST_CASE("pickup or walking back stops the follow-up attacks")
{
    MUHelper::AttackFollowUp followUp;
    followUp.OnAttack(true, true);
    followUp.Clear();

    CHECK_FALSE(followUp.IsDue(false));
}
