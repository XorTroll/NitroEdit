
#pragma once
#include <ui/ui_Background.hpp>
#include <ui/ui_TextFont.hpp>
#include <ntr/nfs/nfs_SelfNitroFs.hpp>

namespace ui::menu {

    constexpr u32 NormalLineHeight = 12;
    constexpr u32 ScrollMenuIconWidth = 32;
    constexpr u32 ScrollMenuIconHeight = 32;

    enum class DialogResult {
        Yes,
        No,
        Cancel
    };

    struct ScrollMenuEntry {
        gfx::abgr1555::Color *icon_gfx;
        std::string text;
        std::function<void()> on_select;
        std::function<void()> on_focus;
    };

    void Initialize();
    void MainLoop();
    void UpdateGraphics();

    void ATTR_NORETURN ShowDialogLoop(const std::string &msg);
    void ShowOkDialog(const std::string &msg);
    DialogResult ShowYesNoCancelDialog(const std::string &msg);
    DialogResult ShowYesNoDialog(const std::string &msg);
    void RunWithDialog(const std::string &msg, std::function<void()> fn);
    void LoadScrollMenu(std::vector<ScrollMenuEntry> entries, std::function<void(const u32)> on_other_input);
    void SetScrollMenuVisible(const bool visible);
    void EnableScrollMenuSelection(std::function<void(const std::vector<std::string>&)> on_select_fn);
    void DisableScrollMenuSelection();
    ui::TextFont &GetTextFont();
    ui::Background &GetSubBackground();
    void PushMenu(const std::string &name);
    void PopMenu();
    void AddButtonText(const std::string &key, const std::string &text);
    void ResetButtonTexts();

    inline DialogResult ShowSaveConfirmationDialog() {
        return ShowYesNoCancelDialog("Would you like to save\nany made changes?");
    }

    inline DialogResult ShowOpenConfirmationDialog(const std::string &name) {
        return ShowYesNoDialog("Would you like to open\nthis " + name + "?");
    }

}