//
// Created by archeart on 2026/7/14.
//

#ifndef APSISUI2_DEVSCREEN_H
#define APSISUI2_DEVSCREEN_H

#include <cstdint>

#include "DevScreenDriver.h"
#include "lcore/IDGenerator.h"  // IDGenerator

namespace ldevice {

    enum class ScreenColorMode : uint8_t {
        BW   = 0,    // 黑白双色
        GRAY = 1,    // 灰度模式
        RGB565  = 2, // 16位色模式
        RGB666 = 3,  // 18位色模式
        RGB888  = 4  // 24位色模式
    };

    class Screen {
    private:
        uint32_t uni_id;
        uint32_t width;
        uint32_t height;
        uint32_t refresh_rate;
        ScreenColorMode color_mode;
        ScreenDriver* driver_;

    public:
        Screen()
            : width(0),
              height(0),
              refresh_rate(1),
              color_mode(ScreenColorMode::RGB565),
              driver_(nullptr)
        {
            uni_id =
                lcore::IDGenerator<Screen>::generate();
        }

        Screen(
            uint32_t width,
            uint32_t height,
            uint32_t refresh_rate,
            ScreenColorMode color_mode,
            ScreenDriver* driver
        )
            : width(width),
              height(height),
              refresh_rate(refresh_rate),
              color_mode(color_mode),
              driver_(driver)
        {
            uni_id = lcore::IDGenerator<Screen>::generate();
        }

        virtual ~Screen() {
            lcore::IDGenerator<Screen>::release(uni_id);
        }

        void setDriver(ScreenDriver* driver) {
            driver_ = driver;
        }

        uint32_t getID() const {
            return uni_id;
        }

        uint32_t& getWidth() {return width;}
        const uint32_t& getWidth() const {return width;}

        uint32_t& getHeight() {return height;}
        const uint32_t& getHeight() const {return height;}

        uint32_t& getRefreshRate() {return refresh_rate;}
        const uint32_t& getRefreshRate() const {return refresh_rate;}

        ScreenColorMode& getColorMode() {return color_mode;}
        const ScreenColorMode& getColorMode() const {return color_mode;}

        ScreenDriver* getDriver() {return driver_;}
        const ScreenDriver* getDriver() const {return driver_;}

        Screen(const Screen&) = delete;
        Screen& operator=(const Screen&) = delete;
    };
}

#endif //APSISUI2_DEVSCREEN_H
