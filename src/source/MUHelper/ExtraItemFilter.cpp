#include "ExtraItemFilter.h"

#include <algorithm>
#include <cwctype>

namespace MUHelper::ExtraItemFilter
{

namespace
{
constexpr std::wstring_view Whitespace = L" \t";

std::wstring_view Trim(std::wstring_view text)
{
    const std::size_t first = text.find_first_not_of(Whitespace);
    if (first == std::wstring_view::npos)
    {
        return {};
    }

    const std::size_t last = text.find_last_not_of(Whitespace);
    return text.substr(first, last - first + 1);
}

bool SameLetterIgnoringCase(wchar_t left, wchar_t right)
{
    return std::towlower(static_cast<std::wint_t>(left)) == std::towlower(static_cast<std::wint_t>(right));
}
} // namespace

bool Matches(std::wstring_view itemName, std::wstring_view entry)
{
    const std::wstring_view trimmedEntry = Trim(entry);
    if (trimmedEntry.empty())
    {
        return false;
    }

    const auto found =
        std::search(itemName.begin(), itemName.end(), trimmedEntry.begin(), trimmedEntry.end(), SameLetterIgnoringCase);
    return found != itemName.end();
}

bool MatchesAny(std::initializer_list<std::wstring_view> itemNames, const std::set<std::wstring>& entries)
{
    for (const std::wstring& entry : entries)
    {
        for (const std::wstring_view itemName : itemNames)
        {
            if (Matches(itemName, entry))
            {
                return true;
            }
        }
    }

    return false;
}

} // namespace MUHelper::ExtraItemFilter
