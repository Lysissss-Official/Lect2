//
// Created by archeart on 2026/7/13.
//
// ApsisUI II - Demo2
// Basic Structure Focusing Test
//

#ifndef APSISUI2_DEMO2_H
#define APSISUI2_DEMO2_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <thread>

#include "lapp/Application.h"
#include "lui/base/UIStructure.h"
#include "lui/base/UICtrller.h"
#include "lui/base/UIRender.h"
#include "lui/theme/ApsisUI2/UIThemeApsisUI2.h"
#include "lui/extension/font/font_con_24.h"

class Demo2 final : public lapp::Application {
private:
    static constexpr uint32_t LOGIC_SCALE = 10000;

    enum class NodeStyle : uint8_t {
        panel,
        status_bar,
        button,
        text_box,
        list_box
    };

    struct Node {
        lui::strc::Element element;
        NodeStyle style = NodeStyle::panel;

        lui::theme::apsis::TextData text;
        bool has_text = false;
        bool focusable = false;
    };

    lui::strc::Page page_;
    std::deque<Node> nodes_;

public:
    Demo2() {
        type = lapp::APP_NATIVE;
        start_page = &page_;
    }

    void app_setup() override {
        if (!screen || !renderer || !controller) {
            return;
        }

        buildPage();
        updateLayout();

        page_.rebuildFocusMap();

        for (Node& node : nodes_) {
            if (node.focusable) {
                page_.setFocus(&node.element);
                break;
            }
        }

        updateLayout();

        renderer->setCurrentPage(&page_);
        redraw();
    }

    void app_main() override {
        if (!screen || !renderer || !controller) {
            return;
        }

        using Clock = std::chrono::steady_clock;
        auto last_move = Clock::now();

        while (true) {
            if (controller->getKB(VK_ESCAPE) == 1) {
                break;
            }

            const auto now = Clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_move
                );

            if (elapsed.count() >= 80) {
                bool moved = false;

                switch (static_cast<int>(controller->getSW())) {
                    case 0:
                        moved = page_.moveFocus(
                            lui::strc::DirecIndex::right
                        );
                        break;

                    case 90:
                        moved = page_.moveFocus(
                            lui::strc::DirecIndex::up
                        );
                        break;

                    case 180:
                        moved = page_.moveFocus(
                            lui::strc::DirecIndex::left
                        );
                        break;

                    case 270:
                        moved = page_.moveFocus(
                            lui::strc::DirecIndex::down
                        );
                        break;

                    default:
                        break;
                }

                if (moved) {
                    last_move = now;
                    updateLayout();
                    redraw();
                }
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(16)
            );
        }
    }

private:
    void updateLayout() {
        page_.updatePhysical(
            static_cast<uint16_t>(screen->getWidth()),
            static_cast<uint16_t>(screen->getHeight()),
            LOGIC_SCALE
        );
    }

    void redraw() {
        renderer->requestReDraw(
            &page_,
            &Demo2::drawPageEntry,
            this
        );
    }

    static void drawPageEntry(
        lui::Render::DrawContext& context,
        void* extra_data
    ) {
        auto* self = static_cast<Demo2*>(extra_data);

        if (self) {
            self->drawPage(context);
        }
    }

    static lui::Render::ClipRect rectOf(
        const lui::strc::BasicItem& item
    ) {
        return {
            item.getParam(lui::strc::ParamIndex::phys_x1),
            item.getParam(lui::strc::ParamIndex::phys_y1),
            item.getParam(lui::strc::ParamIndex::phys_x2),
            item.getParam(lui::strc::ParamIndex::phys_y2)
        };
    }

    void drawPage(
        lui::Render::DrawContext& page_context
    ) {
        // 整页背景。
        lui::theme::apsis::page(
            page_context,
            nullptr
        );

        // nodes_ 的顺序就是绘制顺序：先 panel，后内部 Element。
        for (Node& node : nodes_) {
            const lui::Render::ClipRect node_clip =
                lui::Render::ClipRect::intersect(
                    page_context.clip,
                    rectOf(node.element)
                );

            if (node_clip.empty()) {
                continue;
            }

            lui::Render::DrawContext node_context {
                .target = node.element,
                .screen = page_context.screen,
                .clip = node_clip,
                .progress = page_context.progress
            };

            void* text_data =
                node.has_text
                    ? static_cast<void*>(&node.text)
                    : nullptr;

            switch (node.style) {
                case NodeStyle::panel:
                    lui::theme::apsis::panel(
                        node_context,
                        nullptr
                    );
                    break;

                case NodeStyle::status_bar:
                    lui::theme::apsis::statusBar(
                        node_context,
                        text_data
                    );
                    break;

                case NodeStyle::button:
                    lui::theme::apsis::button(
                        node_context,
                        text_data
                    );
                    break;

                case NodeStyle::text_box:
                    lui::theme::apsis::textBox(
                        node_context,
                        text_data
                    );
                    break;

                case NodeStyle::list_box:
                    lui::theme::apsis::listBox(
                        node_context,
                        text_data
                    );
                    break;
            }
        }
    }

    Node& addNode(
        NodeStyle style,
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        bool focusable = false,
        const char16_t* text = nullptr,
        lui::theme::apsis::TextAlign align =
            lui::theme::apsis::TextAlign::left
    ) {
        nodes_.emplace_back();
        Node& node = nodes_.back();

        node.style = style;
        node.focusable = focusable;

        if (text) {
            node.has_text = true;
            node.text.text = text;
            node.text.font = &lui::ext::fc24;
            node.text.align = align;
        }

        auto& element = node.element;

        element.getParam(
            lui::strc::ParamIndex::logic_x1
        ) = x1;

        element.getParam(
            lui::strc::ParamIndex::logic_y1
        ) = y1;

        element.getParam(
            lui::strc::ParamIndex::logic_x2
        ) = x2;

        element.getParam(
            lui::strc::ParamIndex::logic_y2
        ) = y2;

        element.getParam(
            lui::strc::ParamIndex::scroll_x
        ) = 0;

        element.getParam(
            lui::strc::ParamIndex::scroll_y
        ) = 0;

        // A: focusable, C: visible
        element.getParam(
            lui::strc::ParamIndex::strc_type
        ) =
            (focusable ? 0x01 : 0x00) |
            0x04;

        page_.addChild(&element);
        return node;
    }

    void addPanel(
        int32_t y1,
        int32_t y2
    ) {
        addNode(
            NodeStyle::panel,
            200,
            y1,
            9800,
            y2
        );
    }

    void buildPage() {
        nodes_.clear();

        page_.getParam(
            lui::strc::ParamIndex::logic_x1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::logic_y1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::logic_x2
        ) = 10000;

        // 页面高度可超过一屏；10000 只是缩放单位。
        page_.getParam(
            lui::strc::ParamIndex::logic_y2
        ) = 10000;//23200;

        page_.getParam(
            lui::strc::ParamIndex::scroll_x
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::scroll_y
        ) = 0;

        addNode(
            NodeStyle::status_bar,
            0, 0,
            10000, 700,
            false,
            u"LectOS 2 | ApsisUI II Demo 2",
            lui::theme::apsis::TextAlign::center
        );

        addPanel(900, 2100);

        addNode(
            NodeStyle::text_box,
            1800, 1150,
            8200, 1850,
            false,
            u"ApsisUI II — LectOS 2",
            lui::theme::apsis::TextAlign::center
        );

        addPanel(2300, 6100);

        addNode(
            NodeStyle::button,
            500, 2750,
            3000, 3650,
            true,
            u"Option A",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            500, 4050,
            3000, 4950,
            true,
            u"Option B",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            3500, 2750,
            6500, 3650,
            true,
            u"Settings",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::list_box,
            3500, 4050,
            6500, 5350,
            true,
            u"Item 1 / Item 2 / Item 3"
        );

        addNode(
            NodeStyle::button,
            7000, 2750,
            9500, 3650,
            true,
            u"Help",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            7000, 4050,
            9500, 4950,
            true,
            u"About",
            lui::theme::apsis::TextAlign::center
        );

        addPanel(6400, 11100);

        addNode(
            NodeStyle::text_box,
            500, 6800,
            4700, 7550,
            true,
            u"Sensor A: 247.3 kPa"
        );

        addNode(
            NodeStyle::text_box,
            500, 7750,
            4700, 8500,
            true,
            u"Sensor B: 18.7 C"
        );

        addNode(
            NodeStyle::text_box,
            500, 8700,
            4700, 9450,
            true,
            u"Sensor C: 1024 rpm"
        );

        addNode(
            NodeStyle::text_box,
            500, 9650,
            4700, 10400,
            true,
            u"Sensor D: 0.82 V"
        );

        addNode(
            NodeStyle::text_box,
            5300, 6800,
            9500, 7550,
            true,
            u"Axis X: +12.5 mm"
        );

        addNode(
            NodeStyle::text_box,
            5300, 7750,
            9500, 8500,
            true,
            u"Axis Y: -3.2 mm"
        );

        addNode(
            NodeStyle::text_box,
            5300, 8700,
            9500, 9450,
            true,
            u"Axis Z: +45.1 mm"
        );

        addNode(
            NodeStyle::text_box,
            5300, 9650,
            9500, 10400,
            true,
            u"Tilt: 2.8 deg"
        );

        addPanel(11400, 15000);

        addNode(
            NodeStyle::button,
            500, 12000,
            4700, 13000,
            true,
            u"Zero All Axes",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            500, 13400,
            4700, 14400,
            true,
            u"Run Calibration",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            5300, 12000,
            9500, 13000,
            true,
            u"Load Profile",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            NodeStyle::button,
            5300, 13400,
            9500, 14400,
            true,
            u"Save Profile",
            lui::theme::apsis::TextAlign::center
        );

        addPanel(15300, 21500);

        static constexpr const char16_t* LOG_LINES[] = {
            u"[12:00] System boot ................ OK",
            u"[12:01] SPI bus init ............... OK",
            u"[12:01] Sensor polling started ..... OK",
            u"[12:02] Calibration data loaded .... OK",
            u"[12:03] Axis calculator online ..... OK",
            u"[12:04] Watchdog armed ............. OK",
            u"[12:05] Ready for input ............ OK"
        };

        int32_t log_y = 15700;

        for (const char16_t* line : LOG_LINES) {
            addNode(
                NodeStyle::text_box,
                500, log_y,
                9500, log_y + 700,
                false,
                line
            );

            log_y += 820;
        }

        addNode(
            NodeStyle::text_box,
            500, 22100,
            9500, 23100,
            false,
            u"Arrow keys = move focus | ESC = exit",
            lui::theme::apsis::TextAlign::center
        );
    }
};

#endif // APSISUI2_DEMO2_H
