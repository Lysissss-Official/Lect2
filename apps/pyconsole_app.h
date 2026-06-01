//
// PyConsole — pocketpy 交互式控制台 (REPL)
// ============================================================================
// 每次 Enter 后创建输出元素 + 新输入元素，纵向平铺实现滚动历史。
// 多行输出按 \n 拆分为多个 Element，每行一个。
// 键盘输入经 ctrllerd 全键轮询，所有按键均使用上升沿检测。
//
// 布局：
//   [欢迎信息]
//   [> 1+1   ]  ← 历史输入（只读）
//   [2       ]  ← 输出行 1
//   [        ]  ← 输出行 2（多行时）
//   [> _     ]  ← 当前输入行（总是最后一个元素）
//

#ifndef APSISUI2_PYCONSOLE_APP_H
#define APSISUI2_PYCONSOLE_APP_H

#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "../include/lapp/Application.h"
#include "../include/lcore/VMMgr.h"
#include "../include/lui/platform/linux7/theme/UIRenderApsisUI2.h"
#include "../include/lui/extension/font/font_con_24.h"

class PyConsoleApp : public lapp::Application {
public:
    PyConsoleApp() {
        type       = lapp::APP_VM;
        start_page = &page;
    }

    ~PyConsoleApp() override { delete vm; }

    void app_setup() override {
        vm = new lcore::VMMgr();
        buildPage();
        page.updatePhysical(screen->width, screen->height);

        auto* theme_r = dynamic_cast<lui::UIRenderApsisUI2*>(renderer);
        if (theme_r) theme_r->setFont(&lui::ext::fc24);
    }

    void app_main() override {
        if (!renderer || !controller) return;

        using clock = std::chrono::steady_clock;
        auto last_poll = clock::now();

        while (true) {
            if (pollKeyRise(VK_ESCAPE)) break;

            auto now = clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_poll).count() < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            last_poll = now;
            bool need_redraw = false;

            // 方向键 — 焦点导航（自动滚动到可见区域）
            if (pollKeyRise(VK_UP))   { page.moveFocusUp();   need_redraw = true; }
            if (pollKeyRise(VK_DOWN)) { page.moveFocusDown(); need_redraw = true; }

            if (pollKeyRise(VK_BACK) && !input_buf.empty()) {
                input_buf.pop_back();
                need_redraw = true;
            }
            if (pollKeyRise(VK_RETURN) && !input_buf.empty()) {
                handleEnter();
                need_redraw = true;
            }
            char ch = pollChar();
            if (ch) { input_buf += ch; need_redraw = true; }

            if (need_redraw) {
                // 更新当前输入行（总是最后一个元素）
                {
                    std::lock_guard<std::recursive_mutex> lock(page.state_mutex);
                    auto& in_el = page.blocks[0].elements.back();
                    in_el.content = "> " + input_buf;
                }
                auto& in_el = page.blocks[0].elements.back();
                renderer->requestReDraw(&in_el);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    static constexpr float LINE_H   = 7.0f;   // 行高（逻辑百分比）
    static constexpr float GAP      = 1.0f;   // 行间距
    static constexpr float MARGIN_X = 2.0f;   // 左边距
    static constexpr float ELEM_W   = 96.0f;  // 元素宽度

    lui::Page page;
    lcore::VMMgr* vm = nullptr;
    std::string input_buf;
    float next_y = 7.0f;                // 下一个元素的起始 y 坐标（留出状态栏空间）
    uint8_t prev_keys[256] = {};

    // ---- 页面构建 ----

    lui::Element makeElem(const std::string& content) {
        lui::Element el;
        el.type     = lui::ELE_TEXTBOX;
        el.content  = content;
        el.logic_x1 = MARGIN_X;  el.logic_y1 = next_y;
        el.logic_x2 = ELEM_W;    el.logic_y2 = next_y + LINE_H;
        next_y += LINE_H + GAP;
        return el;
    }

    void buildPage() {
        lui::Block blk;
        blk.logic_x1 = 0.0f;  blk.logic_y1 = 0.0f;
        blk.logic_x2 = 100.0f; blk.logic_y2 = next_y;  // 随内容增长
        blk.elements.reserve(256);

        blk.elements.push_back(makeElem("pocketpy ready."));
        blk.elements.push_back(makeElem("> "));

        page.blocks.push_back(blk);
    }

    // ---- 命令执行 ----

    void handleEnter() {
        std::string output;
        bool ok = vm->exec(input_buf.c_str(), output);

        std::lock_guard<std::recursive_mutex> lock(page.state_mutex);
        auto& blk = page.blocks[0];

        // 按 \n 拆分输出，每行一个 Element
        if (ok) {
            std::istringstream ss(output);
            std::string line;
            while (std::getline(ss, line)) {
                blk.elements.push_back(makeElem(line));
            }
        } else {
            // 错误信息也按行拆分
            std::istringstream ss(output);
            std::string line;
            while (std::getline(ss, line)) {
                blk.elements.push_back(makeElem("Error: " + line));
            }
        }

        // 创建新输入行
        blk.elements.push_back(makeElem("> "));

        // 扩展区块逻辑下界以容纳全部内容（否则滚动后区块被视口裁剪跳过）
        blk.logic_y2 = next_y;

        // 更新物理坐标，重建焦点图
        page.updatePhysical(screen->width, screen->height);
        // 聚焦到新输入行并滚动到底部
        page.setFocus(&blk.elements.back());
        page.scroll_y = page.max_scroll;

        input_buf.clear();
    }

    // ---- 键盘输入 ----

    char pollChar() {
        bool shift = controller->getKB(VK_SHIFT);
        int vk;

        // A-Z
        for (vk = 'A'; vk <= 'Z'; ++vk)
            if (pollKeyRise(vk))
                return static_cast<char>(shift ? vk : vk + 32);

        // 0-9（Shift 输出标点）
        for (vk = '0'; vk <= '9'; ++vk) {
            if (pollKeyRise(vk)) {
                if (shift) {
                    static const char sym[] = ")!@#$%^&*(";
                    return sym[vk - '0'];
                }
                return static_cast<char>(vk);
            }
        }

        if (pollKeyRise(VK_SPACE)) return ' ';

        // US 标准键盘布局 OEM 映射
        if (pollKeyRise(VK_OEM_1))      return shift ? ':' : ';';    // ;:
        if (pollKeyRise(VK_OEM_2))      return shift ? '?' : '/';    // /?
        if (pollKeyRise(VK_OEM_3))      return shift ? '~' : '`';    // `~
        if (pollKeyRise(VK_OEM_4))      return shift ? '{' : '[';    // [{
        if (pollKeyRise(VK_OEM_5))      return shift ? '|' : '\\';   // \|
        if (pollKeyRise(VK_OEM_6))      return shift ? '}' : ']';    // ]}
        if (pollKeyRise(VK_OEM_7))      return shift ? '"' : '\'';   // '"
        if (pollKeyRise(VK_OEM_COMMA))  return shift ? '<' : ',';    // ,<
        if (pollKeyRise(VK_OEM_PERIOD)) return shift ? '>' : '.';    // .>
        if (pollKeyRise(VK_OEM_MINUS))  return shift ? '_' : '-';    // -_
        if (pollKeyRise(VK_OEM_PLUS))   return shift ? '+' : '=';    // =+

        return '\0';
    }

    bool pollKeyRise(int vk) {
        bool now  = controller->getKB(vk) != 0;
        bool rise = now && !prev_keys[vk];
        prev_keys[vk] = now;
        return rise;
    }
};

#endif //APSISUI2_PYCONSOLE_APP_H
