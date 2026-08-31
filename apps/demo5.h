//
// Created by archeart on 2026/08/31.
//
// ApsisUI II
// Demo5 — Structure / Render / Theme test
//

#ifndef APSISUI2_DEMO5_H
#define APSISUI2_DEMO5_H

#include <chrono>
#include <cstdint>
#include <thread>

#include "lapp/Application.h"
#include "lui/base/UIAction.h"
#include "lui/base/UIStructure.h"
#include "lui/base/UIRender.h"
#include "lui/base/UITheme.h"

#include "lui/theme/ChassisUI/UIThemeChassisUI.h"


class Demo5 final : public lapp::Application {
private:
    lui::strc::Page page_;

    lui::strc::Element* element_a_ = nullptr;
    lui::strc::Element* element_b_ = nullptr;
    lui::strc::Element* element_c_ = nullptr;
    lui::strc::Element* element_d_ = nullptr;


    lui::theme::ChassisUI theme_;

public:
    Demo5() {
        type = lapp::APP_NATIVE;
        start_page = &page_;
    }

    void app_setup() override {
        if (!screen || !renderer || !controller) {
            return;
        }

        // ---------------------------------------------------------
        // Page
        // ---------------------------------------------------------

        page_.getParam(
            lui::strc::ParamIndex::rel_x1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::rel_y1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::rel_x2
        ) = 10000;

        page_.getParam(
            lui::strc::ParamIndex::rel_y2
        ) = 4000;

        page_.getParam(
            lui::strc::ParamIndex::scroll_x
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::scroll_y
        ) = 0;

        // ---------------------------------------------------------
        // Elements
        // ---------------------------------------------------------

        element_a_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 1000,
                    .ly1 = 1000,
                    .lx2 = 3000,
                    .ly2 = 3000,
                    .item_cfg = 0x01
                }
            );

        element_b_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 3500,
                    .ly1 = 500,
                    .lx2 = 6500,
                    .ly2 = 2500,
                    .item_cfg = 0x01
                }
            );

        element_c_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 7000,
                    .ly1 = 2500,
                    .lx2 = 9000,
                    .ly2 = 4500,
                    .item_cfg = 0x01
                }
            );

        element_d_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 3500,
                    .ly1 = 5500,
                    .lx2 = 9000,
                    .ly2 = 7500,
                    .item_cfg = 0x01
                }
            );


        // ---------------------------------------------------------
        // Focus
        // ---------------------------------------------------------

        page_.rebuildFocusMap();

        page_.setFocus(element_a_);

        // ---------------------------------------------------------
        // Render
        // ---------------------------------------------------------

        renderer->setTheme(&theme_);

        renderer->setCurrentPage(&page_);

        renderer->requestReDraw(
            &page_
        );
    }

    void app_main() override {
        if (!screen || !renderer || !controller) {
            return;
        }

        using Clock = std::chrono::steady_clock;

        auto last_move = Clock::now();

        while (true) {
            /*
             * Demo5 只负责输入。
             *
             * FocusMap / Focus 状态属于 Page。
             * 绘制 / 坐标更新属于 Render。
             */

            const auto now = Clock::now();

            if (
                std::chrono::duration_cast<
                    std::chrono::milliseconds
                >(now - last_move).count() >= 80
            ) {
                switch (static_cast<int>(controller->getSW())) {
                    case 0:
                        lui::action::moveNextFocus(&page_, lui::strc::DirecIndex::right, renderer);
                        break;

                    case 90:
                        lui::action::moveNextFocus(&page_, lui::strc::DirecIndex::up, renderer);
                        break;

                    case 180:
                        lui::action::moveNextFocus(&page_, lui::strc::DirecIndex::left, renderer);
                        break;

                    case 270:
                        lui::action::moveNextFocus(&page_, lui::strc::DirecIndex::down, renderer);
                        break;

                    default:
                        break;
                }
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(16)
            );
        }
    }
};

#endif // APSISUI2_DEMO5_H