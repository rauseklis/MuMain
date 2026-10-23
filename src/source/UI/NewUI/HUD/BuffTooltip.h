#pragma once

#include <string>

namespace UI::Buffs::Tooltip
{
    void Append(const std::wstring& text, int color, bool bold, int& count);
    // A thin rule between the title and the body.
    void AppendDivider(int& count);
    void Render(int anchorX, int anchorY, int count);
}
