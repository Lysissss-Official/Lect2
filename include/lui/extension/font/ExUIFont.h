//
// Created by archeart on 2026/5/24.
//

#ifndef APSISUI2_EXUIFONT_H
#define APSISUI2_EXUIFONT_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>
#include <utility>
#include <cstddef>

#include "lcore/IDGenerator.h"

namespace lui::ext::font {

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

        std::string name_;
        uint16_t size_;

        std::string family_;
        std::string style_;

        std::vector<Glyph> glyphs_;
        PixelFormat pixel_format_;

        // 每行占用的字节数
        [[nodiscard]] uint16_t rowBytes(uint16_t w) const {
            return static_cast<uint16_t>((w * static_cast<uint16_t>(pixel_format_) + 7) / 8);
        }

    public:
        Font(std::vector<Glyph> glyphs,
            PixelFormat pixel_format,
            std::string name,
            uint16_t size,
            std::string style = "",
            std::string family = "")
            : uni_id_(lcore::IDGenerator<Font>::generate()),
              glyphs_(std::move(glyphs)),
              pixel_format_(pixel_format),
              name_(std::move(name)),
              size_(size),
              family_(std::move(family)),
              style_(std::move(style))
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

        [[nodiscard]] const std::string& getName() const {
            return name_;
        }

        [[nodiscard]] const std::string& getFamily() const {
            return family_;
        }

        [[nodiscard]] const std::string& getStyle() const {
            return style_;
        }

        [[nodiscard]] uint16_t getSize() const {
            return size_;
        }

        [[nodiscard]] PixelFormat getPixelFormat() const {
            return pixel_format_;
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

    class TTFFont{

    };
}

#endif //APSISUI2_EXUIFONT_H
