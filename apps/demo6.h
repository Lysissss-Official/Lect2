//
// Created by archeart on 2026/09/06.
//
// ApsisUI II
// Demo6 — Nested Focus / Scroll / Animation stress test
//

#ifndef APSISUI2_DEMO6_H
#define APSISUI2_DEMO6_H

#include <chrono>
#include <cstdint>
#include <thread>

#include "lapp/Application.h"
#include "lui/base/UIAction.h"
#include "lui/base/UIStructure.h"
#include "lui/base/UIRender.h"
#include "lui/base/UITheme.h"

#include "lui/theme/ChassisUI/UIThemeChassisUI.h"


class Demo6 final : public lapp::Application {
private:
    using Clock = std::chrono::steady_clock;

    static constexpr int32_t LOGIC_SCALE = 10000;

    lui::strc::Page page_;

    lui::strc::Element* outer_a_ = nullptr;
    lui::strc::Element* outer_b_ = nullptr;
    lui::strc::Element* outer_c_ = nullptr;
    lui::strc::Element* outer_d_ = nullptr;

    lui::strc::Element* inner_a_ = nullptr;
    lui::strc::Element* inner_b_ = nullptr;
    lui::strc::Element* inner_c_ = nullptr;
    lui::strc::Element* inner_d_ = nullptr;

    lui::strc::Element* deep_a_ = nullptr;
    lui::strc::Element* deep_b_ = nullptr;
    lui::strc::Element* deep_c_ = nullptr;
    lui::strc::Element* deep_d_ = nullptr;

    lui::theme::ChassisUI theme_;

    // ---------------------------------------------------------
    // 按键边沿状态
    // true  = 上一轮已经按下
    // false = 上一轮没有按下
    // ---------------------------------------------------------

    bool last_l_ = false;
    bool last_p_ = false;

    // 上一次方向是否存在
    bool last_direction_active_ = false;

public:
    Demo6() {
        type = lapp::APP_NATIVE;
        start_page = &page_;
    }

    void app_setup() override {
        if (!screen || !renderer || !controller) {
            return;
        }

        // -----------------------------------------------------
        // Page
        // -----------------------------------------------------

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

        // -----------------------------------------------------
        // 外层元素
        // -----------------------------------------------------

        outer_a_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 500,
                    .ly1 = 300,
                    .lx2 = 3000,
                    .ly2 = 1200,
                    .item_cfg = 0x01
                }
            );

        outer_b_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 3500,
                    .ly1 = 300,
                    .lx2 = 6000,
                    .ly2 = 1200,
                    .item_cfg = 0x01
                }
            );

        outer_c_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 6500,
                    .ly1 = 300,
                    .lx2 = 9000,
                    .ly2 = 1200,
                    .item_cfg = 0x01
                }
            );

        outer_d_ =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 500,
                    .ly1 = 7000,
                    .lx2 = 9500,
                    .ly2 = 7900,
                    .item_cfg = 0x01
                }
            );

        // -----------------------------------------------------
        // 第一层滚动容器
        //
        // 本身高度只有 4000
        // 内容高度远大于 4000
        // -----------------------------------------------------

        auto* outer_container =
            page_.createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 500,
                    .ly1 = 1500,
                    .lx2 = 9500,
                    .ly2 = 5500,
                    .item_cfg = 0x01
                }
            );

        // -----------------------------------------------------
        // 第一层内容
        // -----------------------------------------------------

        inner_a_ =
            outer_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 500,
                    .ly1 = 300,
                    .lx2 = 3000,
                    .ly2 = 1100,
                    .item_cfg = 0x01
                }
            );

        inner_b_ =
            outer_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 3500,
                    .ly1 = 300,
                    .lx2 = 6000,
                    .ly2 = 1100,
                    .item_cfg = 0x01
                }
            );

        inner_c_ =
            outer_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 500,
                    .ly1 = 1800,
                    .lx2 = 3000,
                    .ly2 = 2600,
                    .item_cfg = 0x01
                }
            );

        inner_d_ =
            outer_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 3500,
                    .ly1 = 1800,
                    .lx2 = 6000,
                    .ly2 = 2600,
                    .item_cfg = 0x01
                }
            );

        // -----------------------------------------------------
        // 第二层嵌套容器
        // -----------------------------------------------------

        auto* inner_container =
            outer_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 6500,
                    .ly1 = 300,
                    .lx2 = 9000,
                    .ly2 = 1800,
                    .item_cfg = 0x01
                }
            );

        // -----------------------------------------------------
        // 第二层内容
        // -----------------------------------------------------

        deep_a_ =
            inner_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 200,
                    .ly1 = 200,
                    .lx2 = 2200,
                    .ly2 = 800,
                    .item_cfg = 0x01
                }
            );

        deep_b_ =
            inner_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 200,
                    .ly1 = 1000,
                    .lx2 = 2200,
                    .ly2 = 1600,
                    .item_cfg = 0x01
                }
            );

        deep_c_ =
            inner_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 200,
                    .ly1 = 1800,
                    .lx2 = 2200,
                    .ly2 = 2400,
                    .item_cfg = 0x01
                }
            );

        deep_d_ =
            inner_container->createChild<lui::strc::Element>(
                lui::strc::ItemStyle {
                    .lx1 = 200,
                    .ly1 = 2600,
                    .lx2 = 2200,
                    .ly2 = 3200,
                    .item_cfg = 0x01
                }
            );

        // -----------------------------------------------------
        // 底部大范围内容
        // 用于测试 Page 本身的滚动
        // -----------------------------------------------------

        page_.createChild<lui::strc::Element>(
            lui::strc::ItemStyle {
                .lx1 = 1000,
                .ly1 = 8500,
                .lx2 = 9000,
                .ly2 = 9300,
                .item_cfg = 0x01
            }
        );

        page_.createChild<lui::strc::Element>(
            lui::strc::ItemStyle {
                .lx1 = 1000,
                .ly1 = 10000,
                .lx2 = 9000,
                .ly2 = 10800,
                .item_cfg = 0x01
            }
        );

        page_.createChild<lui::strc::Element>(
            lui::strc::ItemStyle {
                .lx1 = 1000,
                .ly1 = 11500,
                .lx2 = 9000,
                .ly2 = 12300,
                .item_cfg = 0x01
            }
        );

        page_.createChild<lui::strc::Element>(
            lui::strc::ItemStyle {
                .lx1 = 1000,
                .ly1 = 13000,
                .lx2 = 9000,
                .ly2 = 13800,
                .item_cfg = 0x01
            }
        );

        // -----------------------------------------------------
        // Focus
        // -----------------------------------------------------

        page_.rebuildFocusMap();

        page_.setFocus(outer_a_);

        // -----------------------------------------------------
        // Physical layout
        // -----------------------------------------------------

        updateAbsolute();

        // -----------------------------------------------------
        // Render
        // -----------------------------------------------------

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

        while (true) {

            // -------------------------------------------------
            // ESC
            // -------------------------------------------------

            if (controller->getKB(VK_ESCAPE) == 1) {
                break;
            }

            // -------------------------------------------------
            // 当前按键状态
            // -------------------------------------------------

            const bool l_pressed =
                controller->getKB('L') == 1;

            const bool p_pressed =
                controller->getKB('P') == 1;

            // -------------------------------------------------
            // L：进入 Focus Layer
            //
            // 只在“刚按下”的瞬间触发
            // -------------------------------------------------

            if (l_pressed && !last_l_) {
                if (page_.enterFocusLayer()) {
                    redraw();
                }
            }

            // -------------------------------------------------
            // P：退出 Focus Layer
            //
            // 只在“刚按下”的瞬间触发
            // -------------------------------------------------

            if (p_pressed && !last_p_) {
                if (page_.leaveFocusLayer()) {
                    redraw();
                }
            }

            // -------------------------------------------------
            // 更新 L / P 上一帧状态
            // -------------------------------------------------

            last_l_ = l_pressed;
            last_p_ = p_pressed;

            // -------------------------------------------------
            // 方向键
            //
            // getSW() = 没有方向时应返回其它值
            //
            // 这里同样只在方向从“无 → 有”时触发。
            // 因此按住不会疯狂重复。
            // -------------------------------------------------

            const int direction =
                static_cast<int>(controller->getSW());

            const bool direction_active =
                direction == 0 ||
                direction == 90 ||
                direction == 180 ||
                direction == 270;

            if (direction_active && !last_direction_active_) {

                bool moved = false;

                switch (direction) {
                    case 0:
                        moved = lui::action::moveNextFocus(
                            &page_,
                            lui::strc::DirecIndex::right,
                            renderer
                        );
                        break;

                    case 90:
                        moved = lui::action::moveNextFocus(
                            &page_,
                            lui::strc::DirecIndex::up,
                            renderer
                        );
                        break;

                    case 180:
                        moved = lui::action::moveNextFocus(
                            &page_,
                            lui::strc::DirecIndex::left,
                            renderer
                        );
                        break;

                    case 270:
                        moved = lui::action::moveNextFocus(
                            &page_,
                            lui::strc::DirecIndex::down,
                            renderer
                        );
                        break;

                    default:
                        break;
                }

                if (moved) {
                    redraw();
                }
            }

            last_direction_active_ =
                direction_active;

            // -------------------------------------------------
            // 不要让 app_main 自己跑成 busy loop
            // -------------------------------------------------

            std::this_thread::sleep_for(
                std::chrono::milliseconds(8)
            );
        }
    }

private:

    void updateAbsolute() {
        page_.updateAbsolute(
            static_cast<uint16_t>(screen->getWidth()),
            static_cast<uint16_t>(screen->getHeight()),
            LOGIC_SCALE
        );
    }

    void redraw() {
        updateAbsolute();

        renderer->requestReDraw(
            &page_
        );
    }
};

#endif // APSISUI2_DEMO6_H