//
// Created by archeart on 2026/7/13.
//

#ifndef APSISUI2_ST7305_H
#define APSISUI2_ST7305_H



class ST7305 {
public:
    struct Config {
        spi_host_device_t host = SPI2_HOST;

        gpio_num_t pin_mosi;
        gpio_num_t pin_sclk;
        gpio_num_t pin_cs;
        gpio_num_t pin_dc;
        gpio_num_t pin_rst;

        uint32_t spi_clock_hz = 32'000'000;
    };

    explicit ST7306(const Config& config);

    bool begin();

    void clear(uint16_t color = 0xFFFF);
    void drawPixel(
        int32_t x,
        int32_t y,
        uint16_t rgb565
    );

    bool flush();

    void sleep();
    void wake();

private:
    static constexpr uint16_t WIDTH = 210;
    static constexpr uint16_t HEIGHT = 480;

    Config config_;
    spi_device_handle_t spi_ = nullptr;

    uint8_t framebuffer_[HEIGHT / 2][WIDTH + 2]{};

    bool initBus();
    void reset();

    bool writeCommand(uint8_t command);
    bool writeData(uint8_t data);
    bool writeData(
        const uint8_t* data,
        size_t size
    );

    bool command(
        uint8_t command,
        const uint8_t* data = nullptr,
        size_t size = 0
    );

    void initPanel();
};



#endif //APSISUI2_ST7305_H
