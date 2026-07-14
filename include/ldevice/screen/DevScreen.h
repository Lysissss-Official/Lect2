//
// Created by archeart on 2026/7/14.
//

#ifndef APSISUI2_DEVSCREEN_H
#define APSISUI2_DEVSCREEN_H

#include <cstdint>
#include "lcore/IDGenerator.h"  // IDGenerator

namespace ldevice {

    enum class ColorMode : uint8_t {
        SCREEN_BW   = 0,    // 黑白双色
        SCREEN_GRAY = 1,    // 灰度模式
        SCREEN_RGB565  = 2, // 16位色模式
        SCREEN_RGB666 = 3,  // 18位色模式
        SCREEN_RGB888  = 4  // 24位色模式
    };

    class Screen {
    private:
        uint32_t uni_id;
        uint32_t width;
        uint32_t height;
        ColorMode color_mode;
    public:
        Screen() : width(0), height(0), color_mode(ColorMode::SCREEN_RGB565) {
            uni_id = lcore::IDGenerator<Screen>::generate();
        }
        virtual ~Screen() {
            lcore::IDGenerator<Screen>::release(uni_id);
        }

        void init();
        void close();

        uint32_t getID() const {
            return uni_id;
        }

        uint32_t& getWidth() {return width;}
        const uint32_t& getWidth() const {return width;}

        uint32_t& getHeight() {return height;}
        const uint32_t& getHeight() const {return height;}

        ColorMode& getColorMode() {return color_mode;}
        const ColorMode& getColorMode() const {return color_mode;}

        Screen(const Screen&) = delete;
        Screen& operator=(const Screen&) = delete;
    };
}

#endif //APSISUI2_DEVSCREEN_H
