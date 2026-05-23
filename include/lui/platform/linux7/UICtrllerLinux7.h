//
// Created by archeart on 2026/5/23.
//
// EasyX-based controller implementation.
// Maps arrow keys to a virtual joystick: Right=0°, Up=90°, Left=180°, Down=270°.
//

#ifndef APSISUI2_UICTRLLERLINUX7_H
#define APSISUI2_UICTRLLERLINUX7_H

#include <windows.h>
#include "../../base/UICtrller.h"

namespace lui {
    namespace ctrller {

        class UICtrllerLinux7 : public CtrllerService {
        public:
            UICtrllerLinux7() {
                kb_state.key.resize(256, 0);
            }

            void refreshStatus() override {
                // Clear previous key states
                for (auto& k : kb_state.key) k = 0;

                SHORT r = GetAsyncKeyState(VK_RIGHT);
                SHORT u = GetAsyncKeyState(VK_UP);
                SHORT l = GetAsyncKeyState(VK_LEFT);
                SHORT d = GetAsyncKeyState(VK_DOWN);

                bool pr = (r & 0x8000) != 0;
                bool pu = (u & 0x8000) != 0;
                bool pl = (l & 0x8000) != 0;
                bool pd = (d & 0x8000) != 0;

                if (pr) { kb_state.key[VK_RIGHT] = 1; sw_state.degree = 0.0f; }
                if (pu) { kb_state.key[VK_UP]    = 1; sw_state.degree = 90.0f; }
                if (pl) { kb_state.key[VK_LEFT]  = 1; sw_state.degree = 180.0f; }
                if (pd) { kb_state.key[VK_DOWN]  = 1; sw_state.degree = 270.0f; }

                // Priority: Right > Up > Left > Down (last-set wins with degree)
                // Re-set degree with priority order
                if      (pr) sw_state.degree = 0.0f;
                else if (pu) sw_state.degree = 90.0f;
                else if (pl) sw_state.degree = 180.0f;
                else if (pd) sw_state.degree = 270.0f;
                else         sw_state.degree = -1.0f;
            }
        };
    }
}

#endif //APSISUI2_UICTRLLERLINUX7_H
