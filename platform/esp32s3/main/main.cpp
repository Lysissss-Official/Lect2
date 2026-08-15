#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ldevice/screen/ST7306/ST7306ScreenDriver.h"
#include "ldevice/screen/ST7305/ST7305ScreenDriver.h"

namespace {
constexpr char TAG[] = "DualLCD";

void drawTest(
    ldevice::driver::ST7306ScreenDriver& lcd,
    bool reverse
) {
    lcd.ClearScreenCmd();

    // 外框
    lcd.drawLineCmd(
        0, 0,
        209, 0,
        0x000000
    );

    lcd.drawLineCmd(
        209, 0,
        209, 479,
        0x000000
    );

    lcd.drawLineCmd(
        209, 479,
        0, 479,
        0x000000
    );

    lcd.drawLineCmd(
        0, 479,
        0, 0,
        0x000000
    );

    // 对角线
    lcd.drawLineCmd(
        0, 0,
        209, 479,
        0x000000
    );

    lcd.drawLineCmd(
        209, 0,
        0, 479,
        0x000000
    );

    // 区分两块屏
    if (!reverse) {
        lcd.drawFilledRectCmd(
            20, 40,
            185, 130,
            0x000000,
            0x000000
        );
    }
    else {
        lcd.drawFilledRectCmd(
            20, 350,
            185, 440,
            0x000000,
            0x000000
        );
    }

    // 中间点阵
    for (uint16_t y = 180; y < 300; y += 8) {
        for (uint16_t x = 20; x < 190; x += 8) {
            lcd.drawPixelCmd(
                x,
                y,
                ((x + y) / 8) & 1
                    ? 0x000000
                    : 0xFFFFFF
            );
        }
    }

    lcd.refreshScreenCmd();
}
void drawTest2(
    ldevice::driver::ST7305ScreenDriver& lcd,
    bool reverse
) {
    lcd.ClearScreenCmd();

    // 外框
    lcd.drawLineCmd(
        0, 0,
        209, 0,
        0x000000
    );

    lcd.drawLineCmd(
        209, 0,
        209, 479,
        0x000000
    );

    lcd.drawLineCmd(
        209, 479,
        0, 479,
        0x000000
    );

    lcd.drawLineCmd(
        0, 479,
        0, 0,
        0x000000
    );

    // 对角线
    lcd.drawLineCmd(
        0, 0,
        209, 479,
        0x000000
    );

    lcd.drawLineCmd(
        209, 0,
        0, 479,
        0x000000
    );

    // 区分两块屏
    if (!reverse) {
        lcd.drawFilledRectCmd(
            20, 40,
            185, 130,
            0x000000,
            0x000000
        );
    }
    else {
        lcd.drawFilledRectCmd(
            20, 350,
            185, 440,
            0x000000,
            0x000000
        );
    }

    // 中间点阵
    for (uint16_t y = 180; y < 300; y += 8) {
        for (uint16_t x = 20; x < 190; x += 8) {
            lcd.drawPixelCmd(
                x,
                y,
                ((x + y) / 8) & 1
                    ? 0x000000
                    : 0xFFFFFF
            );
        }
    }

    lcd.refreshScreenCmd();
}
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "Starting dual ST7305 test");

    /*
     * 第一块屏：原来的接口。
     * GPIO48 同时连接开发板 RGB LED，
     * 因此刷新时 RGB 灯可能亮起。
     */
    static ldevice::driver::ST7306ScreenDriver lcd_a({
        .spi_host = SPI3_HOST,

        .pin_mosi = GPIO_NUM_39,
        .pin_sclk = GPIO_NUM_38,
        .pin_cs   = GPIO_NUM_45,
        .pin_dc   = GPIO_NUM_48,
        .pin_rst  = GPIO_NUM_47,

        .spi_clock_hz = 10'000'000,
        .initialize_spi_bus = true
    });

    /*
     * 第二块屏：另一套接口。
     * 使用 SPI3_HOST，不能继续使用 SPI2_HOST，
     * 因为它的 MOSI/SCLK 引脚与第一块不同。
     */
    static ldevice::driver::ST7305ScreenDriver lcd_b({
        .spi_host = SPI2_HOST,

        .pin_mosi = GPIO_NUM_6,
        .pin_sclk = GPIO_NUM_7,
        .pin_cs   = GPIO_NUM_15,
        .pin_dc   = GPIO_NUM_16,
        .pin_rst  = GPIO_NUM_8,

        .spi_clock_hz = 10'000'000,
        .initialize_spi_bus = true
    });

    ESP_LOGI(TAG, "Initializing LCD A");

    const bool lcd_a_ok =
        lcd_a.initScreenCmd();

    ESP_LOGI(
        TAG,
        "LCD A: %s",
        lcd_a_ok ? "OK" : "FAILED"
    );

    ESP_LOGI(TAG, "Initializing LCD B");

    const bool lcd_b_ok =
        lcd_b.initScreenCmd();

    ESP_LOGI(
        TAG,
        "LCD B: %s",
        lcd_b_ok ? "OK" : "FAILED"
    );

    if (lcd_a_ok) {
        drawTest(
            lcd_a,
            false
        );
    }

    if (lcd_b_ok) {
        drawTest2(
            lcd_b,
            true
        );
    }

    uint32_t counter = 0;

    while (true) {
        /*
         * 每两秒重新刷新两块屏，
         * 用来观察花屏是否会随时间变化。
         */
        if (lcd_a_ok) {
            drawTest(
                lcd_a,
                (counter & 1U) != 0
            );
        }

        if (lcd_b_ok) {
            drawTest2(
                lcd_b,
                (counter & 1U) == 0
            );
        }

        ESP_LOGI(
            TAG,
            "Refresh cycle %lu",
            static_cast<unsigned long>(
                counter
            )
        );

        ++counter;

        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );
    }
}

/*
#include <chrono>
#include <thread>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lcore/ThreadMgr.h"
#include "ldevice/screen/DevScreen.h"
#include "ldevice/screen/ST7305/ST7305ScreenDriver.h"
#include "lui/base/UIRender.h"
//#include "apps/demo3.h"
#include "apps/demo4.h"

namespace {
constexpr char TAG[] = "LectOS2";
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "LectOS 2 starting");

    // Driver 内含约 50 KiB framebuffer，
    // 必须使用静态存储，不能放进 app_main 栈。

    static ldevice::driver::ST7305ScreenDriver lcd_driver({
        .spi_host = SPI2_HOST,
        //.pin_mosi = GPIO_NUM_39,
        //.pin_sclk = GPIO_NUM_38,
        //.pin_cs   = GPIO_NUM_45,
        //.pin_dc   = GPIO_NUM_48,
        //.pin_rst  = GPIO_NUM_47,

        // 按实际 PCB 引脚修改
        .pin_mosi = GPIO_NUM_6,
        .pin_sclk = GPIO_NUM_7,
        .pin_cs   = GPIO_NUM_15,
        .pin_dc   = GPIO_NUM_16,
        .pin_rst  = GPIO_NUM_8,

        .spi_clock_hz = 30'000'000,
        .initialize_spi_bus = true
    });

    static ldevice::Screen screen;

    screen.getWidth() = 210;
    screen.getHeight() = 480;
    screen.getRefreshRate() = 10;
    screen.getColorMode() =
        ldevice::ScreenColorMode::RGB565;

    screen.setDriver(&lcd_driver);

    if (!screen.getDriver()->initScreenCmd()) {
        ESP_LOGE(TAG, "ST7305 initialization failed");

        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "ST7305 initialized");


    // --- Platform services ---
    static lui::Render renderer(&screen);

    // --- Thread manager ---
    static lcore::ThreadMgr thread_mgr;

    const uint32_t render_id =
        thread_mgr.registerService(
            lcore::SERVICE_RENDER,
            &renderer
        );

    // 当前板端没有 InputDevice / Ctrller，
    // 因此暂时不注册 SERVICE_CTRLLER。


    // --- Applications ---
    static Demo4 demo4;

    demo4.setServices(
        &screen,
        &renderer,
        nullptr
    );

    const uint32_t demo4_id =
        thread_mgr.registerApp(&demo4);

    // --- Start services ---
    thread_mgr.startService(render_id);

    // --- Start application ---
    // ThreadMgr 内部执行 app_setup() → app_main()
    thread_mgr.startApp(demo4_id);

    // 等待 app_setup 完成
    vTaskDelay(pdMS_TO_TICKS(300));

    // 设置前台应用
    thread_mgr.switchForeground(demo4_id);

    // Demo4 自动巡航，正常情况下不会主动退出，
    // 因而这里会长期阻塞。

    thread_mgr.joinApp(demo4_id);

    // 理论清理路径
    thread_mgr.stopService(render_id);
    screen.getDriver()->closeScreenCmd();

    ESP_LOGI(TAG, "LectOS 2 stopped");
}
*/