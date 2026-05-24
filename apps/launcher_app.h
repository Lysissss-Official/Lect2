//
// Launcher — LectOS 2 启动台
// ============================================================================
// 默认前台应用，展示已注册 App 列表。
// Arrow keys 选择，Enter 启动，` 键从目标应用返回，ESC 退出程序。
//
// 工作原理：
//   启动台线程始终运行，即使切换到目标应用后也不退出。
//   在 "监视模式" (MODE_WATCHING) 下每 50ms 轮询 VK_OEM_3（` 键），
//   检测到后调用 ThreadMgr::switchForeground() 切回自身。
//   若目标应用自行退出（如 ESC），通过 getRuntime()->state 检测并自动返回。
//
// 布局：
//   [标题]
//   [> App 1  ]  ← 当前选中（按钮，带 > 指示符）
//   [  App 2  ]  ← 未选中
//   [  App 3  ]
//   [页脚提示  ]
//

#ifndef APSISUI2_LAUNCHER_APP_H
#define APSISUI2_LAUNCHER_APP_H

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

    // 由 main.cpp 在 registerApp 之后调用
    void setThreadMgr(lcore::ThreadMgr* mgr) { thread_mgr_ = mgr; }

    // 注册可启动的应用条目
    void addEntry(const std::string& name, uint32_t app_id) {
        entries_.push_back({name, app_id});
    }

    void app_setup() override {
        rebuildPage();
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

    lui::Page page;
    lcore::ThreadMgr* thread_mgr_ = nullptr;
    std::vector<AppEntry> entries_;
    Mode   mode_        = MODE_FOREGROUND;
    size_t selected_    = 0;
    uint32_t launched_id_ = 0;
    bool   should_exit_ = false;
    uint8_t prev_keys[256] = {};

    // ---- 前台模式：显示应用列表 ----

    void handleForeground() {
        bool need_rebuild = false;

        if (pollKeyRise(VK_UP) && selected_ > 0) {
            selected_--;
            need_rebuild = true;
        }
        if (pollKeyRise(VK_DOWN) && selected_ + 1 < entries_.size()) {
            selected_++;
            need_rebuild = true;
        }
        if (pollKeyRise(VK_RETURN) && !entries_.empty()) {
            launched_id_ = entries_[selected_].app_id;
            thread_mgr_->switchForeground(launched_id_);
            mode_ = MODE_WATCHING;
            return;
        }
        // ESC 在启动台中退出整个程序
        if (pollKeyRise(VK_ESCAPE)) {
            should_exit_ = true;
            return;
        }

        if (need_rebuild) {
            rebuildPage();
            page.updatePhysical(screen->width, screen->height);
            renderer->requestPageChange(page.getID());
        }
    }

    // ---- 监视模式：按 ` 键返回启动台 / 目标应用退出 ----
    // （Ctrl+C 被系统保留为 SIGINT，改用 VK_OEM_3 反引号键）

    void handleWatching() {
        // 目标应用已退出则自动返回
        auto* rt = thread_mgr_->getRuntime(launched_id_);
        if (!rt || rt->state == lcore::THREAD_STOPPED) {
            returnToLauncher();
            return;
        }

        if (pollKeyRise(VK_OEM_3)) {
            returnToLauncher();
        }
    }

    void returnToLauncher() {
        mode_ = MODE_FOREGROUND;
        rebuildPage();
        page.updatePhysical(screen->width, screen->height);
        thread_mgr_->switchForeground(getID());
        renderer->requestPageChange(page.getID());
    }

    // ---- 页面构建 ----

    void rebuildPage() {
        page.blocks.clear();

        lui::Block blk;
        blk.logic_x1 = 0.0f;  blk.logic_y1 = 0.0f;
        blk.logic_x2 = 100.0f; blk.logic_y2 = 100.0f;

        float y = 7.0f;  // 状态栏下方

        // 标题
        y = addLine(blk, y, "LectOS 2  Launcher", 12.0f);

        // 分隔线
        y += 2.0f;

        // 应用列表
        if (entries_.empty()) {
            y = addLine(blk, y, "(no apps registered)", 8.0f);
        } else {
            for (size_t i = 0; i < entries_.size(); ++i) {
                bool sel = (i == selected_);
                std::string label = sel ? "> " : "  ";
                label += entries_[i].name;

                lui::Element btn;
                btn.type     = lui::ELE_BUTTON;
                btn.content  = label;
                btn.logic_x1 = 10.0f;  btn.logic_y1 = y;
                btn.logic_x2 = 90.0f;  btn.logic_y2 = y + 8.0f;
                blk.elements.push_back(btn);
                y += 10.0f;
            }
        }

        y += 2.0f;

        // 页脚
        y = addLine(blk, y,
            "Arrows: select  |  Enter: launch  |  ` : return  |  ESC: exit", 8.0f);

        page.blocks.push_back(blk);
    }

    static float addLine(lui::Block& blk, float y,
                         const std::string& text, float h) {
        lui::Element el;
        el.type     = lui::ELE_TEXTBOX;
        el.content  = text;
        el.logic_x1 = 5.0f;   el.logic_y1 = y;
        el.logic_x2 = 95.0f;  el.logic_y2 = y + h;
        blk.elements.push_back(el);
        return y + h + 1.0f;
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
