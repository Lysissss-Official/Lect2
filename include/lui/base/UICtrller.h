//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UICTRLLER_H
#define APSISUI2_UICTRLLER_H

#include <cstdint>
#include <vector>

namespace lui {
    namespace ctrller {

        struct KeyboardState {
            std::vector<uint8_t> key;
        };

        struct SwitchState {
            float degree = -1.0f;   // Joystick angle: 0=right, 90=up, 180=left, 270=down, -1=neutral
        };

        class CtrllerService {
        protected:
            KeyboardState kb_state;
            SwitchState   sw_state;

        public:
            uint8_t getKB(uint16_t vk_code) {
                if (vk_code < kb_state.key.size()) {
                    return kb_state.key[vk_code];
                }
                return 2;   // Out of range
            }

            float getSW() {
                return sw_state.degree;
            }

            virtual void refreshStatus() = 0;
        };
    }
}

#endif //APSISUI2_UICTRLLER_H
