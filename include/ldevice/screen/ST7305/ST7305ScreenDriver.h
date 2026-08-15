//
// Created by archeart on 2026/7/15.
//

#ifndef APSISUI2_ST7305SCREENDRIVERV2_H
#define APSISUI2_ST7305SCREENDRIVERV2_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "ldevice/screen/DevScreenDriver.h"

namespace ldevice::driver {

class ST7305ScreenDriver final : public ScreenDriver {
public:
    struct Config {
        spi_host_device_t spi_host = SPI2_HOST;

        gpio_num_t pin_mosi = GPIO_NUM_NC;
        gpio_num_t pin_sclk = GPIO_NUM_NC;
        gpio_num_t pin_cs   = GPIO_NUM_NC;
        gpio_num_t pin_dc   = GPIO_NUM_NC;
        gpio_num_t pin_rst  = GPIO_NUM_NC;

        uint32_t spi_clock_hz = 32'000'000;

        // 共享 SPI 总线时设为 false。
        bool initialize_spi_bus = true;
    };

    explicit ST7305ScreenDriver(const Config& config);
    ~ST7305ScreenDriver() override;

    ST7305ScreenDriver(const ST7305ScreenDriver&) = delete;
    ST7305ScreenDriver& operator=(const ST7305ScreenDriver&) = delete;

    bool initScreenCmd() override;
    bool closeScreenCmd() override;
    bool refreshScreenCmd() override;
    bool ClearScreenCmd() override;

    bool drawPixelCmd(
        uint16_t x,
        uint16_t y,
        uint32_t color
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

    bool sleep();
    bool wake();

    [[nodiscard]] bool initialized() const {
        return initialized_;
    }

private:
    /*
     * 这些不是 Screen 的通用描述，而是当前 210×480 模组的
     * ST7305 RAM 排布参数，因此保留在具体驱动内部。
     */
    static constexpr uint16_t PANEL_WIDTH  = 384;
    static constexpr uint16_t PANEL_HEIGHT = 168;

    static constexpr uint16_t RAM_ROWS =
        PANEL_HEIGHT / 2;

    static constexpr uint16_t ROW_BYTES =
        PANEL_WIDTH + 2;

    static constexpr std::size_t FRAMEBUFFER_SIZE =
        static_cast<std::size_t>(RAM_ROWS) *
        ROW_BYTES;

    static constexpr uint8_t COLUMN_START = 0x04;
    static constexpr uint8_t COLUMN_END   = 0x38;
    static constexpr uint8_t ROW_START    = 0x00;
    static constexpr uint8_t ROW_END      =
        static_cast<uint8_t>(RAM_ROWS - 1);

    Config config_;

    spi_device_handle_t spi_device_ = nullptr;

    bool bus_owned_ = false;
    bool initialized_ = false;

    std::array<uint8_t, FRAMEBUFFER_SIZE>
        framebuffer_{};

    bool configurePins();
    bool initBus();
    void closeBus();

    void hardwareReset();

    bool initializePanel();

    bool transmit(
        bool data_mode,
        const void* data,
        std::size_t size,
        uint32_t flags = 0
    );

    bool sendCommand(
        uint8_t command,
        const uint8_t* data = nullptr,
        std::size_t data_size = 0
    );

    static uint8_t colorToPanelBits(
        uint32_t color
    );

    static uint8_t packedPairForColor(
        uint32_t color
    );

    uint8_t& pixelPairByte(
        uint16_t x,
        uint16_t y
    );
};

} // namespace ldevice::driver

#endif //APSISUI2_ST7305SCREENDRIVERV2_H
