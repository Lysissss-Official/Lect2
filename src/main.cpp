//
// Created by archeart on 2026/5/22.
// ------------
// main.cpp
// ApsisUI II entry point — runs the demo application.
// ------------
//

#include <iostream>

#include "easyx.h"
#include "graphics.h"
#include "pocketpy.h"

#include "lui/base/UIStructure.h"
#include "lui/platform/linux7/UICtrllerLinux7.h"
#include "lui/platform/linux7/UITransorLinux7.h"
#include "lui/theme/UIRenderApsisUI2.h"
#include "apps/demo_app.h"

int main() {
    initgraph(1200, 480);

    // --- Screen ---
    lui::scr::Screen screen;
    screen.setID(1);
    screen.width  = 1200;
    screen.height = 480;

    // --- Platform services ---
    lui::ctrller::UICtrllerLinux7  controller;
    lui::transor::UITransorLinux7  translator;
    lui::render::UIRenderApsisUI2  renderer(&translator);

    // --- Demo application ---
    DemoApp app;
    app.setID(1);
    app.setup(&screen, &renderer, &controller, &translator);

    std::cout << "LectOS 2 / ApsisUI II Demo" << std::endl;
    std::cout << "Arrow keys = navigate focus | ESC = exit" << std::endl;

    app.app_main();

    closegraph();
    return 0;
}
