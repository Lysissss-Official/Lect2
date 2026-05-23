//
// ApsisUI II — LectOS 2 演示应用
// 展示页面布局、滚动内容、焦点导航和动画焦点过渡。
//

#ifndef APSISUI2_DEMO_APP_H
#define APSISUI2_DEMO_APP_H

#include <chrono>
#include <thread>

#include "easyx.h"
#include "graphics.h"

#include "../include/lui/base/UIStructure.h"
#include "../include/lui/base/UICtrller.h"
#include "../include/lui/base/UITransor.h"
#include "../include/lui/base/UIRender.h"
#include "../include/lui/platform/linux7/UICtrllerLinux7.h"
#include "../include/lui/platform/linux7/UITransorLinux7.h"
#include "../include/lui/theme/UIRenderApsisUI2.h"
#include "../include/lui/extension/font/font_20.h"

using namespace lui;

class DemoApp : public app::Application {
public:
    DemoApp() {
        type       = app::APP_NATIVE;
        start_page = &demo_page;
    }

    void setup(scr::Screen*               scr,
               render::Render*             ren,
               ctrller::CtrllerService*    ctl,
               transor::TranslatorService* trs) {
        screen     = scr;
        renderer   = ren;
        controller = ctl;
        translator = trs;

        buildPage();
        demo_page.updatePhysical(scr->width, scr->height);

        auto* theme_r = dynamic_cast<render::UIRenderApsisUI2*>(ren);
        if (theme_r) theme_r->setFont(&lui::ext::font::f20);
    }

    void app_main() override {
        if (!screen || !renderer || !controller) return;

        using clock = std::chrono::steady_clock;
        auto last_input = clock::now();

        // 持续渲染循环以保证动画流畅
        while (true) {
            controller->refreshStatus();

            if (controller->getKB(VK_ESCAPE) == 1) break;

            auto now = clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_input).count();

            if (elapsed >= 80) {
                float deg = controller->getSW();
                bool moved = false;

                switch (static_cast<int>(deg)) {
                    case 0:   moved = demo_page.moveFocusRight(); break;
                    case 90:  moved = demo_page.moveFocusUp();    break;
                    case 180: moved = demo_page.moveFocusLeft();  break;
                    case 270: moved = demo_page.moveFocusDown();  break;
                    case -1:  break;
                }

                if (moved) last_input = now;
            }

            // 逐帧渲染，确保动画插值平滑
            BeginBatchDraw();
            renderer->renderPage(&demo_page);
            FlushBatchDraw();

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    pge::Page demo_page;

    void buildPage() {
        float y = 5.0f;

        // ============================================================
        // 区块 1 — 页眉（始终可见，页面顶部）
        // ============================================================
        {
            blk::Block header;
            header.logic_x1 =  0.0f; header.logic_y1 = y;
            header.logic_x2 = 100.0f; header.logic_y2 = y + 12.0f;

            ele::Element title;
            title.type    = ele::ELE_TEXTBOX;
            title.content = "ApsisUI II  —  LectOS 2 Demo";
            title.logic_x1 = 20.0f; title.logic_y1 = y + 1.0f;
            title.logic_x2 = 80.0f; title.logic_y2 = y + 10.0f;
            header.elements.push_back(title);

            demo_page.blocks.push_back(header);
            y += 14.0f;
        }

        // ============================================================
        // 区块 2 — 快捷操作（三列按钮网格）
        // ============================================================
        {
            blk::Block actions;
            actions.logic_x1 =  2.0f; actions.logic_y1 = y;
            actions.logic_x2 = 98.0f; actions.logic_y2 = y + 38.0f;

            auto addBtn = [&](const char* label, float lx, float ly, float rx, float ry) {
                ele::Element btn;
                btn.type    = ele::ELE_BUTTON;
                btn.content = label;
                btn.logic_x1 = lx; btn.logic_y1 = ly;
                btn.logic_x2 = rx; btn.logic_y2 = ry;
                actions.elements.push_back(btn);
            };

            addBtn("Option A",   5.0f,  y + 4.0f, 30.0f, y + 14.0f);
            addBtn("Option B",   5.0f,  y +18.0f, 30.0f, y + 28.0f);
            addBtn("Settings",  35.0f,  y + 4.0f, 65.0f, y + 14.0f);

            {
                ele::Element lst;
                lst.type    = ele::ELE_LIST;
                lst.content = "Item 1 / Item 2 / Item 3";
                lst.logic_x1 = 35.0f; lst.logic_y1 = y + 18.0f;
                lst.logic_x2 = 65.0f; lst.logic_y2 = y + 32.0f;
                actions.elements.push_back(lst);
            }

            addBtn("Help",      70.0f,  y + 4.0f, 95.0f, y + 14.0f);
            addBtn("About",     70.0f,  y +18.0f, 95.0f, y + 28.0f);

            demo_page.blocks.push_back(actions);
            y += 40.0f;
        }

        // ============================================================
        // 区块 3 — 数据面板（向下滚动时进入视野）
        // ============================================================
        {
            blk::Block data;
            data.logic_x1 =  2.0f; data.logic_y1 = y;
            data.logic_x2 = 98.0f; data.logic_y2 = y + 48.0f;

            auto addField = [&](const char* label, float lx, float ly, float rx, float ry) {
                ele::Element f;
                f.type    = ele::ELE_TEXTBOX;
                f.content = label;
                f.logic_x1 = lx; f.logic_y1 = ly;
                f.logic_x2 = rx; f.logic_y2 = ry;
                data.elements.push_back(f);
            };

            addField("Sensor A:  247.3 kPa",      5.0f, y + 2.0f,  47.0f, y +10.0f);
            addField("Sensor B:   18.7 C",        5.0f, y +12.0f,  47.0f, y +20.0f);
            addField("Sensor C:  1024 rpm",       5.0f, y +22.0f,  47.0f, y +30.0f);
            addField("Sensor D:    0.82 V",       5.0f, y +32.0f,  47.0f, y +40.0f);

            addField("Axis X:     +12.5 mm",     52.0f, y + 2.0f,  95.0f, y +10.0f);
            addField("Axis Y:      -3.2 mm",     52.0f, y +12.0f,  95.0f, y +20.0f);
            addField("Axis Z:     +45.1 mm",     52.0f, y +22.0f,  95.0f, y +30.0f);
            addField("Tilt:        2.8 deg",     52.0f, y +32.0f,  95.0f, y +40.0f);

            demo_page.blocks.push_back(data);
            y += 50.0f;
        }

        // ============================================================
        // 区块 4 — 校准控件（滚动进入视野）
        // ============================================================
        {
            blk::Block calib;
            calib.logic_x1 =  2.0f; calib.logic_y1 = y;
            calib.logic_x2 = 98.0f; calib.logic_y2 = y + 36.0f;

            auto addBtn = [&](const char* label, float lx, float ly, float rx, float ry) {
                ele::Element btn;
                btn.type    = ele::ELE_BUTTON;
                btn.content = label;
                btn.logic_x1 = lx; btn.logic_y1 = ly;
                btn.logic_x2 = rx; btn.logic_y2 = ry;
                calib.elements.push_back(btn);
            };

            addBtn("Zero All Axes",     5.0f,  y + 4.0f, 47.0f, y + 14.0f);
            addBtn("Run Calibration",   5.0f,  y +18.0f, 47.0f, y + 28.0f);
            addBtn("Load Profile",     52.0f,  y + 4.0f, 95.0f, y + 14.0f);
            addBtn("Save Profile",     52.0f,  y +18.0f, 95.0f, y + 28.0f);

            demo_page.blocks.push_back(calib);
            y += 38.0f;
        }

        // ============================================================
        // 区块 5 — 诊断日志（长列表；大幅延伸页面高度）
        // ============================================================
        {
            blk::Block diag;
            diag.logic_x1 =  2.0f; diag.logic_y1 = y;
            diag.logic_x2 = 98.0f; diag.logic_y2 = y + 50.0f;

            auto addEntry = [&](const char* text, float ly) {
                ele::Element e;
                e.type    = ele::ELE_TEXTBOX;
                e.content = text;
                e.logic_x1 = 5.0f;  e.logic_y1 = ly;
                e.logic_x2 = 95.0f; e.logic_y2 = ly + 7.0f;
                diag.elements.push_back(e);
            };

            addEntry("[12:00] System boot ........................ OK",     y + 2.0f);
            addEntry("[12:01] SPI bus init ....................... OK",     y + 9.0f);
            addEntry("[12:01] Sensor polling started ............. OK",     y +16.0f);
            addEntry("[12:02] Calibration data loaded ............ OK",     y +23.0f);
            addEntry("[12:03] Axis calculator online ............. OK",     y +30.0f);
            addEntry("[12:04] Watchdog armed ..................... OK",     y +37.0f);
            addEntry("[12:05] Ready for input .................... OK",     y +44.0f);

            demo_page.blocks.push_back(diag);
            y += 52.0f;
        }

        // ============================================================
        // 区块 6 — 页脚（远在下方；需滚动查看）
        // ============================================================
        {
            blk::Block footer;
            footer.logic_x1 =  2.0f; footer.logic_y1 = y;
            footer.logic_x2 = 98.0f; footer.logic_y2 = y + 14.0f;

            ele::Element info;
            info.type    = ele::ELE_TEXTBOX;
            info.content = "Arrow keys = move focus  |  ESC = exit  |  Page scrolls to follow";
            info.logic_x1 = 5.0f; info.logic_y1 = y + 2.0f;
            info.logic_x2 = 95.0f; info.logic_y2 = y + 11.0f;
            footer.elements.push_back(info);

            demo_page.blocks.push_back(footer);
        }
    }
};

#endif //APSISUI2_DEMO_APP_H
