#include "stdafx.h"
#include "Data/DataHandler/ModelPreloadStore.h"

#include <fstream>
#include <iterator>
#include <string>

#include "Engine/Object/ZzzOpenData.h"

namespace Data::Models
{
namespace
{
constexpr const char* PreloadFileName = "ModelPreload.txt";

ModelPreloadList& List()
{
    static ModelPreloadList list = []
    {
        std::ifstream file(PreloadFileName);
        const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return ModelPreloadList::Parse(text);
    }();
    return list;
}

bool IsKnownId(const PreloadEntry& entry)
{
    if (entry.kind == PreloadKind::Npc)
    {
        return entry.id >= MODEL_NPC_BEGIN && entry.id < MODEL_NPC_END;
    }
    return entry.id >= 0 && entry.id < MONSTER_MODEL_COUNT;
}

void SaveList()
{
    std::ofstream file(PreloadFileName, std::ios::trunc);
    file << List().Serialize();
}
} // namespace

void RememberModelForMap(int map, PreloadEntry entry)
{
    if (map < 0 || !IsKnownId(entry) || !List().Add(map, entry))
    {
        return;
    }

    SaveList();
}

void PreloadModelsForMap(int map)
{
    for (const PreloadEntry& entry : List().EntriesFor(map))
    {
        if (!IsKnownId(entry))
        {
            continue;
        }

        if (entry.kind == PreloadKind::Npc)
        {
            OpenNpc(entry.id);
        }
        else
        {
            OpenMonsterModel(static_cast<EMonsterModelType>(entry.id));
        }
    }
}
} // namespace Data::Models
