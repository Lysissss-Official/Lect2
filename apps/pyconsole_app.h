//
// PyConsole — pocketpy 交互式控制台
// ============================================================================
// 键盘输入经 ctrllerd 全键轮询，App 通过 controller->getKB() 读取。
// A-Z/0-9/空格 键入字符，Backspace 删除，Enter 执行，ESC 退出。
//

#ifndef APSISUI2_PYCONSOLE_APP_H
#define APSISUI2_PYCONSOLE_APP_H

#include <chrono>
#include <string>
#include <thread>

#include "../include/lapp/Application.h"
#include "../include/lcore/VMMgr.h"

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
    }

    void app_main() override {
        if (!renderer || !controller) return;

        using clock = std::chrono::steady_clock;
        auto last_poll = clock::now();

        while (true) {
            if (controller->getKB(VK_ESCAPE)) break;

            auto now = clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_poll).count() < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            last_poll = now;
            bool dirty = false;

            // Backspace
            if (controller->getKB(VK_BACK) && !input_buf.empty()) {
                input_buf.pop_back();
                dirty = true;
            }
            // Enter — 执行
            if (controller->getKB(VK_RETURN) && !input_buf.empty()) {
                dirty = true;
                std::string output;
                if (vm->exec(input_buf.c_str(), output))
                    out_box.content = output.empty() ? "OK" : output;
                else
                    out_box.content = "Error: " + output;
                input_buf.clear();
            }
            // 可打印字符键
            char ch = pollChar();
            if (ch) { input_buf += ch; dirty = true; }

            if (dirty) {
                in_box.content = "> " + input_buf;
                renderer->requestElementRedraw(in_box.getID());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    lui::Page page;
    lui::Element out_box, in_box;
    lcore::VMMgr* vm = nullptr;
    std::string input_buf;

    // 上一次轮询的按键状态，用于上升沿检测（防止一次按键触发多次）
    uint8_t prev_keys[256] = {};

    char pollChar() {
        // 字母 A-Z（Shift 切换大小写）
        for (int vk = 'A'; vk <= 'Z'; ++vk) {
            if (controller->getKB(vk) && !prev_keys[vk]) {
                prev_keys[vk] = 1;
                bool shift = controller->getKB(VK_SHIFT);
                return static_cast<char>(shift ? vk : vk + 32);
            }
            prev_keys[vk] = controller->getKB(vk);
        }
        // 数字 0-9
        for (int vk = '0'; vk <= '9'; ++vk) {
            if (controller->getKB(vk) && !prev_keys[vk]) {
                prev_keys[vk] = 1;
                return static_cast<char>(vk);
            }
            prev_keys[vk] = controller->getKB(vk);
        }
        // 空格
        if (pollKeyRise(VK_SPACE)) return ' ';
        // 标点
        bool shift = controller->getKB(VK_SHIFT);
        if (pollKeyRise(VK_OEM_PERIOD)) return shift ? '>' : '.';
        if (pollKeyRise(VK_OEM_COMMA))  return shift ? '<' : ',';
        if (pollKeyRise(VK_OEM_MINUS))  return shift ? '_' : '-';
        if (pollKeyRise(VK_OEM_PLUS))   return shift ? '+' : '=';
        if (pollKeyRise(VK_OEM_2))      return shift ? '?' : '/';
        if (pollKeyRise(VK_OEM_3))      return shift ? '"' : '\'';
        if (pollKeyRise(VK_OEM_4))      return shift ? '{' : '[';
        if (pollKeyRise(VK_OEM_6))      return shift ? '}' : ']';
        if (pollKeyRise(VK_OEM_7))      return shift ? '|' : '\\';
        if (pollKeyRise(0x39))          return shift ? '(' : '9';  // '(' on shift+9 area
        if (pollKeyRise(0x30))          return shift ? ')' : '0';
        return '\0';
    }

    bool pollKeyRise(int vk) {
        bool now = controller->getKB(vk) != 0;
        bool rise = now && !prev_keys[vk];
        prev_keys[vk] = now;
        return rise;
    }

    void buildPage() {
        out_box.type     = lui::ELE_TEXTBOX;
        out_box.content  = "pocketpy ready.";
        out_box.logic_x1 = 2.0f;  out_box.logic_y1 = 8.0f;
        out_box.logic_x2 = 98.0f; out_box.logic_y2 = 72.0f;

        in_box.type      = lui::ELE_TEXTBOX;
        in_box.content   = "> ";
        in_box.logic_x1  = 2.0f;  in_box.logic_y1 = 76.0f;
        in_box.logic_x2  = 98.0f; in_box.logic_y2 = 94.0f;

        lui::Block blk;
        blk.logic_x1 = 0.0f;  blk.logic_y1 = 0.0f;
        blk.logic_x2 = 100.0f; blk.logic_y2 = 100.0f;
        blk.elements.push_back(out_box);
        blk.elements.push_back(in_box);

        page.blocks.push_back(blk);
    }
};

#endif //APSISUI2_PYCONSOLE_APP_H
