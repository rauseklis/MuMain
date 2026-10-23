#include "doctest.h"

#include "UI/NewUI/HUD/MasterSkillTreeLayout.h"

namespace
{
constexpr float AvailableHeight = static_cast<float>(UI::MasterTree::CellsBottom - UI::MasterTree::CellsTop);
}

TEST_CASE("a tree with a few ranks keeps full-size slots and the original row pitch")
{
    const auto rows = UI::MasterTree::ComputeRows(4, AvailableHeight);
    CHECK(rows.scale == doctest::Approx(1.f));
    CHECK(rows.pitch == doctest::Approx(UI::MasterTree::MaximumRowPitch));
}

TEST_CASE("every rank count fits between the top and the bottom of the cells with a gap between rows")
{
    for (int ranks = 1; ranks <= 10; ++ranks)
    {
        const auto rows = UI::MasterTree::ComputeRows(ranks, AvailableHeight);
        const float lastBottom = (ranks - 1) * rows.pitch + UI::MasterTree::SlotHeight * rows.scale;
        CHECK(lastBottom <= AvailableHeight + 0.01f);
        if (ranks > 1)
        {
            CHECK(rows.pitch - UI::MasterTree::SlotHeight * rows.scale >= UI::MasterTree::MinimumRowGap - 0.01f);
        }
    }
}

TEST_CASE("more ranks never make the slots larger")
{
    float previous = 1.f;
    for (int ranks = 1; ranks <= 10; ++ranks)
    {
        const auto rows = UI::MasterTree::ComputeRows(ranks, AvailableHeight);
        CHECK(rows.scale <= previous + 0.0001f);
        previous = rows.scale;
    }
}

TEST_CASE("the three columns sit inside the window with the same gap")
{
    CHECK(UI::MasterTree::ColumnX(0) == UI::MasterTree::ColumnLeft);
    const int last = UI::MasterTree::ColumnX(2) + UI::MasterTree::ColumnWidth;
    CHECK(last + UI::MasterTree::ColumnLeft == UI::MasterTree::WindowWidth);
}

TEST_CASE("the four slots of a row stay inside their column")
{
    const UI::MasterTree::RowMetrics rows;
    for (int column = 0; column < UI::MasterTree::ColumnCount; ++column)
    {
        const auto first = UI::MasterTree::SlotOf(column, 0, 1, rows);
        const auto lastSlot = UI::MasterTree::SlotOf(column, 3, 1, rows);
        CHECK(first.x >= UI::MasterTree::ColumnX(column));
        CHECK(lastSlot.right() <= UI::MasterTree::ColumnX(column) + UI::MasterTree::ColumnWidth);
    }
}
