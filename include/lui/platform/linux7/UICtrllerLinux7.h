//
// UICtrllerLinux7 — EasyX-based controller (full-keyboard polling)
// ============================================================================
// 每帧轮询全部 256 个虚拟键码，存入 kb_state.key[]。
// 同时从方向键计算虚拟摇杆角度 (SwitchState)。
//

#ifndef APSISUI2_UICTRLLERLINUX7_H
#define APSISUI2_UICTRLLERLINUX7_H

#include <windows.h>
#include "../../base/UICtrller.h"

namespace lui {

    class UICtrllerLinux7 : public CtrllerService {
    public:
        UICtrllerLinux7() {
            kb_state.key.resize(256, 0);
        }

        void refreshStatus() override {
            // Wine: 确保后台线程有消息队列
            MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

            // 轮询全部 256 个虚拟键码
            for (int vk = 0; vk < 256; ++vk) {
                kb_state.key[vk] = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
            }

            // 方向键 → 虚拟摇杆角度
            bool pr = kb_state.key[VK_RIGHT];
            bool pu = kb_state.key[VK_UP];
            bool pl = kb_state.key[VK_LEFT];
            bool pd = kb_state.key[VK_DOWN];

            if      (pr) sw_state.degree = 0.0f;
            else if (pu) sw_state.degree = 90.0f;
            else if (pl) sw_state.degree = 180.0f;
            else if (pd) sw_state.degree = 270.0f;
            else         sw_state.degree = -1.0f;
        }
    };
}

#endif //APSISUI2_UICTRLLERLINUX7_H
