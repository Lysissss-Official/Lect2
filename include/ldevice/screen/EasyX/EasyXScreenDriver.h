//
// Created by archeart on 2026/7/15.
//

#ifndef APSISUI2_EASYXSCREENDRIVER_H
#define APSISUI2_EASYXSCREENDRIVER_H

#include <cstdint>

#include "easyx.h"
#include "graphics.h"

#include "ldevice/screen/DevScreenDriver.h"

namespace ldevice::driver {

    class EasyXScreenDriver final : public ScreenDriver {
    public:
        struct Config {
            uint16_t width = 1200;
            uint16_t height = 480;

            const wchar_t* title =
                L"LectOS 2 | ApsisUI II";

            uint32_t clear_color = 0x000000;

            // EasyX 双缓冲（单片机狠狠羡慕）
            bool use_batch_draw = true;
        };

        explicit EasyXScreenDriver(
            const Config& config
        );

        ~EasyXScreenDriver() override;

        EasyXScreenDriver(
            const EasyXScreenDriver&
        ) = delete;

        EasyXScreenDriver& operator=(
            const EasyXScreenDriver&
        ) = delete;

        bool initScreenCmd() override;
        bool closeScreenCmd() override;
        bool refreshScreenCmd() override;
        bool ClearScreenCmd() override;

        bool drawPixelCmd(
            uint16_t x,
            uint16_t y,
            uint32_t color
        ) override;

        bool drawFontCmd(
            uint16_t x,
            uint16_t y,
            uint32_t color,
            uint16_t font_type,
            uint16_t unicode
        ) override;

        bool drawLineCmd(
            uint16_t x1,
            uint16_t y1,
            uint16_t x2,
            uint16_t y2,
            uint32_t color
        ) override;

        bool drawFilledRectCmd(
            uint16_t x1,
            uint16_t y1,
            uint16_t x2,
            uint16_t y2,
            uint32_t fill_color,
            uint32_t border_color
        ) override;

        [[nodiscard]] bool initialized() const {
            return initialized_;
        }

    private:
        Config config_;
        bool initialized_ = false;

        static COLORREF toEasyXColor(
            uint32_t color
        );

        static int fontHeightFromType(
            uint16_t font_type
        );
    };

} // namespace ldevice::driver

#endif //APSISUI2_EASYXSCREENDRIVER_H
