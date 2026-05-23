//
// Created by archeart on 2026/5/23.
//
// EasyX-based translator implementation.
// Translates LUI drawing commands into EasyX graphics.h calls.
//

#ifndef APSISUI2_UITRANSORLINUX7_H
#define APSISUI2_UITRANSORLINUX7_H

#include "easyx.h"
#include "graphics.h"
#include "../../base/UITransor.h"

namespace lui {
    namespace transor {

        class UITransorLinux7 : public TranslatorService {
        public:
            void drawPixelCmd(uint16_t x, uint16_t y, uint16_t color) override {
                putpixel(static_cast<int>(x), static_cast<int>(y),
                         static_cast<COLORREF>(color));
            }

            void drawFontCmd(uint16_t x, uint16_t y, uint16_t color,
                             uint16_t font_typ, uint16_t unicode) override {
                settextcolor(static_cast<COLORREF>(color));
                // font_typ: 0=small, 1=medium, 2=large
                int height = 12;
                if (font_typ == 1) height = 16;
                if (font_typ == 2) height = 24;
                settextstyle(height, 0, _T("Consolas"));

                TCHAR ch = static_cast<TCHAR>(unicode);
                outtextxy(static_cast<int>(x), static_cast<int>(y), ch);
            }

            void ClearDeviceCmd() override {
                cleardevice();
            }

            void drawLineCmd(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                             uint16_t color) override {
                setlinecolor(static_cast<COLORREF>(color));
                line(static_cast<int>(x1), static_cast<int>(y1),
                     static_cast<int>(x2), static_cast<int>(y2));
            }

            // Utility: draw a filled rectangle with a border
            void drawFilledRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                uint16_t fill_color, uint16_t border_color) {
                setfillcolor(static_cast<COLORREF>(fill_color));
                setlinecolor(static_cast<COLORREF>(border_color));
                fillrectangle(static_cast<int>(x1), static_cast<int>(y1),
                              static_cast<int>(x2), static_cast<int>(y2));
            }

            // Utility: output a string at pixel position
            void drawString(uint16_t x, uint16_t y, const char* str,
                            uint16_t color = WHITE, int font_h = 16) {
                settextcolor(static_cast<COLORREF>(color));
                settextstyle(font_h, 0, _T("Consolas"));
                outtextxy(static_cast<int>(x), static_cast<int>(y), str);
            }
        };
    }
}

#endif //APSISUI2_UITRANSORLINUX7_H
