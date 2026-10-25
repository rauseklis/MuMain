#include "Data/DataHandler/ModelPreloadList.h"

#include <algorithm>
#include <charconv>
#include <sstream>

namespace Data::Models
{
namespace
{
constexpr char NpcCode = 'N';
constexpr char MonsterCode = 'M';

char CodeOf(PreloadKind kind)
{
    return kind == PreloadKind::Npc ? NpcCode : MonsterCode;
}
} // namespace

bool ModelPreloadList::Add(int map, PreloadEntry entry)
{
    std::vector<PreloadEntry>& entries = m_entries[map];
    if (entries.size() >= MaxEntriesPerMap || std::ranges::find(entries, entry) != entries.end())
    {
        return false;
    }

    entries.push_back(entry);
    return true;
}

std::vector<PreloadEntry> ModelPreloadList::EntriesFor(int map) const
{
    const auto found = m_entries.find(map);
    return found == m_entries.end() ? std::vector<PreloadEntry>{} : found->second;
}

std::string ModelPreloadList::Serialize() const
{
    std::ostringstream text;
    for (const auto& [map, entries] : m_entries)
    {
        for (const PreloadEntry& entry : entries)
        {
            text << map << ' ' << CodeOf(entry.kind) << ' ' << entry.id << '\n';
        }
    }
    return text.str();
}

ModelPreloadList ModelPreloadList::Parse(std::string_view text)
{
    ModelPreloadList list;
    std::istringstream lines{std::string(text)};
    std::string line;
    while (std::getline(lines, line))
    {
        std::istringstream fields(line);
        int map = 0;
        int id = 0;
        char code = 0;
        if (!(fields >> map >> code >> id) || (code != NpcCode && code != MonsterCode))
        {
            continue;
        }

        list.Add(map, {code == NpcCode ? PreloadKind::Npc : PreloadKind::Monster, id});
    }
    return list;
}
} // namespace Data::Models
