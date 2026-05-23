//
// Created by archeart on 2026/5/22.
// ------------
// main.cpp
// 临时主程序替代
// ------------
//

#include <iostream>

#include "easyx.h"
#include "graphics.h"
#include "pocketpy.h"

int main() {
    initgraph(640, 480);
    circle(200,200,100);
    outtextxy(20, 20, "Hello, LectOS 2! 你好！");
    getchar();
    getchar();
    return 0;
}
