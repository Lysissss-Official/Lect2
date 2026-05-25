//
// Launcher — LectOS 2 启动台
// ============================================================================
// 默认前台应用，以 Apple-Watch 风格网格展示已注册 App。
// Arrow keys 四向导航，Enter 启动，` 键从目标应用返回，ESC 退出程序。
//
// 工作原理：
//   启动台线程始终运行，即使切换到目标应用后也不退出。
//   在 "监视模式" (MODE_WATCHING) 下每 50ms 轮询 VK_OEM_3（` 键），
//   检测到后调用 ThreadMgr::switchForeground() 切回自身。
//   若目标应用自行退出（如 ESC），通过 getRuntime()->state 检测并自动返回。
//
// 布局（Apple Watch 风格网格）：
//   应用以方形图标排列在网格中，选中的应用位于屏幕中央且尺寸最大，
//   周围应用按切比雪夫距离依次缩小，方向键在网格中四向移动选择。
//
//   屏幕 1200x480（宽高比 2.5:1），逻辑坐标通过 ASPECT 系数补偿，
//   使每个格子在物理屏幕上呈现为正方形。
//

#ifndef APSISUI2_LAUNCHER_APP_H
#define APSISUI2_LAUNCHER_APP_H

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "../include/lapp/Application.h"
#include "../include/lcore/ThreadMgr.h"
#include "../include/lui/platform/linux7/theme/UIRenderApsisUI2.h"
#include "../include/lui/extension/font/font_con_24.h"

class LauncherApp : public lapp::Application {
public:
    LauncherApp() {
        type       = lapp::APP_NATIVE;
        start_page = &page;
    }

    void setThreadMgr(lcore::ThreadMgr* mgr) { thread_mgr_ = mgr; }

    void addEntry(const std::string& name, uint32_t app_id) {
        entries_.push_back({name, app_id});
    }

    void app_setup() override {
        rebuildPage();
        if (!entries_.empty()) {
            auto& grid_el = page.blocks[0].elements[1];
            page.current_focused = &grid_el;
            grid_el.focused = true;
        }
        page.updatePhysical(screen->width, screen->height);

        auto* theme_r = dynamic_cast<lui::UIRenderApsisUI2*>(renderer);
        if (theme_r) theme_r->setFont(&lui::ext::fc24);
    }

    void app_main() override {
        if (!renderer || !controller || !thread_mgr_) return;

        while (!should_exit_) {
            if (mode_ == MODE_FOREGROUND)
                handleForeground();
            else
                handleWatching();

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

private:
    struct AppEntry {
        std::string name;
        uint32_t    app_id;
    };

    enum Mode { MODE_FOREGROUND, MODE_WATCHING };

    // 网格常量（屏幕 1200x480，宽高比 2.5:1）
    // 逻辑坐标 Y 每单位 = 4.8px，X 每单位 = 12px，
    // ASPECT = 480/1200 = 0.4 用于将 Y 轴逻辑尺寸换算为 X 轴，
    // 使格子在物理屏幕上呈现正方形。
    static constexpr int   GRID_COLS = 5;
    static constexpr float CELL_H    = 16.0f;
    static constexpr float ASPECT    = 480.0f / 1200.0f;
    static constexpr float CELL_W    = CELL_H * ASPECT;       // 6.4
    static constexpr float PITCH_H   = 21.0f;
    static constexpr float PITCH_W   = PITCH_H * ASPECT;      // 8.4
    static constexpr float CENTER_X  = 50.0f;
    static constexpr float CENTER_Y  = 48.0f;

    lui::Page page;
    lcore::ThreadMgr* thread_mgr_ = nullptr;
    std::vector<AppEntry> entries_;
    Mode     mode_               = MODE_FOREGROUND;
    size_t   selected_           = 0;
    uint32_t launched_id_        = 0;
    bool     should_exit_        = false;
    uint8_t  prev_keys[256]      = {};
    bool     backtick_debounce_  = false;

    // ---- 前台模式：网格选择 ----

    void handleForeground() {
        bool need_rebuild = false;

        int sel_row = static_cast<int>(selected_) / GRID_COLS;
        int sel_col = static_cast<int>(selected_) % GRID_COLS;

        if (pollKeyRise(VK_UP) && sel_row > 0) {
            selected_ -= GRID_COLS;
            need_rebuild = true;
        }
        if (pollKeyRise(VK_DOWN)) {
            size_t nxt = selected_ + GRID_COLS;
            if (nxt < entries_.size()) {
                selected_ = nxt;
                need_rebuild = true;
            }
        }
        if (pollKeyRise(VK_LEFT) && sel_col > 0) {
            selected_--;
            need_rebuild = true;
        }
        if (pollKeyRise(VK_RIGHT)) {
            size_t nxt = selected_ + 1;
            if (sel_col + 1 < GRID_COLS && nxt < entries_.size()) {
                selected_ = nxt;
                need_rebuild = true;
            }
        }
        if (pollKeyRise(VK_RETURN) && !entries_.empty()) {
            launched_id_ = entries_[selected_].app_id;
            thread_mgr_->switchForeground(launched_id_);
            mode_ = MODE_WATCHING;
            prev_keys[VK_OEM_3] = 0;
            backtick_debounce_  = false;
            return;
        }
        if (pollKeyRise(VK_ESCAPE)) {
            should_exit_ = true;
            return;
        }

        if (need_rebuild) {
            refreshPage();
        }
    }

    // ---- 监视模式 ----
    // 双重检测策略：
    //   1. 上升沿检测（pollKeyRise）— 处理标准情况
    //   2. 电平检测 + 防抖 — 当上升沿因 Wine/键盘布局边界情况漏检时兜底
    //      （某些输入法或键盘布局下 VK_OEM_3 的 GetAsyncKeyState 行为不一致）

    void handleWatching() {
        auto* rt = thread_mgr_->getRuntime(launched_id_);
        if (!rt || rt->state == lcore::THREAD_STOPPED) {
            returnToLauncher();
            return;
        }

        if (pollKeyRise(VK_OEM_3)) {
            returnToLauncher();
            return;
        }

        // 电平检测兜底（带防抖）
        bool down = controller->getKB(VK_OEM_3) != 0;
        if (down && !backtick_debounce_) {
            backtick_debounce_ = true;
            returnToLauncher();
            return;
        }
        if (!down)
            backtick_debounce_ = false;
    }

    void returnToLauncher() {
        mode_ = MODE_FOREGROUND;
        backtick_debounce_ = false;
        refreshPage();
        thread_mgr_->switchForeground(getID());
        // requestPageChange 已由 refreshPage() 调用
    }

    // ---- 页面构建 ----

    void refreshPage() {
        rebuildPage();
        // 在 updatePhysical 之前设置 current_focused，
        // 防止 recalculateFocus 把焦点重置到标题等非目标元素
        if (selected_ < entries_.size()) {
            auto& grid_el = page.blocks[0].elements[1 + selected_];
            page.current_focused = &grid_el;
            grid_el.focused = true;
        }
        page.updatePhysical(screen->width, screen->height);
        renderer->requestPageChange(page.getID());
    }

    void rebuildPage() {
        page.current_focused = nullptr;
        page.blocks.clear();

        int sel_row = static_cast<int>(selected_) / GRID_COLS;
        int sel_col = static_cast<int>(selected_) % GRID_COLS;

        lui::Block blk;
        blk.logic_x1 = 0.0f;  blk.logic_y1 = 0.0f;
        blk.logic_x2 = 100.0f; blk.logic_y2 = 100.0f;

        // 标题
        {
            lui::Element title;
            title.type     = lui::ELE_TEXTBOX;
            title.content  = "LectOS 2  Launcher";
            title.logic_x1 = 5.0f;   title.logic_y1 = 2.0f;
            title.logic_x2 = 50.0f;  title.logic_y2 = 8.0f;
            blk.elements.push_back(title);
        }

        // 应用网格
        for (size_t i = 0; i < entries_.size(); ++i) {
            int row  = static_cast<int>(i) / GRID_COLS;
            int col  = static_cast<int>(i) % GRID_COLS;
            int dr   = row - sel_row;
            int dc   = col - sel_col;
            int dist = std::max(std::abs(dr), std::abs(dc));

            float scale = getCellScale(dist);
            float cw = CELL_W * scale;
            float ch = CELL_H * scale;
            float cx = CENTER_X + static_cast<float>(dc) * PITCH_W;
            float cy = CENTER_Y + static_cast<float>(dr) * PITCH_H;

            lui::Element btn;
            btn.type     = lui::ELE_BUTTON;
            btn.content  = (dist <= 1) ? entries_[i].name : "";
            btn.logic_x1 = cx - cw / 2.0f;
            btn.logic_y1 = cy - ch / 2.0f;
            btn.logic_x2 = cx + cw / 2.0f;
            btn.logic_y2 = cy + ch / 2.0f;
            blk.elements.push_back(btn);
        }

        // 页脚提示
        {
            lui::Element footer;
            footer.type     = lui::ELE_TEXTBOX;
            footer.content  = "Arrows: navigate  |  Enter: launch  |  ` : return  |  ESC: exit";
            footer.logic_x1 = 3.0f;   footer.logic_y1 = 94.0f;
            footer.logic_x2 = 97.0f;  footer.logic_y2 = 98.0f;
            blk.elements.push_back(footer);
        }

        page.blocks.push_back(blk);
    }

    static float getCellScale(int dist) {
        switch (dist) {
            case 0:  return 1.00f;
            case 1:  return 0.65f;
            case 2:  return 0.45f;
            case 3:  return 0.35f;
            default: return 0.28f;
        }
    }

    // ---- 输入 ----

    bool pollKeyRise(int vk) {
        bool now  = controller->getKB(vk) != 0;
        bool rise = now && !prev_keys[vk];
        prev_keys[vk] = now;
        return rise;
    }
};

#endif //APSISUI2_LAUNCHER_APP_H
