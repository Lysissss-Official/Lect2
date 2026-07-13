//
// Created by archeart on 2026/7/13.
//
// ApsisUI II - Demo3
// Nested Structure Rolling Test
//

#ifndef APSISUI2_DEMO3_H
#define APSISUI2_DEMO3_H

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

class Demo3 final : public lapp::Application {
private:
    static constexpr uint32_t LOGIC_SCALE = 10000;

    enum class Style : uint8_t {
        status,
        panel,
        button,
        text,
        container
    };

    struct Node {
        lui::strc::Element element;
        Style style = Style::panel;
        lui::theme::apsis::TextData text;
        bool has_text = false;
        bool focusable = false;
    };

    lui::strc::Page page_;
    std::deque<Node> nodes_;

    bool last_enter_ = false;
    bool last_backspace_ = false;

public:
    Demo3() {
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
            if (
                node.focusable &&
                node.element.getParent() == &page_
            ) {
                page_.setFocus(&node.element);
                break;
            }
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

            const bool enter_now =
                controller->getKB(VK_RETURN) == 1;

            const bool backspace_now =
                controller->getKB(VK_BACK) == 1;

            if (enter_now && !last_enter_) {
                if (page_.enterFocusLayer()) {
                    redraw();
                }
            }

            if (backspace_now && !last_backspace_) {
                if (page_.leaveFocusLayer()) {
                    redraw();
                }
            }

            last_enter_ = enter_now;
            last_backspace_ = backspace_now;

            const auto now = Clock::now();

            if (
                std::chrono::duration_cast<
                    std::chrono::milliseconds
                >(now - last_move).count() >= 90
            ) {
                bool moved = false;

                switch (
                    static_cast<int>(
                        controller->getSW()
                    )
                ) {
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
            static_cast<uint16_t>(
                screen->getWidth()
            ),
            static_cast<uint16_t>(
                screen->getHeight()
            ),
            LOGIC_SCALE
        );
    }

    void redraw() {
        renderer->requestReDraw(
            &page_,
            &Demo3::drawEntry,
            this
        );
    }

    static void drawEntry(
        lui::Render::DrawContext& context,
        void* extra_data
    ) {
        auto* self =
            static_cast<Demo3*>(extra_data);

        if (!self) {
            return;
        }

        self->updateLayout();

        context.clip = {
            0,
            0,
            static_cast<int32_t>(
                self->screen->getWidth()
            ),
            static_cast<int32_t>(
                self->screen->getHeight()
            )
        };

        self->drawPage(context);
    }

    static lui::Render::ClipRect rectOf(
        const lui::strc::BasicItem& item
    ) {
        return {
            item.getParam(
                lui::strc::ParamIndex::phys_x1
            ),
            item.getParam(
                lui::strc::ParamIndex::phys_y1
            ),
            item.getParam(
                lui::strc::ParamIndex::phys_x2
            ),
            item.getParam(
                lui::strc::ParamIndex::phys_y2
            )
        };
    }

    Node* findNode(
        lui::strc::BasicItem* item
    ) {
        for (Node& node : nodes_) {
            if (&node.element == item) {
                return &node;
            }
        }

        return nullptr;
    }

    void drawPage(
        lui::Render::DrawContext& context
    ) {
        lui::theme::apsis::page(
            context,
            nullptr
        );

        for (
            lui::strc::BasicItem* child :
            page_.getChildren()
        ) {
            drawRecursive(
                child,
                context
            );
        }
    }

    void drawRecursive(
        lui::strc::BasicItem* item,
        const lui::Render::DrawContext& parent
    ) {
        Node* node = findNode(item);

        if (!node) {
            return;
        }

        const auto clip =
            lui::Render::ClipRect::intersect(
                parent.clip,
                rectOf(*item)
            );

        if (clip.empty()) {
            return;
        }

        lui::Render::DrawContext context {
            .target = *item,
            .transor = parent.transor,
            .clip = clip,
            .progress = parent.progress
        };

        void* text_data =
            node->has_text
                ? static_cast<void*>(&node->text)
                : nullptr;

        switch (node->style) {
            case Style::status:
                lui::theme::apsis::statusBar(
                    context,
                    text_data
                );
                break;

            case Style::panel:
                lui::theme::apsis::panel(
                    context,
                    nullptr
                );
                break;

            case Style::button:
                lui::theme::apsis::button(
                    context,
                    text_data
                );
                break;

            case Style::text:
                lui::theme::apsis::textBox(
                    context,
                    text_data
                );
                break;

            case Style::container:
                lui::theme::apsis::panel(
                    context,
                    nullptr
                );
                lui::theme::apsis::outline(
                    context,
                    nullptr
                );
                lui::theme::apsis::drawTextContent(
                    context,
                    static_cast<
                        lui::theme::apsis::TextData*
                    >(text_data)
                );
                break;
        }

        for (
            lui::strc::BasicItem* child :
            item->getChildren()
        ) {
            drawRecursive(
                child,
                context
            );
        }
    }

    Node& addNode(
        lui::strc::BasicItem* parent,
        Style style,
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        bool focusable,
        const char16_t* text,
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
            lui::strc::ParamIndex::strc_type
        ) =
            (focusable ? 0x01 : 0x00) |
            0x04;

        parent->addChild(&element);
        return node;
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

        page_.getParam(
            lui::strc::ParamIndex::logic_y2
        ) = 10000;

        page_.getParam(
            lui::strc::ParamIndex::scroll_x
        ) = 0;

        page_.getParam(
            lui::strc::ParamIndex::scroll_y
        ) = 0;

        addNode(
            &page_,
            Style::status,
            0, 0,
            10000, 750,
            false,
            u"Demo3 | Nested Structure + Inner Scroll",
            lui::theme::apsis::TextAlign::center
        );

        // 根层左侧容器高 7600
        // 内部菜单 15000
        Node& system = addNode(
            &page_,
            Style::container,
            350, 1150,
            4850, 8750,
            true,
            u"System",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 1000,
            4200, 2200,
            true,
            u"Display",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 2600,
            4200, 3800,
            true,
            u"Power",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 4200,
            4200, 5400,
            true,
            u"Storage",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 5800,
            4200, 7000,
            true,
            u"Network",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 7400,
            4200, 8600,
            true,
            u"Bluetooth",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 9000,
            4200, 10200,
            true,
            u"Clock",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 10600,
            4200, 11800,
            true,
            u"Language",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 12200,
            4200, 13400,
            true,
            u"Developer",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &system.element,
            Style::button,
            300, 13800,
            4200, 15000,
            true,
            u"Factory Reset",
            lui::theme::apsis::TextAlign::center
        );

        // 根层右侧容器
        Node& tools = addNode(
            &page_,
            Style::container,
            5150, 1150,
            9650, 8750,
            true,
            u"Tools",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &tools.element,
            Style::button,
            300, 1000,
            4200, 2200,
            true,
            u"Console",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &tools.element,
            Style::button,
            300, 2600,
            4200, 3800,
            true,
            u"File Manager",
            lui::theme::apsis::TextAlign::center
        );

        // 第二级容器高 4600
        // 内部菜单 11600
        Node& diagnostics = addNode(
            &tools.element,
            Style::container,
            300, 4200,
            4200, 8800,
            true,
            u"Diagnostics",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 900,
            3650, 1900,
            true,
            u"LCD Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 2300,
            3650, 3300,
            true,
            u"Input Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 3700,
            3650, 4700,
            true,
            u"SPI Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 5100,
            3650, 6100,
            true,
            u"I2C Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 6500,
            3650, 7500,
            true,
            u"Memory Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 7900,
            3650, 8900,
            true,
            u"Flash Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 9300,
            3650, 10300,
            true,
            u"PSRAM Test",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &diagnostics.element,
            Style::button,
            250, 10700,
            3650, 11700,
            true,
            u"Stress Test",
            lui::theme::apsis::TextAlign::center
        );

        // Feature 测试进入前后外层容器自动滚动情况
        addNode(
            &tools.element,
            Style::button,
            300, 9400,
            4200, 10600,
            true,
            u"Profiler",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &tools.element,
            Style::button,
            300, 11000,
            4200, 12200,
            true,
            u"About",
            lui::theme::apsis::TextAlign::center
        );

        addNode(
            &page_,
            Style::text,
            1250, 9100,
            8750, 9850,
            false,
            u"Enter: child | Backspace: parent | move to scroll",
            lui::theme::apsis::TextAlign::center
        );
    }
};

#endif // APSISUI2_DEMO3_H