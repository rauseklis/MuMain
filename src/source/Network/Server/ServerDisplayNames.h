#pragma once

// How this server's list entries are named. The server list only gives the client a server number and its load; the
// group name comes from Data\Local\ServerList.bmd (still "Valhalla" there) and the rest is built here.
namespace Network::ServerNames
{
    // The name of the server group as shown to the player.
    const wchar_t* GroupName(const wchar_t* nameInFile);

    // Writes the label of the server ("Emberion 50x") for the 1-based number of the server in its group and returns
    // true. Returns false for a number which has no label, so that the caller keeps the classic name.
    bool ServerLabel(const wchar_t* groupName, int serverNumberInGroup, wchar_t* label, size_t labelSize);

    // Writes "[Emberion 50x]" into title when the server has a label; otherwise leaves title as it is.
    void FormatServerTitle(wchar_t* title, size_t titleSize, const wchar_t* groupName, int serverNumberInGroup);
}
