//
// Created by archeart on 2026/7/15.
//

#ifndef APSISUI2_DEVSCREENDRIVER_H
#define APSISUI2_DEVSCREENDRIVER_H

#include <cstdint>
#include <stdexcept>

namespace ldevice {
    class ScreenDriver {
    public:
        virtual ~ScreenDriver() = default;

        virtual bool initScreenCmd() {return false;}
        virtual bool closeScreenCmd() {return false;}
        virtual bool refreshScreenCmd() {return false;}
        virtual bool ClearScreenCmd() {return false;}

        virtual bool drawPixelCmd(uint16_t x, uint16_t y, uint32_t color) {return false;}
        virtual bool drawFontCmd(uint16_t x, uint16_t y, uint32_t color, uint16_t font_typ, uint16_t unicode) {return false;}

        virtual bool drawLineCmd(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color) {return false;}
        virtual bool drawFilledRectCmd(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                    uint32_t fill_color, uint32_t border_color) {return false;}
    };
}

#endif //APSISUI2_DEVSCREENDRIVER_H
