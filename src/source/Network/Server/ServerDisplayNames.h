#pragma once

// How this server's list entries are named. The server list only gives the client a server number and its load; the
// group name comes from Data\Local\ServerList.bmd (still "Valhalla" there) and the rest is built here.
namespace Network::ServerNames
{
    // The name of the server group as shown to the player.
    const wchar_t* GroupName(const wchar_t* nameInFile);

    // Writes the label of the server ("Inferno 500x") for the 1-based number of the server in its group and returns
    // true. Returns false for a number which has no label, so that the caller keeps the classic name.
    bool ServerLabel(const wchar_t* groupName, int serverNumberInGroup, wchar_t* label, size_t labelSize);

    // Where the server is listed in the server select menu: a lower value is listed higher. Servers without an entry
    // keep their number as position, after the ones that have one.
    int ServerDisplayRank(int serverNumberInGroup);

    // The colours a server entry is drawn with in the server select menu (0xRRGGBB). `glow` is the colour of the soft
    // light inside the button frame, `text` the label colour and `textHover` the label colour under the cursor.
    struct ServerAccent
    {
        unsigned int glow;
        unsigned int text;
        unsigned int textHover;
    };

    // Fills accent and returns true for a server which has its own colours; returns false for the others, which are
    // drawn like any other button.
    bool ServerAccentColors(int serverNumberInGroup, ServerAccent& accent);

    // Writes "[Inferno 500x]" into title when the server has a label; otherwise leaves title as it is.
    void FormatServerTitle(wchar_t* title, size_t titleSize, const wchar_t* groupName, int serverNumberInGroup);
}
