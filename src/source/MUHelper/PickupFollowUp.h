#pragma once

#include "GameLogic/Automation/Pickup.h"

// What the MU Helper does after one pickup attempt.
namespace MUHelper::PickupFollowUp
{

enum class Action
{
    // Stop tracking the drop: it was requested, is gone, or cannot be reached.
    ForgetDrop,
    // Keep tracking the drop, but choose the closest wanted drop again on the next tick.
    ChooseAgainLater,
    // Keep going for the same drop.
    KeepTarget,
};

struct Step
{
    Action action;
    // False while walking to the drop: hunting waits until the character arrives.
    bool continueOtherWork;
};

[[nodiscard]] constexpr Step After(GameLogic::Automation::PickupResult result)
{
    using GameLogic::Automation::PickupResult;

    switch (result)
    {
    case PickupResult::Requested:
    case PickupResult::Gone:
    case PickupResult::NoPath:
        return {Action::ForgetDrop, true};
    case PickupResult::Approaching:
        return {Action::KeepTarget, false};
    case PickupResult::TooFar:
        return {Action::ChooseAgainLater, true};
    case PickupResult::Busy:
        return {Action::KeepTarget, true};
    }

    return {Action::ChooseAgainLater, true};
}

} // namespace MUHelper::PickupFollowUp
