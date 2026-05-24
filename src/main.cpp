//
// ApsisUI II entry point — Launcher 默认前台，管理所有应用。
//

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
#include "apps/launcher_app.h"

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

    // 启动守护线程
    threadMgr.startService(ren_id);
    threadMgr.startService(ctl_id);

    // 启动全部应用（每个 app_setup → app_main 在独立线程运行）
    threadMgr.startApp(launcher_id);
    threadMgr.startApp(demo_id);
    threadMgr.startApp(pycon_id);

    // 等待 app_setup 完成
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 启动台默认前台
    threadMgr.switchForeground(launcher_id);

    // 阻塞直到用户在启动台中按 ESC 退出
    threadMgr.joinApp(launcher_id);

    // 清理
    threadMgr.stopApp(demo_id);
    threadMgr.stopApp(pycon_id);
    threadMgr.stopService(ctl_id);
    threadMgr.stopService(ren_id);

    screen.close();
    return 0;
}
