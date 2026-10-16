#pragma once

#include <cstdint>
#include <string>

namespace UI::Buffs
{
    std::wstring FormatDuration(std::uint64_t expiresAt, std::uint64_t now);
}
