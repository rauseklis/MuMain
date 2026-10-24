#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace GameLogic::Items::Sets
{
inline constexpr std::size_t SerializedFullOptionCount = 5;
inline constexpr std::size_t ExtraStatOptionCount = 4;

// These options are shown in the existing full-set section and use its
// activation rules. The serialized Season 6 BMD layout remains unchanged.
void AddAncientFullStatOptions(std::uint8_t setNumber, std::span<std::uint8_t> options, std::span<std::uint8_t> values);
} // namespace GameLogic::Items::Sets
