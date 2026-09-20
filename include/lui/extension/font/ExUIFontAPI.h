//
// Created by Admin on 2026/9/20.
//

#ifndef APSISUI2_EXUIFONTAPI_H
#define APSISUI2_EXUIFONTAPI_H

#include "lui/base/UITheme.h"
#include "lui/extension/font/ExUIFont.h"

// TODO: 更改旧 API 以适配新结构规范

namespace lui::ext {
    void drawChar(ldevice::Screen* screen,
    uint16_t x, uint16_t y, uint32_t color,
    uint16_t uni) const {
        if (!screen) return;
        auto it = std::lower_bound(info.begin(), info.end(),
        FontInfo{uni, 0, 0, {}}, cmp);
        if (it == info.end() || it->unicode != uni) return;
        for (uint16_t cnt = 0; cnt < it->height * it->width; ++cnt) {
            if ((it->data[cnt / 8] >> (7 - cnt % 8)) & 1) {
                screen->getDriver()->drawPixelCmd(x + cnt % it->width,
                                             y + cnt / it->width, color);
            }
        }
    }

    void drawString(ldevice::Screen* screen,
    uint16_t x, uint16_t y, uint32_t color,
    const std::u16string& unistr) const {
        if (!screen) return;
        uint16_t deltax = 0;
        for (char16_t ch : unistr) {
            auto it = std::lower_bound(info.begin(), info.end(),
            FontInfo{static_cast<uint16_t>(ch), 0, 0, {}}, cmp);
            if (it == info.end() || it->unicode != ch) continue;
            for (uint16_t cnt = 0; cnt < it->height * it->width; ++cnt) {
                if ((it->data[cnt / 8] >> (7 - cnt % 8)) & 1) {
                    screen->getDriver()->drawPixelCmd(deltax + x + cnt % it->width,
                    y + cnt / it->width, color);
                }
            }
            deltax += it->width;
        }
    }

    uint16_t getStringWidth(const std::u16string& unistr) const {
        uint16_t w = 0;
        for (char16_t ch : unistr) {
            auto it = std::lower_bound(info.begin(), info.end(),
            FontInfo{static_cast<uint16_t>(ch), 0, 0, {}}, cmp);
            if (it != info.end() && it->unicode == ch)
                w += it->width;
        }
        return w;
    }
}

#endif //APSISUI2_EXUIFONTAPI_H
