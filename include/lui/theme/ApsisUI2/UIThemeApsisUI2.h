//
// Created by archeart on 2026/7/12.
//

#ifndef APSISUI2_UITHEMEAPSISUI2_H
#define APSISUI2_UITHEMEAPSISUI2_H

#include <algorithm>
#include <cstdint>
#include <string>

#include "../../base/UIRender.h"
#include "../../extension/font/FontBase.h"

namespace lui::theme::apsis {

    namespace color {
        constexpr uint32_t BG_DARK       = 0x0A0A0A;
        constexpr uint32_t BG_PANEL      = 0x121212;
        constexpr uint32_t ACCENT        = 0x303000;
        constexpr uint32_t TEXT_PRIMARY  = 0xE0E0E0;
        constexpr uint32_t TEXT_DIM      = 0x808060;
        constexpr uint32_t FOCUS_BORDER  = 0xCCCC00;
        constexpr uint32_t BUTTON_FILL   = 0x252500;
        constexpr uint32_t BUTTON_HOVER  = 0x454500;
        constexpr uint32_t STATUS_BAR_BG = 0x060606;
        constexpr uint32_t LIST_MARKER   = 0x808060;
    }

    enum class TextAlign : uint8_t {
        left,
        center
    };

    struct TextData {
        const char16_t* text = nullptr;
        ext::FontBase* font = nullptr;

        TextAlign align = TextAlign::left;

        int32_t padding_x = 6;
        int32_t padding_y = 4;

        uint32_t normal_color = color::TEXT_PRIMARY;
        uint32_t focused_color = 0xFFFFFF;
    };

    inline bool isFocused(
        const Render::DrawContext& ctx
    ) {
        auto* element =
            dynamic_cast<const strc::Element*>(
                &ctx.target
            );

        return element && element->focused;
    }

    inline void drawTextContent(
        Render::DrawContext& ctx,
        TextData* data
    ) {
        if (
            !data ||
            !data->text ||
            !data->font
        ) {
            return;
        }

        std::u16string text(data->text);

        int32_t x =
            ctx.clip.x1 +
            data->padding_x;

        const int32_t y =
            ctx.clip.y1 +
            data->padding_y;

        if (data->align == TextAlign::center) {
            const int32_t text_width =
                static_cast<int32_t>(
                    data->font->getStringWidth(text)
                );

            x =
                ctx.clip.x1 +
                (
                    ctx.clip.x2 -
                    ctx.clip.x1 -
                    text_width
                ) / 2;
        }

        const uint32_t text_color =
            isFocused(ctx)
                ? data->focused_color
                : data->normal_color;

        data->font->drawString(
            &ctx.screen,
            static_cast<uint16_t>(
                std::max<int32_t>(x, 0)
            ),
            static_cast<uint16_t>(
                std::max<int32_t>(y, 0)
            ),
            text_color,
            text
        );
    }

    inline void page(
        Render::DrawContext& ctx,
        void*
    ) {
        ctx.screen.getDriver()->drawFilledRectCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            color::BG_DARK,
            color::BG_DARK
        );
    }

    inline void panel(
        Render::DrawContext& ctx,
        void*
    ) {
        ctx.screen.getDriver()->drawFilledRectCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            color::BG_PANEL,
            color::ACCENT
        );
    }

    inline void statusBar(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        ctx.screen.getDriver()->drawFilledRectCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            color::STATUS_BAR_BG,
            color::STATUS_BAR_BG
        );

        drawTextContent(
            ctx,
            static_cast<TextData*>(extra_data)
        );
    }

    inline void button(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        const bool focused =
            isFocused(ctx);

        ctx.screen.getDriver()->drawFilledRectCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            focused
                ? color::BUTTON_HOVER
                : color::BUTTON_FILL,
            focused
                ? color::FOCUS_BORDER
                : color::ACCENT
        );

        drawTextContent(
            ctx,
            static_cast<TextData*>(extra_data)
        );
    }

    inline void textBox(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        const bool focused =
            isFocused(ctx);

        ctx.screen.getDriver()->drawFilledRectCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            color::BG_PANEL,
            focused
                ? color::FOCUS_BORDER
                : color::ACCENT
        );

        drawTextContent(
            ctx,
            static_cast<TextData*>(extra_data)
        );
    }

    inline void listBox(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        textBox(
            ctx,
            extra_data
        );

        const int32_t marker_x =
            ctx.clip.x2 - 8;

        const int32_t marker_y1 =
            ctx.clip.y1 + 4;

        const int32_t marker_y2 =
            ctx.clip.y2 - 4;

        if (marker_y2 <= marker_y1) {
            return;
        }

        ctx.screen.getDriver()->drawLineCmd(
            static_cast<uint16_t>(marker_x),
            static_cast<uint16_t>(marker_y1),
            static_cast<uint16_t>(marker_x),
            static_cast<uint16_t>(marker_y2),
            color::LIST_MARKER
        );
    }

    inline void outline(
        Render::DrawContext& ctx,
        void*
    ) {
        const uint32_t border =
            isFocused(ctx)
                ? color::FOCUS_BORDER
                : color::ACCENT;

        ctx.screen.getDriver()->drawLineCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y1),
            border
        );

        ctx.screen.getDriver()->drawLineCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y2),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            border
        );

        ctx.screen.getDriver()->drawLineCmd(
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x1),
            static_cast<uint16_t>(ctx.clip.y2),
            border
        );

        ctx.screen.getDriver()->drawLineCmd(
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y1),
            static_cast<uint16_t>(ctx.clip.x2),
            static_cast<uint16_t>(ctx.clip.y2),
            border
        );
    }

}

#endif // APSISUI2_UITHEMEAPSISUI2_H