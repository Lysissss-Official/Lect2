//
// Created by archeart on 2026/7/12.
//

#ifndef APSISUI2_UITHEMEAPSISUI2_H
#define APSISUI2_UITHEMEAPSISUI2_H

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
    }

    struct TextData {
        const char16_t* text = nullptr;
        ext::FontBase* font = nullptr;
    };

    inline void panel(
        Render::DrawContext& ctx,
        void*
    ) {
        ctx.transor.drawFilledRect(
            ctx.clip.x1,
            ctx.clip.y1,
            ctx.clip.x2,
            ctx.clip.y2,
            color::BG_PANEL,
            color::ACCENT
        );
    }

    inline void button(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        auto* element =
            dynamic_cast<strc::Element*>(&ctx.target);

        const bool focused =
            element && element->focused;

        ctx.transor.drawFilledRect(
            ctx.clip.x1,
            ctx.clip.y1,
            ctx.clip.x2,
            ctx.clip.y2,
            focused
                ? color::BUTTON_HOVER
                : color::BUTTON_FILL,
            focused
                ? color::FOCUS_BORDER
                : color::ACCENT
        );

        auto* text =
            static_cast<TextData*>(extra_data);

        if (!text || !text->text || !text->font) {
            return;
        }

        text->font->drawString(
            &ctx.transor,
            static_cast<uint16_t>(ctx.clip.x1 + 6),
            static_cast<uint16_t>(ctx.clip.y1 + 4),
            focused
                ? 0xFFFFFF
                : color::TEXT_PRIMARY,
            text->text
        );
    }

    inline void textBox(
        Render::DrawContext& ctx,
        void* extra_data
    ) {
        auto* element =
            dynamic_cast<strc::Element*>(&ctx.target);

        const bool focused =
            element && element->focused;

        ctx.transor.drawFilledRect(
            ctx.clip.x1,
            ctx.clip.y1,
            ctx.clip.x2,
            ctx.clip.y2,
            color::BG_PANEL,
            focused
                ? color::FOCUS_BORDER
                : color::ACCENT
        );

        auto* text =
            static_cast<TextData*>(extra_data);

        if (!text || !text->text || !text->font) {
            return;
        }

        text->font->drawString(
            &ctx.transor,
            static_cast<uint16_t>(ctx.clip.x1 + 6),
            static_cast<uint16_t>(ctx.clip.y1 + 4),
            focused
                ? 0xFFFFFF
                : color::TEXT_PRIMARY,
            text->text
        );
    }

    inline void outline(
        Render::DrawContext& ctx,
        void*
    ) {
        const uint32_t border =
            dynamic_cast<strc::Element*>(&ctx.target) &&
            static_cast<strc::Element&>(ctx.target).focused
                ? color::FOCUS_BORDER
                : color::ACCENT;

        ctx.transor.drawLineCmd(
            ctx.clip.x1, ctx.clip.y1,
            ctx.clip.x2, ctx.clip.y1,
            border
        );

        ctx.transor.drawLineCmd(
            ctx.clip.x1, ctx.clip.y2,
            ctx.clip.x2, ctx.clip.y2,
            border
        );

        ctx.transor.drawLineCmd(
            ctx.clip.x1, ctx.clip.y1,
            ctx.clip.x1, ctx.clip.y2,
            border
        );

        ctx.transor.drawLineCmd(
            ctx.clip.x2, ctx.clip.y1,
            ctx.clip.x2, ctx.clip.y2,
            border
        );
    }

}

#endif