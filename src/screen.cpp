//
// Screen 平台实现 — EasyX (Windows) 显示设备初始化
//

#include "lui/base/UIStructure.h"

#include "graphics.h"   // EasyX: initgraph, closegraph

namespace lui {

    void Screen::init() {
        initgraph(width, height);
    }

    void Screen::close() {
        closegraph();
    }

} // namespace lui
