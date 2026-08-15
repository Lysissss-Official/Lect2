//
// Created by archeart on 2026/7/13.
// Rewritten for the unique_ptr-owned UIStructure.
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

    enum class WidgetKind : uint8_t {
        panel,
        status_bar,
        button,
        text_box,
        list_box
    };

    struct Widget {
        lui::strc::Element* element = nullptr;
        WidgetKind kind = WidgetKind::panel;

        lui::theme::apsis::TextData text {};
        bool has_text = false;
    };

    lui::strc::Page page_;
    std::deque<Widget> widgets_;

    lui::strc::Element* first_focus_ = nullptr;

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

        if (first_focus_) {
            page_.setFocus(first_focus_);
            updateLayout();
        }

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
    static constexpr uint16_t visibleFlag() {
        return 0x04;
    }

    static constexpr uint16_t focusableFlag() {
        return 0x01;
    }

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
        lui::theme::apsis::page(
            page_context,
            nullptr
        );

        for (Widget& widget : widgets_) {
            if (!widget.element) {
                continue;
            }

            const auto node_clip =
                lui::Render::ClipRect::intersect(
                    page_context.clip,
                    rectOf(*widget.element)
                );

            if (node_clip.empty()) {
                continue;
            }

            lui::Render::DrawContext node_context {
                .target = *widget.element,
                .screen = page_context.screen,
                .clip = node_clip,
                .progress = page_context.progress
            };

            void* text_data =
                widget.has_text
                    ? static_cast<void*>(&widget.text)
                    : nullptr;

            switch (widget.kind) {
                case WidgetKind::panel:
                    lui::theme::apsis::panel(
                        node_context,
                        nullptr
                    );
                    break;

                case WidgetKind::status_bar:
                    lui::theme::apsis::statusBar(
                        node_context,
                        text_data
                    );
                    break;

                case WidgetKind::button:
                    lui::theme::apsis::button(
                        node_context,
                        text_data
                    );
                    break;

                case WidgetKind::text_box:
                    lui::theme::apsis::textBox(
                        node_context,
                        text_data
                    );
                    break;

                case WidgetKind::list_box:
                    lui::theme::apsis::listBox(
                        node_context,
                        text_data
                    );
                    break;
            }
        }
    }

    Widget& addWidget(
        WidgetKind kind,
        const lui::strc::ItemStyle& style,
        const char16_t* text = nullptr,
        lui::theme::apsis::TextAlign align =
            lui::theme::apsis::TextAlign::left
    ) {
        widgets_.emplace_back();
        Widget& widget = widgets_.back();

        widget.kind = kind;
        widget.element =
            page_.createChild<lui::strc::Element>(style);

        if (text) {
            widget.has_text = true;
            widget.text.text = text;
            widget.text.font = &lui::ext::fc24;
            widget.text.align = align;
        }

        return widget;
    }

    lui::strc::Element* addPanel(
        int32_t y1,
        int32_t y2
    ) {
        return addWidget(
            WidgetKind::panel,
            {
                .lx1 = 200,
                .ly1 = y1,
                .lx2 = 9800,
                .ly2 = y2,
                .strc_cfg = visibleFlag()
            }
        ).element;
    }

    lui::strc::Element* addStatusBar(
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        const char16_t* text
    ) {
        return addWidget(
            WidgetKind::status_bar,
            {
                .lx1 = x1,
                .ly1 = y1,
                .lx2 = x2,
                .ly2 = y2,
                .strc_cfg = visibleFlag()
            },
            text,
            lui::theme::apsis::TextAlign::center
        ).element;
    }

    lui::strc::Element* addButton(
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        const char16_t* text
    ) {
        return addWidget(
            WidgetKind::button,
            {
                .lx1 = x1,
                .ly1 = y1,
                .lx2 = x2,
                .ly2 = y2,
                .strc_cfg =
                    visibleFlag() |
                    focusableFlag()
            },
            text,
            lui::theme::apsis::TextAlign::center
        ).element;
    }

    lui::strc::Element* addTextBox(
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        const char16_t* text,
        bool focusable = false,
        lui::theme::apsis::TextAlign align =
            lui::theme::apsis::TextAlign::left
    ) {
        return addWidget(
            WidgetKind::text_box,
            {
                .lx1 = x1,
                .ly1 = y1,
                .lx2 = x2,
                .ly2 = y2,
                .strc_cfg = visibleFlag() | (focusable ? focusableFlag() : 0)
            },
            text,
            align
        ).element;
    }

    lui::strc::Element* addListBox(
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        const char16_t* text
    ) {
        return addWidget(
            WidgetKind::list_box,
            {
                .lx1 = x1,
                .ly1 = y1,
                .lx2 = x2,
                .ly2 = y2,
                .strc_cfg =
                    visibleFlag() |
                    focusableFlag()
            },
            text
        ).element;
    }

    void buildPage() {
        first_focus_ = nullptr;

        widgets_.clear();
        page_.clearChildren();

        page_.getParam(
            lui::strc::ParamIndex::logic_x1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::logic_y1
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::logic_x2
        ) = 10000;

        page_.getParam(
            lui::strc::ParamIndex::logic_y2
        ) = 10000;

        page_.getParam(
            lui::strc::ParamIndex::scroll_x
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::scroll_y
        ) = 0;

        addStatusBar(
            0, 0,
            10000, 700,
            u"LectOS 2 | ApsisUI II Demo 2"
        );

        addPanel(900, 2100);

        addTextBox(
            1800, 1150,
            8200, 1850,
            u"ApsisUI II — LectOS 2",
            false,
            lui::theme::apsis::TextAlign::center
        );

        addPanel(2300, 6100);

        first_focus_ = addButton(
            500, 2750,
            3000, 3650,
            u"Option A"
        );

        addButton(
            500, 4050,
            3000, 4950,
            u"Option B"
        );

        addButton(
            3500, 2750,
            6500, 3650,
            u"Settings"
        );

        addListBox(
            3500, 4050,
            6500, 5350,
            u"Item 1 / Item 2 / Item 3"
        );

        addButton(
            7000, 2750,
            9500, 3650,
            u"Help"
        );

        addButton(
            7000, 4050,
            9500, 4950,
            u"About"
        );

        addPanel(6400, 11100);

        addTextBox(
            500, 6800,
            4700, 7550,
            u"Sensor A: 247.3 kPa",
            true
        );

        addTextBox(
            500, 7750,
            4700, 8500,
            u"Sensor B: 18.7 C",
            true
        );

        addTextBox(
            500, 8700,
            4700, 9450,
            u"Sensor C: 1024 rpm",
            true
        );

        addTextBox(
            500, 9650,
            4700, 10400,
            u"Sensor D: 0.82 V",
            true
        );

        addTextBox(
            5300, 6800,
            9500, 7550,
            u"Axis X: +12.5 mm",
            true
        );

        addTextBox(
            5300, 7750,
            9500, 8500,
            u"Axis Y: -3.2 mm",
            true
        );

        addTextBox(
            5300, 8700,
            9500, 9450,
            u"Axis Z: +45.1 mm",
            true
        );

        addTextBox(
            5300, 9650,
            9500, 10400,
            u"Tilt: 2.8 deg",
            true
        );

        addPanel(11400, 15000);

        addButton(
            500, 12000,
            4700, 13000,
            u"Zero All Axes"
        );

        addButton(
            500, 13400,
            4700, 14400,
            u"Run Calibration"
        );

        addButton(
            5300, 12000,
            9500, 13000,
            u"Load Profile"
        );

        addButton(
            5300, 13400,
            9500, 14400,
            u"Save Profile"
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
            addTextBox(
                500, log_y,
                9500, log_y + 700,
                line
            );

            log_y += 820;
        }

        addTextBox(
            500, 22100,
            9500, 23100,
            u"Arrow keys = move focus | ESC = exit",
            false,
            lui::theme::apsis::TextAlign::center
        );
    }
};

#endif // APSISUI2_DEMO2_H