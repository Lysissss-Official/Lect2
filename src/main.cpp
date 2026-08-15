//
// ApsisUI II entry point — Launcher 默认前台，管理所有应用。
//

#include <chrono>
#include <thread>

#include "lcore/ThreadMgr.h"
#include "ldevice/screen/DevScreen.h"
#include "lapp/Application.h"
#include "lui/base/UIStructure.h"
#include "lui/platform/linux7/UICtrllerLinux7.h"
#include "lui/platform/linux7/UITransorLinux7.h"
//#include "lui/platform/linux7/theme/UIRenderApsisUI2.h"
//#include "apps/demo_app.h"
//#include "apps/pyconsole_app.h"
//#include "apps/launcher_app.h"
#include "apps/demo2.h"
//#include "apps/demo3.h"
#include "ldevice/screen/EasyX/EasyXScreenDriver.h"

int main() {
    // --- Screen（EasyX 窗口）---
    ldevice::Screen screen;
    ldevice::driver::EasyXScreenDriver easyx_vsd({
        .width = 1200,
        .height = 480,
        .title = L"LectOS 2 | ApsisUI II",
        .clear_color = 0x101010,
        .use_batch_draw = false
    });
    screen.getWidth()  = 1200;
    screen.getHeight() = 480;
    screen.setDriver(&easyx_vsd);
    screen.getDriver()->initScreenCmd();

    // --- Platform services ---
    lui::UICtrllerLinux7  controller;
    //lui::UITransorLinux7  translator;
    lui::Render  renderer(&screen);

    // --- Thread manager ---
    lcore::ThreadMgr threadMgr;

    uint32_t ctl_id = threadMgr.registerService(lcore::SERVICE_CTRLLER, &controller);
    uint32_t ren_id = threadMgr.registerService(lcore::SERVICE_RENDER,  &renderer);

    /*
    // --- 启动台（最先注册，默认前台）---
    LauncherApp launcher;
    launcher.setServices(&screen, &renderer, &controller, &translator);
    uint32_t launcher_id = threadMgr.registerApp(&launcher);
    launcher.setThreadMgr(&threadMgr);

    // --- Demo 应用 ---
    DemoApp demo;
    demo.setServices(&screen, &renderer, &controller, &translator);
    uint32_t demo_id = threadMgr.registerApp(&demo);
    launcher.addEntry("ApsisUI II Demo", demo_id);

    // --- PyConsole 应用 ---
    PyConsoleApp pycon;
    pycon.setServices(&screen, &renderer, &controller, &translator);
    uint32_t pycon_id = threadMgr.registerApp(&pycon);
    launcher.addEntry("pocketpy Console", pycon_id);
    */

    Demo2 demo2;
    demo2.setServices(&screen,&renderer,&controller);
    const uint32_t demo2_id = threadMgr.registerApp(&demo2);

    //Demo3 demo3;
    //demo3.setServices(&screen,&renderer,&controller);
    //const uint32_t demo3_id = threadMgr.registerApp(&demo3);

    // 启动守护线程
    threadMgr.startService(ren_id);
    threadMgr.startService(ctl_id);

    // 启动全部应用（每个 app_setup → app_main 在独立线程运行）
    //threadMgr.startApp(launcher_id);
    //threadMgr.startApp(demo_id);
    //threadMgr.startApp(pycon_id);
    threadMgr.startApp(demo2_id);
    //threadMgr.startApp(demo3_id);

    // 等待 app_setup 完成
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 启动台默认前台
    //threadMgr.switchForeground(launcher_id);
    threadMgr.switchForeground(demo2_id);

    // 阻塞直到用户在启动台中按 ESC 退出
    //threadMgr.joinApp(launcher_id);
    threadMgr.joinApp(demo2_id);

    // 清理
    //threadMgr.stopApp(demo_id);
    //threadMgr.stopApp(pycon_id);
    threadMgr.stopService(ctl_id);
    threadMgr.stopService(ren_id);

    screen.getDriver()->closeScreenCmd();
    return 0;
}
