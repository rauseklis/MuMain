#pragma once

namespace MUHelper
{

// Why the MU Helper picks a dropped item up. It is written to MuError.log with every pickup, so a player can see
// which of their pickup settings took an item they did not want.
enum class ObtainReason
{
    None,
    Zen,
    Jewel,
    Ancient,
    Excellent,
    ExtraItem,
    PickAll,
};

[[nodiscard]] constexpr const char* ObtainReasonName(ObtainReason reason)
{
    switch (reason)
    {
    case ObtainReason::Zen:
        return "zen";
    case ObtainReason::Jewel:
        return "jewel";
    case ObtainReason::Ancient:
        return "set item";
    case ObtainReason::Excellent:
        return "excellent";
    case ObtainReason::ExtraItem:
        return "extra item list";
    case ObtainReason::PickAll:
        return "pick all near items";
    case ObtainReason::None:
        break;
    }

    return "none";
}

} // namespace MUHelper
