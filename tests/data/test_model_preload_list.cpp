#include <doctest.h>

#include "Data/DataHandler/ModelPreloadList.h"

using Data::Models::ModelPreloadList;
using Data::Models::PreloadEntry;
using Data::Models::PreloadKind;

TEST_CASE("an entry is added once per map [data][preload]")
{
    ModelPreloadList list;
    CHECK(list.Add(0, {PreloadKind::Npc, 1045}));
    CHECK_FALSE(list.Add(0, {PreloadKind::Npc, 1045}));
    CHECK(list.Add(0, {PreloadKind::Monster, 1045}));
    CHECK(list.Add(2, {PreloadKind::Npc, 1045}));
    CHECK(list.EntriesFor(0).size() == 2);
    CHECK(list.EntriesFor(2).size() == 1);
    CHECK(list.EntriesFor(7).empty());
}

TEST_CASE("the list round-trips through its text form [data][preload]")
{
    ModelPreloadList list;
    list.Add(0, {PreloadKind::Npc, 1120});
    list.Add(0, {PreloadKind::Monster, 12});
    list.Add(2, {PreloadKind::Npc, 1055});

    const ModelPreloadList loaded = ModelPreloadList::Parse(list.Serialize());

    CHECK(loaded.EntriesFor(0) == list.EntriesFor(0));
    CHECK(loaded.EntriesFor(2) == list.EntriesFor(2));
}

TEST_CASE("unreadable lines are skipped [data][preload]")
{
    const ModelPreloadList loaded = ModelPreloadList::Parse("garbage\n0 X 5\n0 N\n3 N 1048\n\n3 M 7\n");

    CHECK(loaded.EntriesFor(0).empty());
    REQUIRE(loaded.EntriesFor(3).size() == 2);
    CHECK(loaded.EntriesFor(3)[0] == PreloadEntry{PreloadKind::Npc, 1048});
}

TEST_CASE("a map keeps at most MaxEntriesPerMap entries [data][preload]")
{
    ModelPreloadList list;
    for (int id = 0; id < static_cast<int>(ModelPreloadList::MaxEntriesPerMap) + 10; ++id)
    {
        list.Add(1, {PreloadKind::Npc, id});
    }

    CHECK(list.EntriesFor(1).size() == ModelPreloadList::MaxEntriesPerMap);
}
