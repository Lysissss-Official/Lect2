//
// Created by archeart on 2026/7/15.
//

#include "ST7305ScreenDriver.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ldevice::driver {

namespace {

void delayMs(uint32_t milliseconds) {
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

} // namespace

ST7305ScreenDriver::ST7305ScreenDriver(
    const Config& config
) : config_(config) {
    framebuffer_.fill(0x00);
}

ST7305ScreenDriver::~ST7305ScreenDriver() {
    closeScreenCmd();
}

bool ST7305ScreenDriver::initScreenCmd() {
    if (initialized_) {
        return true;
    }

    if (!configurePins()) {
        return false;
    }

    if (!initBus()) {
        return false;
    }

    hardwareReset();

    if (!initializePanel()) {
        closeBus();
        return false;
    }

    if (!ClearScreenCmd()) {
        closeBus();
        return false;
    }

    if (!refreshScreenCmd()) {
        closeBus();
        return false;
    }

    if (!sendCommand(0x29)) { // DISPON
        closeBus();
        return false;
    }

    initialized_ = true;
    return true;
}

bool ST7305ScreenDriver::closeScreenCmd() {
    if (!spi_device_) {
        initialized_ = false;
        return true;
    }

    bool ok = true;

    if (initialized_) {
        ok = refreshScreenCmd() && ok;
        delayMs(20);

        ok = sendCommand(0x28) && ok; // DISPOFF
        ok = sendCommand(0x10) && ok; // SLPIN
        delayMs(120);
    }

    initialized_ = false;
    closeBus();

    return ok;
}

bool ST7305ScreenDriver::configurePins() {
    if (
        config_.pin_mosi == GPIO_NUM_NC ||
        config_.pin_sclk == GPIO_NUM_NC ||
        config_.pin_cs   == GPIO_NUM_NC ||
        config_.pin_dc   == GPIO_NUM_NC ||
        config_.pin_rst  == GPIO_NUM_NC
    ) {
        return false;
    }

    gpio_config_t config{};

    config.pin_bit_mask =
        (1ULL << config_.pin_dc) |
        (1ULL << config_.pin_rst);

    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&config) != ESP_OK) {
        return false;
    }

    gpio_set_level(config_.pin_dc, 0);
    gpio_set_level(config_.pin_rst, 1);

    return true;
}

bool ST7305ScreenDriver::initBus() {
    if (spi_device_) {
        return true;
    }

    if (config_.initialize_spi_bus) {
        spi_bus_config_t bus_config{};

        bus_config.mosi_io_num = config_.pin_mosi;

        bus_config.miso_io_num = GPIO_NUM_NC;

        bus_config.sclk_io_num = config_.pin_sclk;

        bus_config.quadwp_io_num = GPIO_NUM_NC;

        bus_config.quadhd_io_num = GPIO_NUM_NC;

        bus_config.max_transfer_sz = 4092;

        const esp_err_t result =
            spi_bus_initialize(config_.spi_host, &bus_config, SPI_DMA_CH_AUTO);

        if (result != ESP_OK) {
            return false;
        }

        bus_owned_ = true;
    }

    spi_device_interface_config_t device_config{};

    device_config.mode = 0;

    device_config.clock_speed_hz = static_cast<int>(config_.spi_clock_hz);

    device_config.spics_io_num = config_.pin_cs;

    device_config.queue_size = 1;

    const esp_err_t result =
        spi_bus_add_device(config_.spi_host, &device_config, &spi_device_);

    if (result != ESP_OK) {
        if (bus_owned_) {
            spi_bus_free(config_.spi_host);
            bus_owned_ = false;
        }

        spi_device_ = nullptr;
        return false;
    }

    return true;
}

void ST7305ScreenDriver::closeBus() {
    if (spi_device_) {
        spi_bus_remove_device(spi_device_);
        spi_device_ = nullptr;
    }

    if (bus_owned_) {
        spi_bus_free(config_.spi_host);
        bus_owned_ = false;
    }
}

void ST7305ScreenDriver::hardwareReset() {
    gpio_set_level(config_.pin_rst, 0);
    delayMs(50);

    gpio_set_level(config_.pin_rst, 1);
    delayMs(20);
}

bool ST7305ScreenDriver::transmit(
    bool data_mode,
    const void* data,
    std::size_t size,
    uint32_t flags
) {
    if (!spi_device_ || !data || size == 0) {
        return false;
    }

    gpio_set_level(
        config_.pin_dc,
        data_mode ? 1 : 0
    );

    spi_transaction_t transaction{};

    transaction.length = size * 8;
    transaction.tx_buffer = data;
    transaction.flags = flags;

    return spi_device_polling_transmit(
        spi_device_,
        &transaction
    ) == ESP_OK;
}

bool ST7305ScreenDriver::sendCommand(
    uint8_t command,
    const uint8_t* data,
    std::size_t data_size
) {
    if (!spi_device_) {
        return false;
    }

    if (
        spi_device_acquire_bus(
            spi_device_,
            portMAX_DELAY
        ) != ESP_OK
    ) {
        return false;
    }

    const uint32_t command_flags =
        data_size > 0
            ? SPI_TRANS_CS_KEEP_ACTIVE
            : 0;

    const bool command_ok = transmit(
        false,
        &command,
        sizeof(command),
        command_flags
    );

    bool data_ok = true;

    if (command_ok && data && data_size > 0) {
        data_ok = transmit(
            true,
            data,
            data_size
        );
    }

    spi_device_release_bus(spi_device_);

    return command_ok && data_ok;
}

bool ST7305ScreenDriver::initializePanel() {
    static constexpr uint8_t D6[] = {
        0x17, 0x02
    };

    static constexpr uint8_t D1[] = {
        0x01
    };

    static constexpr uint8_t C0[] = {
        0x0E, 0x0A
    };

    static constexpr uint8_t C1[] = {
        0x41, 0x41, 0x41, 0x41
    };

    static constexpr uint8_t C2[] = {
        0x32, 0x32, 0x32, 0x32
    };

    static constexpr uint8_t C4[] = {
        0x46, 0x46, 0x46, 0x46
    };

    static constexpr uint8_t C5[] = {
        0x46, 0x46, 0x46, 0x46
    };

    static constexpr uint8_t B2[] = {
        0x12
    };

    static constexpr uint8_t B3[] = {
        0xE5, 0xF6, 0x05, 0x46, 0x77,
        0x77, 0x77, 0x77, 0x76, 0x45
    };

    static constexpr uint8_t B4[] = {
        0x05, 0x46, 0x77, 0x77,
        0x77, 0x77, 0x76, 0x45
    };

    static constexpr uint8_t B7[] = {
        0x13
    };

    static constexpr uint8_t B0[] = {
        0x78
    };

    if (!sendCommand(0xD6, D6, sizeof(D6))) return false;
    if (!sendCommand(0xD1, D1, sizeof(D1))) return false;
    if (!sendCommand(0xC0, C0, sizeof(C0))) return false;
    if (!sendCommand(0xC1, C1, sizeof(C1))) return false;
    if (!sendCommand(0xC2, C2, sizeof(C2))) return false;
    if (!sendCommand(0xC4, C4, sizeof(C4))) return false;
    if (!sendCommand(0xC5, C5, sizeof(C5))) return false;
    if (!sendCommand(0xB2, B2, sizeof(B2))) return false;
    if (!sendCommand(0xB3, B3, sizeof(B3))) return false;
    if (!sendCommand(0xB4, B4, sizeof(B4))) return false;
    if (!sendCommand(0xB7, B7, sizeof(B7))) return false;
    if (!sendCommand(0xB0, B0, sizeof(B0))) return false;

    if (!sendCommand(0x11)) {
        return false;
    }

    delayMs(120);

    static constexpr uint8_t D8[] = {
        0x80
    };

    static constexpr uint8_t C9[] = {
        0x00
    };

    static constexpr uint8_t MADCTL[] = {
        0x48
    };

    static constexpr uint8_t DTFORM[] = {
        0x32
    };

    static constexpr uint8_t B9[] = {
        0x00
    };

    static constexpr uint8_t B8[] = {
        0x0A
    };

    static constexpr uint8_t TEON[] = {
        0x00
    };

    static constexpr uint8_t D0[] = {
        0xFF
    };

    if (!sendCommand(0xD8, D8, sizeof(D8))) return false;
    if (!sendCommand(0xC9, C9, sizeof(C9))) return false;
    if (!sendCommand(0x36, MADCTL, sizeof(MADCTL))) return false;
    if (!sendCommand(0x3A, DTFORM, sizeof(DTFORM))) return false;
    if (!sendCommand(0xB9, B9, sizeof(B9))) return false;
    if (!sendCommand(0xB8, B8, sizeof(B8))) return false;
    if (!sendCommand(0x35, TEON, sizeof(TEON))) return false;
    if (!sendCommand(0xD0, D0, sizeof(D0))) return false;
    if (!sendCommand(0x38)) return false;

    return true;
}

uint8_t ST7305ScreenDriver::colorToPanelBits(
    uint32_t color
) {
    const uint16_t rgb565 = static_cast<uint16_t>(color);

    const uint8_t red = (rgb565 & 0x8000U) ? 1U : 0U;

    const uint8_t green = (rgb565 & 0x0400U) ? 1U : 0U;

    const uint8_t blue = (rgb565 & 0x0010U) ? 1U : 0U;

    return static_cast<uint8_t>(
        (blue << 2U) | (green << 1U) | red
    );
}

uint8_t ST7305ScreenDriver::packedPairForColor(
    uint32_t color
) {
    const uint8_t bits = colorToPanelBits(color);

    const uint8_t active = static_cast<uint8_t>((bits << 2U) | (bits << 5U));

    return static_cast<uint8_t>(
        (~active) & 0xFCU
    );
}

uint8_t& ST7305ScreenDriver::pixelPairByte(
    uint16_t x,
    uint16_t y
) {
    const std::size_t row = static_cast<std::size_t>(y >> 1U);
    const std::size_t column = static_cast<std::size_t>(x) + 2U;

    return framebuffer_[
        row * ROW_BYTES + column
    ];
}

bool ST7305ScreenDriver::drawPixelCmd(
    uint16_t x,
    uint16_t y,
    uint32_t color
) {
    if (x >= PANEL_WIDTH || y >= PANEL_HEIGHT) {
        return false;
    }

    uint8_t& target = pixelPairByte(x, y);

    const uint8_t bits = colorToPanelBits(color);

    if ((y & 1U) == 0U) {
        constexpr uint8_t mask = 0x1CU;

        target = static_cast<uint8_t>(
            (target | mask) & static_cast<uint8_t>(~(bits << 2U))
        );
    }
    else {
        constexpr uint8_t mask = 0xE0U;

        target = static_cast<uint8_t>(
            (target | mask) & static_cast<uint8_t>(~(bits << 5U))
        );
    }

    return true;
}

bool ST7305ScreenDriver::ClearScreenCmd() {
    const uint8_t packed = packedPairForColor(0xFFFFU);

    for (uint16_t row = 0; row < RAM_ROWS; ++row) {
        uint8_t* row_data = framebuffer_.data() + static_cast<std::size_t>(row) * ROW_BYTES;

        row_data[0] = 0x00;
        row_data[1] = 0x00;

        std::memset(row_data + 2, packed, PANEL_WIDTH);
    }

    return true;
}

bool ST7305ScreenDriver::drawLineCmd(
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2,
    uint32_t color
) {
    int32_t x = static_cast<int32_t>(x1);
    int32_t y = static_cast<int32_t>(y1);

    const int32_t target_x = static_cast<int32_t>(x2);
    const int32_t target_y = static_cast<int32_t>(y2);

    const int32_t dx = std::abs(target_x - x);
    const int32_t sx = x < target_x ? 1 : -1;

    const int32_t dy = -std::abs(target_y - y);
    const int32_t sy = y < target_y ? 1 : -1;

    int32_t error = dx + dy;

    while (true) {
        if (x >= 0 && y >= 0 && x < PANEL_WIDTH && y < PANEL_HEIGHT) {
            drawPixelCmd(
                static_cast<uint16_t>(x),
                static_cast<uint16_t>(y),
                color
            );
        }

        if (x == target_x && y == target_y) {
            break;
        }

        const int32_t error2 =
            error * 2;

        if (error2 >= dy) {
            error += dy;
            x += sx;
        }

        if (error2 <= dx) {
            error += dx;
            y += sy;
        }
    }

    return true;
}

bool ST7305ScreenDriver::drawFilledRectCmd(
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2,
    uint32_t fill_color,
    uint32_t border_color
) {
    uint16_t left = std::min(x1, x2);

    uint16_t right = std::max(x1, x2);

    uint16_t top = std::min(y1, y2);

    uint16_t bottom = std::max(y1, y2);

    if (left >= PANEL_WIDTH || top >= PANEL_HEIGHT) {
        return false;
    }

    right = std::min<uint16_t>(right, PANEL_WIDTH - 1);

    bottom = std::min<uint16_t>(bottom, PANEL_HEIGHT - 1);

    for (uint16_t y = top; y <= bottom; ++y) {
        for (uint16_t x = left; x <= right; ++x) {
            drawPixelCmd(
                x,
                y,
                fill_color
            );
        }
    }

    drawLineCmd(
        left,
        top,
        right,
        top,
        border_color
    );

    drawLineCmd(
        left,
        bottom,
        right,
        bottom,
        border_color
    );

    drawLineCmd(
        left,
        top,
        left,
        bottom,
        border_color
    );

    drawLineCmd(
        right,
        top,
        right,
        bottom,
        border_color
    );

    return true;
}

bool ST7305ScreenDriver::refreshScreenCmd() {
    if (!spi_device_) {
        return false;
    }

    static constexpr uint8_t column_range[] = {
        COLUMN_START,
        COLUMN_END
    };

    static constexpr uint8_t row_range[] = {
        ROW_START,
        ROW_END
    };

    if (!sendCommand(
        0x2A,
        column_range,
        sizeof(column_range)
    )) {
        return false;
    }

    if (!sendCommand(
        0x2B,
        row_range,
        sizeof(row_range)
    )) {
        return false;
    }

    if (
        spi_device_acquire_bus(
            spi_device_,
            portMAX_DELAY
        ) != ESP_OK
    ) {
        return false;
    }

    constexpr std::size_t CHUNK_SIZE = 4092;

    const uint8_t ram_write = 0x2C;

    // RAMWR 后面还有 framebuffer 数据，因此保持 CS。
    bool ok = transmit(
        false,
        &ram_write,
        sizeof(ram_write),
        SPI_TRANS_CS_KEEP_ACTIVE
    );

    std::size_t offset = 0;

    while (ok && offset < framebuffer_.size()) {
        const std::size_t remaining =
            framebuffer_.size() - offset;

        const std::size_t chunk_size =
            std::min(
                CHUNK_SIZE,
                remaining
            );

        const bool is_last =
            offset + chunk_size >=
            framebuffer_.size();

        ok = transmit(
            true,
            framebuffer_.data() + offset,
            chunk_size,
            is_last
                ? 0
                : SPI_TRANS_CS_KEEP_ACTIVE
        );

        offset += chunk_size;
    }

    spi_device_release_bus(spi_device_);

    return ok;
}

bool ST7305ScreenDriver::sleep() {
    if (!spi_device_) {
        return false;
    }

    bool ok = true;

    ok = sendCommand(0x28) && ok;
    ok = sendCommand(0x10) && ok;

    delayMs(120);
    return ok;
}

bool ST7305ScreenDriver::wake() {
    if (!spi_device_) {
        return false;
    }

    if (!sendCommand(0x11)) {
        return false;
    }

    delayMs(120);
    return sendCommand(0x29);
}

} // namespace ldevice::driver