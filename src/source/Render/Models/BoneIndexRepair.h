#pragma once

namespace Render::Models
{
// Some shipped BMD models carry vertices or normals whose bone index lies outside the model's bone table.
// Skinning with such an index reads whatever matrix an earlier model left in the shared bone array (or
// memory past it), which drags the vertex to another model's position and draws long flat "folded planes".
// A bad index is attached to the root bone instead.
inline constexpr int RootBoneIndex = 0;

[[nodiscard]] constexpr bool IsBoneIndexValid(int node, int boneCount)
{
    return node >= 0 && node < boneCount;
}

[[nodiscard]] constexpr int RepairBoneIndex(int node, int boneCount)
{
    return IsBoneIndexValid(node, boneCount) ? node : RootBoneIndex;
}
}
