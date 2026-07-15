//
// Created by archeart on 2026/7/15.
//

#include "EasyXScreenDriver.h"

#include <algorithm>

namespace ldevice::driver {

EasyXScreenDriver::EasyXScreenDriver(
    const Config& config
) : config_(config) {}

EasyXScreenDriver::~EasyXScreenDriver() {
    closeScreenCmd();
}

COLORREF EasyXScreenDriver::toEasyXColor(
    uint32_t color
) {
    /*
     * LectOS 当前颜色值按 0xRRGGBB 使用。
     * EasyX / COLORREF 内部是 0x00BBGGRR，
     * 因此通过 RGB() 显式转换。
     */
    const uint8_t red =
        static_cast<uint8_t>(
            (color >> 16U) & 0xFFU
        );

    const uint8_t green =
        static_cast<uint8_t>(
            (color >> 8U) & 0xFFU
        );

    const uint8_t blue =
        static_cast<uint8_t>(
            color & 0xFFU
        );

    return RGB(red, green, blue);
}

int EasyXScreenDriver::fontHeightFromType(
    uint16_t font_type
) {
    switch (font_type) {
        case 0:
            return 12;

        case 1:
            return 16;

        case 2:
            return 24;

        default:
            return std::max<int>(
                static_cast<int>(font_type),
                8
            );
    }
}

bool EasyXScreenDriver::initScreenCmd() {
    if (initialized_) {
        return true;
    }

    if (
        config_.width == 0 ||
        config_.height == 0
    ) {
        return false;
    }

    initgraph(
        static_cast<int>(config_.width),
        static_cast<int>(config_.height)
    );

    if (config_.title) {
        HWND window =
            GetHWnd();

        if (window) {
            SetWindowTextW(
                window,
                config_.title
            );
        }
    }

    if (config_.use_batch_draw) {
        BeginBatchDraw();
    }

    initialized_ = true;

    ClearScreenCmd();
    refreshScreenCmd();

    return true;
}

bool EasyXScreenDriver::closeScreenCmd() {
    if (!initialized_) {
        return true;
    }

    if (config_.use_batch_draw) {
        FlushBatchDraw();
        EndBatchDraw();
    }

    closegraph();

    initialized_ = false;
    return true;
}

bool EasyXScreenDriver::refreshScreenCmd() {
    if (!initialized_) {
        return false;
    }

    if (config_.use_batch_draw) {
        FlushBatchDraw();
    }

    return true;
}

bool EasyXScreenDriver::ClearScreenCmd() {
    if (!initialized_) {
        return false;
    }

    setbkcolor(
        toEasyXColor(
            config_.clear_color
        )
    );

    cleardevice();
    return true;
}

bool EasyXScreenDriver::drawPixelCmd(
    uint16_t x,
    uint16_t y,
    uint32_t color
) {
    if (
        !initialized_ ||
        x >= config_.width ||
        y >= config_.height
    ) {
        return false;
    }

    putpixel(
        static_cast<int>(x),
        static_cast<int>(y),
        toEasyXColor(color)
    );

    return true;
}

bool EasyXScreenDriver::drawFontCmd(
    uint16_t x,
    uint16_t y,
    uint32_t color,
    uint16_t font_type,
    uint16_t unicode
) {
    if (!initialized_) {
        return false;
    }

    settextcolor(
        toEasyXColor(color)
    );

    setbkmode(TRANSPARENT);

    settextstyle(
        fontHeightFromType(font_type),
        0,
        "Consolas"
    );

    outtextxy(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<TCHAR>(unicode)
    );

    return true;
}

bool EasyXScreenDriver::drawLineCmd(
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2,
    uint32_t color
) {
    if (!initialized_) {
        return false;
    }

    setlinecolor(
        toEasyXColor(color)
    );

    line(
        static_cast<int>(x1),
        static_cast<int>(y1),
        static_cast<int>(x2),
        static_cast<int>(y2)
    );

    return true;
}

bool EasyXScreenDriver::drawFilledRectCmd(
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2,
    uint32_t fill_color,
    uint32_t border_color
) {
    if (!initialized_) {
        return false;
    }

    const int left =
        std::min<int>(x1, x2);

    const int right =
        std::max<int>(x1, x2);

    const int top =
        std::min<int>(y1, y2);

    const int bottom =
        std::max<int>(y1, y2);

    setfillcolor(
        toEasyXColor(fill_color)
    );

    setlinecolor(
        toEasyXColor(border_color)
    );

    fillrectangle(
        left,
        top,
        right,
        bottom
    );

    return true;
}

} // namespace ldevice::driver