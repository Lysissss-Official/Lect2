//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UICTRLLER_H
#define APSISUI2_UICTRLLER_H

#include <stdint.h>

namespace lui {
    namespace ctrller {
        struct KeyboardState {
            std::vector<uint8_t> key;
        };
        struct SwitchState {
            float degree;
        };

        class CtrllerService {
        protected:
            KeyboardState kb_state;
            SwitchState sw_state;
        public:
            uint8_t getKB(uint16_t no) {
                if (no < kb_state.key.size()) {
                    return kb_state.key[no];
                }
                else {
                    return 2;
                }
            }
            double getSW() {
                return sw_state;
            }
            virtual void refreshStatus() = 0;
        };
    }
}

#endif //APSISUI2_UICTRLLER_H
