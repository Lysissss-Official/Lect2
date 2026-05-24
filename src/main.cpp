//
// ApsisUI II entry point — Demo + PyConsole 双应用。

#include <chrono>
#include <thread>

#include "lcore/ThreadMgr.h"
#include "lapp/Application.h"
#include "lui/base/UIStructure.h"
#include "lui/platform/linux7/UICtrllerLinux7.h"
#include "lui/platform/linux7/UITransorLinux7.h"
#include "lui/platform/linux7/theme/UIRenderApsisUI2.h"
#include "apps/demo_app.h"
#include "apps/pyconsole_app.h"

int main() {
    // --- Screen（EasyX 窗口）---
    lui::Screen screen;
    screen.width  = 1200;
    screen.height = 480;
    screen.init();

    // --- Platform services ---
    lui::UICtrllerLinux7  controller;
    lui::UITransorLinux7  translator;
    lui::UIRenderApsisUI2  renderer(&translator);

    // --- Thread manager ---
    lcore::ThreadMgr threadMgr;

    uint32_t ctl_id = threadMgr.registerService(lcore::SERVICE_CTRLLER, &controller);
    uint32_t ren_id = threadMgr.registerService(lcore::SERVICE_RENDER,  &renderer);

    // --- Demo 应用 ---
    DemoApp demo;
    demo.setServices(&screen, &renderer, &controller, &translator);
    uint32_t demo_id = threadMgr.registerApp(&demo);

    // --- PyConsole 应用 ---
    PyConsoleApp pycon;
    pycon.setServices(&screen, &renderer, &controller, &translator);
    uint32_t pycon_id = threadMgr.registerApp(&pycon);

    // 启动守护线程
    threadMgr.startService(ren_id);
    threadMgr.startService(ctl_id);

    // 启动两个应用
    threadMgr.startApp(demo_id);
    threadMgr.startApp(pycon_id);

    // 等待 app_setup 完成（简单延时，生产环境应改用同步机制）
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // PyConsole 默认前台（注入其 start_page 到 renderd）


    threadMgr.switchForeground(demo_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    threadMgr.switchForeground(pycon_id);


    threadMgr.joinApp(pycon_id);
    threadMgr.joinApp(demo_id);

    threadMgr.stopService(ctl_id);
    threadMgr.stopService(ren_id);

    screen.close();
    return 0;
}
