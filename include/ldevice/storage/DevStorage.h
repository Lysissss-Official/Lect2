//
// Created by archeart on 2026/7/15.
//

#ifndef APSISUI2_DEVSTORAGE_H
#define APSISUI2_DEVSTORAGE_H

#include <cstdint>

namespace ldevice {
    enum class StorageType : uint8_t {
        SCREEN_BW   = 0,    // 黑白双色
        SCREEN_GRAY = 1,    // 灰度模式
        SCREEN_RGB565  = 2, // 16位色模式
        SCREEN_RGB666 = 3,  // 18位色模式
        SCREEN_RGB888  = 4  // 24位色模式
    };

    class Storage {
        public:

    };
}

#endif //APSISUI2_DEVSTORAGE_H
