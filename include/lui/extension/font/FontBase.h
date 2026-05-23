//
// Created by archeart on 2026/5/23.
//
// 统一位图字体基类 — 存储、查找、基于Transor的绘制。
// 属于 lui::ext::font；不直接接触平台驱动。
//

#ifndef APSISUI2_FONTBASE_H
#define APSISUI2_FONTBASE_H

#include <vector>
#include <algorithm>
#include <cstdint>
#include <string>

#include "../../base/UITransor.h"

namespace lui {
    namespace ext {
        namespace font {

            enum FontType {
                FONT_UNDEFINED = 0,
                FONT_SYSTEM    = 1,
                FONT_USER      = 2
            };

            class FontBase {
            public:
                struct FontInfo {
                    uint16_t unicode;
                    uint16_t width;
                    uint16_t height;
                    std::vector<uint8_t> data;
                };

            protected:
                FontType type = FONT_UNDEFINED;
                std::vector<FontInfo> info;

                static bool cmp(const FontInfo& a, const FontInfo& b) {
                    return a.unicode < b.unicode;
                }

            public:
                FontBase(FontType ty, const std::vector<FontInfo> in)
                    : type(ty), info(in) {}

                FontType getType() const { return type; }

                uint16_t findUni(uint16_t uni) const {
                    auto it = std::lower_bound(info.begin(), info.end(),
                        FontInfo{uni, 0, 0, {}}, cmp);
                    if (it != info.end() && it->unicode == uni)
                        return static_cast<uint16_t>(std::distance(info.begin(), it));
                    return static_cast<uint16_t>(-1);
                }

                FontInfo* findUniPtr(uint16_t uni) {
                    auto it = std::lower_bound(info.begin(), info.end(),
                        FontInfo{uni, 0, 0, {}}, cmp);
                    if (it != info.end() && it->unicode == uni)
                        return &(*it);
                    return nullptr;
                }

                FontInfo getInfo(uint16_t no) const { return info[no]; }

                // --- 基于 Transor 的绘制接口 ---

                void drawChar(lui::transor::TranslatorService* ts,
                              uint16_t x, uint16_t y, uint32_t color,
                              uint16_t uni) const {
                    if (!ts) return;
                    auto it = std::lower_bound(info.begin(), info.end(),
                        FontInfo{uni, 0, 0, {}}, cmp);
                    if (it == info.end() || it->unicode != uni) return;
                    for (uint16_t cnt = 0; cnt < it->height * it->width; ++cnt) {
                        if ((it->data[cnt / 8] >> (7 - cnt % 8)) & 1) {
                            ts->drawPixelCmd(x + cnt % it->width,
                                             y + cnt / it->width, color);
                        }
                    }
                }

                void drawString(lui::transor::TranslatorService* ts,
                                uint16_t x, uint16_t y, uint32_t color,
                                const std::u16string& unistr) const {
                    if (!ts) return;
                    uint16_t deltax = 0;
                    for (char16_t ch : unistr) {
                        auto it = std::lower_bound(info.begin(), info.end(),
                            FontInfo{static_cast<uint16_t>(ch), 0, 0, {}}, cmp);
                        if (it == info.end() || it->unicode != ch) continue;
                        for (uint16_t cnt = 0; cnt < it->height * it->width; ++cnt) {
                            if ((it->data[cnt / 8] >> (7 - cnt % 8)) & 1) {
                                ts->drawPixelCmd(deltax + x + cnt % it->width,
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
            };

        } // namespace font
    } // namespace ext
} // namespace lui

#endif //APSISUI2_FONTBASE_H
