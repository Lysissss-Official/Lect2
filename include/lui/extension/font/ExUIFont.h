//
// Created by archeart on 2026/5/24.
//

#ifndef APSISUI2_EXUIFONT_H
#define APSISUI2_EXUIFONT_H

#include <vector>
#include <cstdint>
#include <algorithm>

#include "lcore/IDGenerator.h"

namespace lui::ext {
    class Font {
    public:
        enum class PixelFormat : uint8_t {
            Mono1 = 1,
            Gray2 = 2,
            Gray4 = 4,
            Gray8 = 8,
            RGB565 = 16,
            RGBA8888 = 32
        };

        struct Glyph {
            // Unicode编码
            uint32_t unicode = 0;

            // 实际位图宽高
            uint16_t width = 0;
            uint16_t height = 0;

            // 排版后光标前进距离
            uint16_t advance_x = 0;

            // 按行连续存储的位图数据
            std::vector<uint8_t> bitmap;
        };

    private:
        uint32_t uni_id_;
        std::vector<Glyph> glyphs_;
        PixelFormat pixelFormat_;

        // 每行占用的字节数
        [[nodiscard]] uint16_t rowBytes(uint16_t w) const {
            return static_cast<uint16_t>((w * static_cast<uint16_t>(pixelFormat_) + 7) / 8);
        }

    public:
        Font(std::vector<Glyph> glyphs, PixelFormat pixelFormat)
            : uni_id_(lcore::IDGenerator<Font>::generate()),
              glyphs_(std::move(glyphs)),
              pixelFormat_(pixelFormat)
        {
            std::sort(
                glyphs_.begin(),
                glyphs_.end(),
                [](const Glyph& a, const Glyph& b) {
                    return a.unicode < b.unicode;
                }
            );
        }

        ~Font() {
            lcore::IDGenerator<Font>::release(uni_id_);
        }

        [[nodiscard]] uint32_t getID() const {
            return uni_id_;
        }

        [[nodiscard]] std::size_t getGlyphCount() const {
            return glyphs_.size();
        }

        [[nodiscard]] const Glyph* getGlyph(uint32_t unicode) const {
            auto it = std::lower_bound(
                glyphs_.begin(),
                glyphs_.end(),
                unicode,
                [](const Glyph& glyph, uint32_t value) {
                    return glyph.unicode < value;
                }
            );

            if (it == glyphs_.end() || it->unicode != unicode)
                return nullptr;

            return &(*it);
        }

    };
}

#endif //APSISUI2_EXUIFONT_H
