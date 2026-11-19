#include "doctest.h"

#include <set>
#include <string>

#include "MUHelper/ExtraItemFilter.h"
#include "MUHelper/PickupFollowUp.h"

namespace Filter = MUHelper::ExtraItemFilter;
namespace FollowUp = MUHelper::PickupFollowUp;
using GameLogic::Automation::PickupResult;

TEST_CASE("an extra item entry matches the name shown on the ground")
{
    // A Box of Luck +5 is labelled "Silver medal" on the ground.
    const std::set<std::wstring> patterns{L"Silver medal"};
    CHECK(Filter::MatchesAny({L"Box of Luck +5", L"Silver medal"}, patterns));
}

TEST_CASE("an extra item entry ignores upper and lower case")
{
    CHECK(Filter::Matches(L"Silver medal", L"silver MEDAL"));
    CHECK(Filter::Matches(L"Box of Kundun +3", L"box of kundun"));
    CHECK(Filter::Matches(L"Box of Luck +5", L"BOX"));
}

TEST_CASE("an extra item entry matches part of a name")
{
    CHECK(Filter::Matches(L"Gold medal", L"medal"));
    CHECK(Filter::Matches(L"Box of Kundun +3", L"Kundun +3"));
    CHECK_FALSE(Filter::Matches(L"Box of Kundun +3", L"Kundun +4"));
    CHECK_FALSE(Filter::Matches(L"Jewel of Bless", L"Silver"));
}

TEST_CASE("spaces around an extra item entry are ignored, an empty entry matches nothing")
{
    CHECK(Filter::Matches(L"Silver medal", L"  Silver medal "));
    CHECK_FALSE(Filter::Matches(L"Silver medal", L""));
    CHECK_FALSE(Filter::Matches(L"Silver medal", L"   "));
}

TEST_CASE("no entry means no extra item is picked")
{
    const std::set<std::wstring> none;
    CHECK_FALSE(Filter::MatchesAny({L"Box of Luck +5", L"Silver medal"}, none));
}

TEST_CASE("a drop out of range does not keep the helper waiting for it")
{
    const FollowUp::Step step = FollowUp::After(PickupResult::TooFar);
    CHECK(step.action == FollowUp::Action::ChooseAgainLater);
    CHECK(step.continueOtherWork);
}

TEST_CASE("a drop the character cannot walk to is given up, and hunting goes on")
{
    const FollowUp::Step step = FollowUp::After(PickupResult::NoPath);
    CHECK(step.action == FollowUp::Action::ForgetDrop);
    CHECK(step.continueOtherWork);
}

TEST_CASE("the other pickup results keep their earlier behaviour")
{
    CHECK(FollowUp::After(PickupResult::Requested).action == FollowUp::Action::ForgetDrop);
    CHECK(FollowUp::After(PickupResult::Requested).continueOtherWork);
    CHECK(FollowUp::After(PickupResult::Gone).action == FollowUp::Action::ForgetDrop);
    CHECK(FollowUp::After(PickupResult::Gone).continueOtherWork);
    CHECK(FollowUp::After(PickupResult::Approaching).action == FollowUp::Action::KeepTarget);
    CHECK_FALSE(FollowUp::After(PickupResult::Approaching).continueOtherWork);
    CHECK(FollowUp::After(PickupResult::Busy).action == FollowUp::Action::KeepTarget);
    CHECK(FollowUp::After(PickupResult::Busy).continueOtherWork);
}
