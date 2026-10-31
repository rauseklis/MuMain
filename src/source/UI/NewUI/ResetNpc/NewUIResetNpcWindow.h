// NewUIResetNpcWindow.h: the window of Raymond the Reset Master, the NPC which resets characters.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <chrono>

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/ResetNpc/ResetNpcModel.h"

namespace SEASON3B
{
// Shows what the next reset costs (level, Zen, fruits from the Jewel Bank) and that nothing may be worn, next to what
// the character has, and a button which asks for the reset. The server decides: the window only shows what the server
// answers (0xDC sub 0x80) and sends the request of the button. Pressing the button twice (the second press within a few
// seconds) avoids resetting by accident. A question mark explains all requirements in a tooltip.
class CNewUIResetNpcWindow : public CNewUIObj
{
public:
    enum IMAGE_LIST
    {
        IMAGE_RESET_FRUIT = BITMAP_INTERFACE_RESET_NPC_BEGIN, // newui_reset_fruit.tga (32 x 32)
        IMAGE_RESET_GLYPHS, // newui_reset_glyphs.tga: level, check, cross, help, shield (8 cells of 32 x 32)
    };

    static constexpr int WINDOW_WIDTH = 330;
    static constexpr int WINDOW_HEIGHT = 292;

    CNewUIResetNpcWindow();
    ~CNewUIResetNpcWindow() override;

    bool Create(CNewUIManager* pNewUIMng);
    void Release();

    // Called once when the window becomes visible.
    void OpeningProcess();
    // Called once when the window is hidden: lets the server know that the dialog with the NPC is over.
    void ClosingProcess();

    // Called from the network dispatch (WSclient.cpp, 0xDC).
    void SetState(const ResetNpc::State& state);

    bool Render() override;
    bool Update() override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    float GetLayerDepth() override;

private:
    struct Rect
    {
        int x;
        int y;
        int width;
        int height;
    };

    enum class RowIcon
    {
        Level,
        Zen,
        Fruit,
        Equipment,
    };

    [[nodiscard]] Rect ButtonRect() const;
    [[nodiscard]] Rect CloseRect() const;
    // The question mark sits on the title line, left of the close button.
    [[nodiscard]] Rect HelpRect() const;
    void RenderRequirementRow(int row, RowIcon icon, const wchar_t* label, const std::wstring& value, bool met) const;
    void RenderHelpButton() const;
    void RenderHelpTooltip() const;
    void PressResetButton();

    CNewUIManager* m_pNewUIMng = nullptr;
    int m_posX = 0;
    int m_posY = 0;
    ResetNpc::State m_state;
    bool m_hasState = false;
    bool m_waitingForAnswer = false;
    bool m_confirmPending = false;
    std::chrono::steady_clock::time_point m_confirmDeadline;
    std::chrono::steady_clock::time_point m_answerDeadline;
    const wchar_t* m_message = L"";
};
} // namespace SEASON3B
