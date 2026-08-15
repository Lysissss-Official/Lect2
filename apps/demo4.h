#ifndef APSISUI2_DEMO4_H
#define APSISUI2_DEMO4_H

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <sstream>
#include <string>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include <iostream>
#include <thread>
#endif

#include "lapp/Application.h"
#include "lmath/scalrr/scalrr_backend.hpp"
#include "lui/base/UIRender.h"
#include "lui/base/UIStructure.h"
#include "lui/theme/ApsisUI2/UIThemeApsisUI2.h"
#include "lui/extension/font/font_con_24.h"

class Demo4 final : public lapp::Application {
    static constexpr int32_t SCALE = 10000;
    static constexpr char TAG[] = "Demo4";

    enum class View : uint8_t { lcd, ui, nested, math };
    enum class Style : uint8_t { status, panel, button, text, outline };

    struct Item {
        lui::strc::Element element;
        Style style = Style::panel;
        lui::theme::apsis::TextData text;
        bool has_text = false;
        bool focusable = false;
    };

    struct MathResult {
        const char* name = "";
        uint64_t us = 0;
        bool pass = false;
    };

    lui::strc::Page page_;
    std::deque<Item> items_;
    View view_ = View::lcd;
    uint32_t tick_ = 0;

    std::array<MathResult, 6> math_{};
    std::array<std::u16string, 6> math_text_{};

public:
    Demo4() {
        type = lapp::APP_NATIVE;
        start_page = &page_;
    }

    void app_setup() override {
        if (!screen || !renderer) return;

        runMath();
        rebuild();

        renderer->setCurrentPage(&page_);
        redraw();
    }

    void app_main() override {
        while (screen && renderer) {
            autoStep();
#ifdef ESP_PLATFORM
            vTaskDelay(pdMS_TO_TICKS(1100));
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(1100));
#endif
        }
    }

private:
    static std::string numberText(const Unbounded& value) {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    template<class F>
    static MathResult test(const char* name, const char* expected, F&& fn) {
        const auto begin = std::chrono::steady_clock::now();
        const std::string value = numberText(fn());
        const auto end = std::chrono::steady_clock::now();

        return {
            name,
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    end - begin
                ).count()
            ),
            value == expected
        };
    }

    void runMath() {
        math_[0] = test("ADD", "111111111011111111100", [] {
            return hqRead("12345678901234567890") +
                   hqRead("98765432109876543210");
        });

        math_[1] = test("MUL",
            "1219326311370217952237463801111263526900", [] {
            return hqRead("12345678901234567890") *
                   hqRead("98765432109876543210");
        });

        math_[2] = test("POW", "1267650600228229401496703205376", [] {
            return scal::quickPow(hqRead("2"), 100);
        });

        math_[3] = test("ROOT", "10000000000", [] {
            return scal::integerRoot(
                hqRead("100000000000000000000"), 2
            );
        });

        math_[4] = test("FACT",
            "265252859812191058636308480000000", [] {
            return scal::factorial(hqRead("30"));
        });

        math_[5] = test("NCR", "155117520", [] {
            return scal::nCr(hqRead("30"), hqRead("15"));
        });

        for (std::size_t i = 0; i < math_.size(); ++i) {
            const auto& result = math_[i];

            std::string line =
                std::string(result.pass ? "PASS " : "FAIL ") +
                result.name + " " +
                std::to_string(result.us) + "us";

            math_text_[i].assign(line.begin(), line.end());

#ifdef ESP_PLATFORM
            ESP_LOGI(
                TAG, "%s %llu us %s",
                result.name,
                static_cast<unsigned long long>(result.us),
                result.pass ? "PASS" : "FAIL"
            );
#endif
        }
    }

    Item& add(
        lui::strc::BasicItem* parent,
        Style style,
        int32_t x1, int32_t y1,
        int32_t x2, int32_t y2,
        bool focusable,
        const char16_t* label = nullptr
    ) {
        items_.emplace_back();
        Item& item = items_.back();

        item.style = style;
        item.focusable = focusable;

        if (label) {
            item.has_text = true;
            item.text.text = label;
            item.text.font = &lui::ext::fc24;
            item.text.align = lui::theme::apsis::TextAlign::center;
        }

        item.element.getParam(lui::strc::ParamIndex::logic_x1) = x1;
        item.element.getParam(lui::strc::ParamIndex::logic_y1) = y1;
        item.element.getParam(lui::strc::ParamIndex::logic_x2) = x2;
        item.element.getParam(lui::strc::ParamIndex::logic_y2) = y2;
        item.element.getParam(lui::strc::ParamIndex::scroll_x) = 0;
        item.element.getParam(lui::strc::ParamIndex::scroll_y) = 0;
        item.element.getParam(lui::strc::ParamIndex::item_cfg) =
            (focusable ? 0x01 : 0x00) | 0x04;

        parent->addChild(&item.element);
        return item;
    }

    void header(const char16_t* text) {
        add(&page_, Style::status, 0, 0, 10000, 1450, false, text);
    }

    void clearPage() {
        page_.clearChildren();
        items_.clear();

        page_.getParam(lui::strc::ParamIndex::logic_x1) = 0;
        page_.getParam(lui::strc::ParamIndex::logic_y1) = 0;
        page_.getParam(lui::strc::ParamIndex::logic_x2) = SCALE;
        page_.getParam(lui::strc::ParamIndex::logic_y2) = SCALE;
        page_.getParam(lui::strc::ParamIndex::scroll_x) = 0;
        page_.getParam(lui::strc::ParamIndex::scroll_y) = 0;
        page_.focus = nullptr;
    }

    void buildLCD() {
        header(u"Demo4 | LCD");

        add(&page_, Style::button, 500, 2200, 3000, 5200, true, u"POINT");
        add(&page_, Style::button, 3750, 2200, 6250, 5200, true, u"LINE");
        add(&page_, Style::button, 7000, 2200, 9500, 5200, true, u"RECT");

        add(
            &page_, Style::text,
            1300, 6500, 8700, 8500,
            false, u"Framebuffer + SPI refresh"
        );
    }

    void buildUI() {
        header(u"Demo4 | Controls");

        add(&page_, Style::button, 500, 2300, 3000, 5700, true, u"OPTION A");
        add(&page_, Style::button, 3750, 2300, 6250, 5700, true, u"OPTION B");
        add(&page_, Style::button, 7000, 2300, 9500, 5700, true, u"SETTINGS");

        add(
            &page_, Style::text,
            1900, 7000, 8100, 8800,
            false, u"Automatic focus"
        );
    }

    void buildNested() {
        header(u"Demo4 | Nested");

        Item& left = add(
            &page_, Style::outline,
            450, 2000, 4750, 9000,
            true, u"SYSTEM"
        );

        add(&left.element, Style::button, 600, 1900, 3700, 4100, true, u"DISPLAY");
        add(&left.element, Style::button, 600, 5000, 3700, 7200, true, u"POWER");

        Item& right = add(
            &page_, Style::outline,
            5250, 2000, 9550, 9000,
            true, u"TOOLS"
        );

        add(&right.element, Style::button, 600, 1900, 3700, 4100, true, u"LCD TEST");
        add(&right.element, Style::button, 600, 5000, 3700, 7200, true, u"SCALRR");
    }

    void buildMath() {
        header(u"Demo4 | SCalRR");

        for (std::size_t i = 0; i < math_.size(); ++i) {
            const int column = static_cast<int>(i % 3);
            const int row = static_cast<int>(i / 3);

            const int32_t x1 = 350 + column * 3250;
            const int32_t y1 = 2200 + row * 3500;

            add(
                &page_,
                math_[i].pass ? Style::text : Style::button,
                x1, y1,
                x1 + 2850, y1 + 2600,
                true,
                math_text_[i].c_str()
            );
        }
    }

    void rebuild() {
        clearPage();

        switch (view_) {
            case View::lcd: buildLCD(); break;
            case View::ui: buildUI(); break;
            case View::nested: buildNested(); break;
            case View::math: buildMath(); break;
        }

        updateLayout();
        page_.rebuildFocusMap();

        for (Item& item : items_) {
            if (
                item.focusable &&
                item.element.getParent() == &page_
            ) {
                page_.setFocus(&item.element);
                break;
            }
        }

        updateLayout();
    }

    void updateLayout() {
        page_.updatePhysical(
            static_cast<uint16_t>(screen->getWidth()),
            static_cast<uint16_t>(screen->getHeight()),
            SCALE
        );
    }

    void autoStep() {
        ++tick_;

        if ((tick_ % 7U) == 0U) {
            view_ = static_cast<View>(
                (static_cast<uint8_t>(view_) + 1U) % 4U
            );

            rebuild();
            renderer->setCurrentPage(&page_);
            redraw();
            return;
        }

#ifdef ESP_PLATFORM
        const uint32_t action = esp_random() % 6U;
#else
        const uint32_t action = tick_ % 6U;
#endif

        bool changed = false;

        switch (action) {
            case 0:
                changed = page_.moveFocus(lui::strc::DirecIndex::right);
                break;
            case 1:
                changed = page_.moveFocus(lui::strc::DirecIndex::down);
                break;
            case 2:
                changed = page_.moveFocus(lui::strc::DirecIndex::left);
                break;
            case 3:
                changed = page_.moveFocus(lui::strc::DirecIndex::up);
                break;
            case 4:
                changed = page_.enterFocusLayer();
                break;
            case 5:
                changed = page_.leaveFocusLayer();
                break;
        }

        if (changed) {
            updateLayout();
            redraw();
        }
    }

    void redraw() {
        renderer->requestReDraw(&page_, &Demo4::drawEntry, this);
    }

    static void drawEntry(
        lui::Render::DrawContext& context,
        void* extra
    ) {
        auto* self = static_cast<Demo4*>(extra);
        if (!self) return;

        self->updateLayout();

        context.clip = {
            0, 0,
            static_cast<int32_t>(self->screen->getWidth()),
            static_cast<int32_t>(self->screen->getHeight())
        };

        self->drawPage(context);

        if (auto* driver = context.screen.getDriver()) {
            driver->refreshScreenCmd();
        }
    }

    static lui::Render::ClipRect rect(
        const lui::strc::BasicItem& item
    ) {
        return {
            item.getParam(lui::strc::ParamIndex::phys_x1),
            item.getParam(lui::strc::ParamIndex::phys_y1),
            item.getParam(lui::strc::ParamIndex::phys_x2),
            item.getParam(lui::strc::ParamIndex::phys_y2)
        };
    }

    Item* find(lui::strc::BasicItem* target) {
        for (Item& item : items_) {
            if (&item.element == target) return &item;
        }
        return nullptr;
    }

    void drawPage(lui::Render::DrawContext& context) {
        lui::theme::apsis::page(context, nullptr);

        if (view_ == View::lcd) {
            drawLCDOverlay(context);
        }

        for (auto* child : page_.getChildren()) {
            drawRecursive(child, context);
        }
    }

    void drawLCDOverlay(lui::Render::DrawContext& context) {
        auto* driver = context.screen.getDriver();
        if (!driver) return;

        const uint16_t width =
            static_cast<uint16_t>(context.screen.getWidth());

        const uint16_t height =
            static_cast<uint16_t>(context.screen.getHeight());

        driver->drawLineCmd(0, 0, width - 1, height - 1, 0x000000);
        driver->drawLineCmd(width - 1, 0, 0, height - 1, 0x000000);

        for (uint16_t x = 0; x < width; x += 12) {
            driver->drawPixelCmd(x, height / 2, 0x000000);
        }
    }

    void drawRecursive(
        lui::strc::BasicItem* target,
        const lui::Render::DrawContext& parent
    ) {
        Item* item = find(target);
        if (!item) return;

        const auto clip =
            lui::Render::ClipRect::intersect(
                parent.clip,
                rect(*target)
            );

        if (clip.empty()) return;

        lui::Render::DrawContext context {
            .target = *target,
            .screen = parent.screen,
            .clip = clip,
            .progress = parent.progress
        };

        void* text =
            item->has_text
                ? static_cast<void*>(&item->text)
                : nullptr;

        switch (item->style) {
            case Style::status:
                lui::theme::apsis::statusBar(context, text);
                break;
            case Style::panel:
                lui::theme::apsis::panel(context, nullptr);
                break;
            case Style::button:
                lui::theme::apsis::button(context, text);
                break;
            case Style::text:
                lui::theme::apsis::textBox(context, text);
                break;
            case Style::outline:
                lui::theme::apsis::panel(context, nullptr);
                lui::theme::apsis::outline(context, nullptr);
                if (item->has_text) {
                    lui::theme::apsis::drawTextContent(
                        context,
                        &item->text
                    );
                }
                break;
        }

        for (auto* child : target->getChildren()) {
            drawRecursive(child, context);
        }
    }
};

#endif