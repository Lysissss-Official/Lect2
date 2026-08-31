//
// Created by archeart on 2026/8/15.
//

#ifndef APSISUI2_UITHEMECHASSISUI_H
#define APSISUI2_UITHEMECHASSISUI_H

#include "lui/base/UITheme.h"
#include <algorithm>
#include <chrono>
#include <cstdint>

namespace lui::theme {
    class ChassisUI final : public Theme {
    public:
        struct Color {
            static constexpr uint32_t BG_DARK      = 0x0A0A0A;
            static constexpr uint32_t BG_PANEL     = 0x121212;
            static constexpr uint32_t ACCENT       = 0x303000;
            static constexpr uint32_t TEXT_PRIMARY = 0xE0E0E0;
            static constexpr uint32_t TEXT_DIM     = 0x808060;
            static constexpr uint32_t FOCUS_BORDER = 0xCCCC00;
            static constexpr uint32_t BUTTON_FILL  = 0x252500;
            static constexpr uint32_t BUTTON_HOVER = 0x454500;
            static constexpr uint32_t STATUS_BAR_BG = 0x060606;
            static constexpr uint32_t LIST_MARKER  = 0x808060;
        };

        bool drawFuncCall(
            lui::DrawContext& context
        ) override {
            switch (context.target.getParam(strc::ParamIndex::item_typ)) {
                // ...
                default: {
                    const auto* element =
                        dynamic_cast<const strc::Element*>(&context.target);

                    const bool focused =
                        element && element->focused;

                    context.screen.getDriver()->drawFilledRectCmd(
                        static_cast<uint16_t>(context.clip.phys_x1),
                        static_cast<uint16_t>(context.clip.phys_y1),
                        static_cast<uint16_t>(context.clip.phys_x2),
                        static_cast<uint16_t>(context.clip.phys_y2),
                        focused ? 0x000000 : 0xFFFFFF,
                        focused ? 0xFFFFFF : 0x000000
                    );

                    break;
                }
            }
            for (const auto& it : context.target.getOptionalParams()) {
                // ...
            }
            return true;
        }

        float timeFuncCall(
            std::chrono::steady_clock::time_point time_start,
            std::chrono::steady_clock::time_point time_end,
            std::chrono::steady_clock::time_point now
        ) override {
            if (time_end <= time_start) {
                return 1.0f;
            }

            const float progress =
                std::chrono::duration<float>(
                    now - time_start
                ).count()
                /
                std::chrono::duration<float>(
                    time_end - time_start
                ).count();

            const float t = std::clamp(
                progress,
                0.0f,
                1.0f
            );

            // Ease Out Quad
            return 1.0f - (1.0f - t) * (1.0f - t);
        }
    };
}


#endif //APSISUI2_UITHEMECHASSISUI_H
