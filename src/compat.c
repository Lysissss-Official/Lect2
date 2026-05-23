//
// Created by archeart on 2026/5/22.
// ------------------------------------------------------------
// compat.c
// 解决 MinGW 编译器在 Linux 下不认 MSVC 库的问题（EasyX: 说的就是你）
// ------------------------------------------------------------
//

#include <stdio.h>

FILE* __imp___iob_func(void) {
    static FILE iob[3];
    static int initialized = 0;
    if (!initialized) {
        iob[0] = *stdin;
        iob[1] = *stdout;
        iob[2] = *stderr;
        initialized = 1;
    }
    return iob;
}