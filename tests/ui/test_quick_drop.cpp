#include <doctest.h>

#include "GameLogic/Items/QuickDrop.h"

using namespace GameLogic::Items::QuickDrop;

TEST_CASE("an ordinary item may be quick-dropped [ui][quick_drop]")
{
    CHECK(Evaluate(Candidate{}) == Verdict::Allowed);
}

TEST_CASE("every blocking rule refuses the quick drop [ui][quick_drop]")
{
    Candidate dead;
    dead.heroDead = true;
    CHECK(Evaluate(dead) == Verdict::HeroDead);

    Candidate valuable;
    valuable.highValue = true;
    CHECK(Evaluate(valuable) == Verdict::HighValue);

    Candidate reinforced;
    reinforced.reinforced = true;
    CHECK(Evaluate(reinforced) == Verdict::Reinforced);

    Candidate banned;
    banned.dropBanned = true;
    CHECK(Evaluate(banned) == Verdict::DropBanned);

    Candidate lostMap;
    lostMap.cursedTempleLostMap = true;
    CHECK(Evaluate(lostMap) == Verdict::CursedTempleLostMap);
}

TEST_CASE("a dead hero is reported before the item rules [ui][quick_drop]")
{
    Candidate everything{true, true, true, true, true};
    CHECK(Evaluate(everything) == Verdict::HeroDead);
}

TEST_CASE("only bag slots count, never equipped gear [ui][quick_drop]")
{
    constexpr int FirstBagSlot = 12;
    constexpr int EndBagSlot = 76;

    CHECK_FALSE(IsBagSlot(0, FirstBagSlot, EndBagSlot));
    CHECK_FALSE(IsBagSlot(FirstBagSlot - 1, FirstBagSlot, EndBagSlot));
    CHECK(IsBagSlot(FirstBagSlot, FirstBagSlot, EndBagSlot));
    CHECK(IsBagSlot(EndBagSlot - 1, FirstBagSlot, EndBagSlot));
    CHECK_FALSE(IsBagSlot(EndBagSlot, FirstBagSlot, EndBagSlot));
    CHECK_FALSE(IsBagSlot(-1, FirstBagSlot, EndBagSlot));
}
