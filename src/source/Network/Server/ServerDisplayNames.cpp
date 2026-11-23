#include "stdafx.h"
#include "ServerDisplayNames.h"

#include <cwchar>
#include <iterator>

namespace Network::ServerNames
{
namespace
{
    constexpr wchar_t GROUP_NAME_IN_FILE[] = L"Valhalla";
    constexpr wchar_t GROUP_NAME[] = L"Emberion";

    // The name of each server, in the order of the server numbers (1 = the first server of the group).
    // They have to match Description and ExperienceRate of the game servers on the server side.
    constexpr const wchar_t* SERVER_LABELS[] = {L"Inferno 500x", L"Ember 5x"};

    // The position in the server select menu, parallel to SERVER_LABELS: "Ember 5x" is listed above "Inferno 500x".
    constexpr int SERVER_DISPLAY_RANKS[] = {2, 1};
    static_assert(std::size(SERVER_DISPLAY_RANKS) == std::size(SERVER_LABELS));
}

const wchar_t* GroupName(const wchar_t* nameInFile)
{
    return wcscmp(nameInFile, GROUP_NAME_IN_FILE) == 0 ? GROUP_NAME : nameInFile;
}

bool ServerLabel(const wchar_t* groupName, int serverNumberInGroup, wchar_t* label, size_t labelSize)
{
    if (serverNumberInGroup < 1 || serverNumberInGroup > static_cast<int>(std::size(SERVER_LABELS)))
    {
        return false;
    }

    (void)groupName;
    swprintf(label, labelSize, L"%ls", SERVER_LABELS[serverNumberInGroup - 1]);
    return true;
}

int ServerDisplayRank(int serverNumberInGroup)
{
    const int knownServers = static_cast<int>(std::size(SERVER_DISPLAY_RANKS));
    if (serverNumberInGroup >= 1 && serverNumberInGroup <= knownServers)
    {
        return SERVER_DISPLAY_RANKS[serverNumberInGroup - 1];
    }

    return knownServers + serverNumberInGroup;
}

void FormatServerTitle(wchar_t* title, size_t titleSize, const wchar_t* groupName, int serverNumberInGroup)
{
    wchar_t label[64];
    if (ServerLabel(groupName, serverNumberInGroup, label, std::size(label)))
    {
        swprintf(title, titleSize, L"[%ls]", label);
    }
}
}
