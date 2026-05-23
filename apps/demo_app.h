//
// Demo application for ApsisUI II — LectOS 2
// Demonstrates page layout, focus navigation via arrow keys, and rendering.
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
    }

    void app_main() override {
        if (!screen || !renderer || !controller) return;

        // Initial render
        BeginBatchDraw();
        renderer->renderPage(&demo_page);
        FlushBatchDraw();

        using clock = std::chrono::steady_clock;
        auto last_input = clock::now();

        while (true) {
            controller->refreshStatus();

            // Exit on ESC
            if (controller->getKB(VK_ESCAPE) == 1) break;

            // Throttle input to ~10 Hz for usable navigation
            auto now = clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_input).count();

            if (elapsed >= 100) {
                float deg = controller->getSW();
                bool moved = false;

                switch (static_cast<int>(deg)) {
                    case 0:   moved = demo_page.moveFocusRight(); break;
                    case 90:  moved = demo_page.moveFocusUp();    break;
                    case 180: moved = demo_page.moveFocusLeft();  break;
                    case 270: moved = demo_page.moveFocusDown();  break;
                }

                if (moved) {
                    BeginBatchDraw();
                    renderer->renderPage(&demo_page);
                    FlushBatchDraw();
                    last_input = now;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    pge::Page demo_page;

    void buildPage() {
        // --- Block 1: Top row — title area (full width, thin) ---
        {
            blk::Block header;
            header.setID(1);
            header.logic_x1 =  0.0f; header.logic_y1 =  5.0f;
            header.logic_x2 = 100.0f; header.logic_y2 = 18.0f;

            ele::Element title;
            title.setID(101);
            title.type    = ele::ELE_TEXTBOX;
            title.content = "ApsisUI II  —  LectOS 2 Demo";
            title.logic_x1 = 25.0f; title.logic_y1 = 5.0f;
            title.logic_x2 = 75.0f; title.logic_y2 = 14.0f;
            header.elements.push_back(title);

            demo_page.blocks.push_back(header);
        }

        // --- Block 2: Mid row — three columns of buttons ---
        {
            blk::Block mid;
            mid.setID(2);
            mid.logic_x1 =  2.0f; mid.logic_y1 = 22.0f;
            mid.logic_x2 = 98.0f; mid.logic_y2 = 58.0f;

            // Column 1
            ele::Element btn_a;
            btn_a.setID(201);
            btn_a.type    = ele::ELE_BUTTON;
            btn_a.content = "Option A";
            btn_a.logic_x1 =  5.0f; btn_a.logic_y1 = 26.0f;
            btn_a.logic_x2 = 30.0f; btn_a.logic_y2 = 36.0f;
            mid.elements.push_back(btn_a);

            ele::Element btn_b;
            btn_b.setID(202);
            btn_b.type    = ele::ELE_BUTTON;
            btn_b.content = "Option B";
            btn_b.logic_x1 =  5.0f; btn_b.logic_y1 = 40.0f;
            btn_b.logic_x2 = 30.0f; btn_b.logic_y2 = 50.0f;
            mid.elements.push_back(btn_b);

            // Column 2
            ele::Element btn_c;
            btn_c.setID(203);
            btn_c.type    = ele::ELE_BUTTON;
            btn_c.content = "Settings";
            btn_c.logic_x1 = 35.0f; btn_c.logic_y1 = 26.0f;
            btn_c.logic_x2 = 65.0f; btn_c.logic_y2 = 36.0f;
            mid.elements.push_back(btn_c);

            ele::Element list;
            list.setID(204);
            list.type    = ele::ELE_LIST;
            list.content = "Item 1 / Item 2 / Item 3";
            list.logic_x1 = 35.0f; list.logic_y1 = 40.0f;
            list.logic_x2 = 65.0f; list.logic_y2 = 54.0f;
            mid.elements.push_back(list);

            // Column 3
            ele::Element btn_d;
            btn_d.setID(205);
            btn_d.type    = ele::ELE_BUTTON;
            btn_d.content = "Help";
            btn_d.logic_x1 = 70.0f; btn_d.logic_y1 = 26.0f;
            btn_d.logic_x2 = 95.0f; btn_d.logic_y2 = 36.0f;
            mid.elements.push_back(btn_d);

            ele::Element btn_e;
            btn_e.setID(206);
            btn_e.type    = ele::ELE_BUTTON;
            btn_e.content = "About";
            btn_e.logic_x1 = 70.0f; btn_e.logic_y1 = 40.0f;
            btn_e.logic_x2 = 95.0f; btn_e.logic_y2 = 50.0f;
            mid.elements.push_back(btn_e);

            demo_page.blocks.push_back(mid);
        }

        // --- Block 3: Bottom row — status / info ---
        {
            blk::Block footer;
            footer.setID(3);
            footer.logic_x1 =  2.0f; footer.logic_y1 = 62.0f;
            footer.logic_x2 = 98.0f; footer.logic_y2 = 92.0f;

            ele::Element info;
            info.setID(301);
            info.type    = ele::ELE_TEXTBOX;
            info.content = "Arrow keys move focus.  ESC exits.";
            info.logic_x1 =  5.0f; info.logic_y1 = 68.0f;
            info.logic_x2 = 95.0f; info.logic_y2 = 80.0f;
            footer.elements.push_back(info);

            ele::Element empty_area;
            empty_area.setID(302);
            empty_area.type    = ele::ELE_EMPTY;
            empty_area.content = "";
            empty_area.logic_x1 =  5.0f; empty_area.logic_y1 = 82.0f;
            empty_area.logic_x2 = 95.0f; empty_area.logic_y2 = 90.0f;
            footer.elements.push_back(empty_area);

            demo_page.blocks.push_back(footer);
        }
    }
};

#endif //APSISUI2_DEMO_APP_H
