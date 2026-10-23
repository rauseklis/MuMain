#pragma once

#include <algorithm>

// Geometry of the master skill tree window, kept free of drawing so it can be tested: three framed columns, each a
// header and four skill slots across, one row per skill rank. Coordinates are logical pixels relative to the window.
namespace UI::MasterTree
{
    constexpr int WindowWidth = 640;
    constexpr int WindowHeight = 428;

    constexpr int ColumnCount = 3;
    constexpr int CellsPerRow = 4;
    constexpr int ColumnLeft = 26;
    constexpr int ColumnWidth = 188;
    constexpr int ColumnGap = 12;
    constexpr int ColumnTop = 66;
    constexpr int ColumnBottom = 380;

    constexpr int HeaderTop = 70;
    constexpr int HeaderHeight = 20;
    constexpr int HeaderInset = 8;

    // Where the first slot of a column starts and the lowest a slot may end.
    constexpr int CellsTop = 98;
    constexpr int CellsBottom = 374;
    constexpr int CellWidth = 43;

    // The slot ring around a 20 x 28 skill icon at full size.
    constexpr float SlotWidth = 30.f;
    constexpr float SlotHeight = 34.f;
    constexpr float IconWidth = 20.f;
    constexpr float IconHeight = 28.f;

    constexpr float MaximumRowPitch = 41.f;
    // The room kept between two rows for the connecting arrow.
    constexpr float MinimumRowGap = 7.f;

    struct RowMetrics
    {
        float pitch = MaximumRowPitch; // from the top of one row to the top of the next
        float scale = 1.f;             // how much the slot and its icon shrink to fit
    };

    // The rows share the height between CellsTop and CellsBottom. With few ranks they keep their full size; with many
    // they shrink together so the arrows between them keep a minimum gap.
    inline RowMetrics ComputeRows(int rankCount, float availableHeight)
    {
        RowMetrics rows;
        if (rankCount <= 1)
        {
            return rows;
        }
        const float ranks = static_cast<float>(rankCount);
        const float fitScale = (availableHeight - MinimumRowGap * (ranks - 1.f)) / (ranks * SlotHeight);
        rows.scale = std::clamp(fitScale, 0.1f, 1.f);
        rows.pitch = std::min(MaximumRowPitch, (availableHeight - SlotHeight * rows.scale) / (ranks - 1.f));
        return rows;
    }

    inline int ColumnX(int column)
    {
        return ColumnLeft + column * (ColumnWidth + ColumnGap);
    }

    struct Slot
    {
        float x = 0.f;
        float y = 0.f;
        float width = 0.f;
        float height = 0.f;
        float centreX() const { return x + width / 2.f; }
        float centreY() const { return y + height / 2.f; }
        float right() const { return x + width; }
        float bottom() const { return y + height; }
    };

    // The slot of the skill in a column, a cell across (0 to 3) and a rank (1 and up).
    inline Slot SlotOf(int column, int cell, int rank, const RowMetrics& rows)
    {
        const float cellsWidth = static_cast<float>(CellsPerRow * CellWidth);
        const float left = ColumnX(column) + (ColumnWidth - cellsWidth) / 2.f + cell * CellWidth;
        Slot slot;
        slot.width = SlotWidth * rows.scale;
        slot.height = SlotHeight * rows.scale;
        slot.x = left + (CellWidth - slot.width) / 2.f;
        slot.y = static_cast<float>(CellsTop) + (rank - 1) * rows.pitch;
        return slot;
    }
}
