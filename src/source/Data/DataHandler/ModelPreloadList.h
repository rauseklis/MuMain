#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Data::Models
{
// What kind of model a map needs: an NPC (an id of the MODEL_NPC range) or a monster (an EMonsterModelType).
enum class PreloadKind
{
    Npc,
    Monster,
};

struct PreloadEntry
{
    PreloadKind kind;
    int id;

    bool operator==(const PreloadEntry& other) const = default;
};

// Remembers which NPC and monster models each map needed, so they can be loaded while the map loads instead of
// the first time an NPC comes into view. Pure data; the file handling is in ModelPreloadStore.
class ModelPreloadList
{
public:
    // The most entries kept per map; a damaged file cannot make a map load everything.
    static constexpr std::size_t MaxEntriesPerMap = 256;

    // Returns whether the entry was new (and so the list changed).
    bool Add(int map, PreloadEntry entry);

    // The entries of the map in the order they were added; empty for an unknown map.
    std::vector<PreloadEntry> EntriesFor(int map) const;

    // One line per entry: "<map> <N|M> <id>". Parse ignores lines it cannot read.
    std::string Serialize() const;
    static ModelPreloadList Parse(std::string_view text);

private:
    std::map<int, std::vector<PreloadEntry>> m_entries;
};
} // namespace Data::Models
