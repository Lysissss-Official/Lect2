//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UITRANSOR_H
#define APSISUI2_UITRANSOR_H

#include <cstdint>
#include <stdexcept>

namespace lui {
    namespace transor {
        class TranslatorService {
        public:
            virtual void drawPixelCmd(uint16_t x, uint16_t y, uint16_t color) {
                throw std::runtime_error("LUI Transor: Attempted to call drawPixelCmd(), but it is not implemented.");
            }
            virtual void drawFontCmd(uint16_t x, uint16_t y, uint16_t color, uint16_t font_typ, uint16_t unicode) {
                throw std::runtime_error("LUI Transor: Attempted to call drawFontCmd(), but it is not implemented.");
            }
            virtual void ClearDeviceCmd() = 0;
            virtual void drawLineCmd(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) = 0;
        };
    }
}

#endif //APSISUI2_UITRANSOR_H
