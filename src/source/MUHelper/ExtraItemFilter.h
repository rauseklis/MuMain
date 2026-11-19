#pragma once

#include <initializer_list>
#include <set>
#include <string>
#include <string_view>

// The MU Helper's "Add Extra Item" list: which dropped items it picks up by name.
namespace MUHelper::ExtraItemFilter
{

// True when the entry appears in the item name. Upper and lower case do not matter, spaces around the entry
// are ignored, and an empty entry matches nothing.
[[nodiscard]] bool Matches(std::wstring_view itemName, std::wstring_view entry);

// True when any entry matches any of the item's names (for example its item name and the name shown on the ground).
[[nodiscard]] bool MatchesAny(std::initializer_list<std::wstring_view> itemNames, const std::set<std::wstring>& entries);

} // namespace MUHelper::ExtraItemFilter
