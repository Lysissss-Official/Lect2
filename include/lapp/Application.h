//
// Created by archeart on 2026/5/24.
//
// LAPP Application 层 — 应用基类
// ============================================================================
// Application 不是一个 UI 组件（不像 Element / Block / Page / Screen），
// 而是持有可执行代码（app_main）与四项平台服务的运行实体。
// 因此独立为 lapp 命名空间，与 lui 平级。
// ============================================================================

#ifndef APSISUI2_APPLICATION_H
#define APSISUI2_APPLICATION_H

#include <string>
#include "lcore/IDGenerator.h"          // IDGenerator
#include "lui/base/UIStructure.h"       // Page
#include "ldevice/screen/DevScreen.h"   // Screen

// 四项平台服务 — 前向声明（成员为指针，不依赖完整类型）
namespace lui {
    class Render;
    class CtrllerService;
    class TranslatorService;
}

namespace lapp {

    enum ApplicationType {
        APP_NATIVE  = 0,  // C++ 原生应用
        APP_VM      = 1,  // pocketpy 脚本应用
        APP_UNKNOWN = 2   // 未初始化
    };

    class Application {
    private:
        uint32_t uni_id;

    public:
        ApplicationType type  = APP_UNKNOWN;
        std::string vm_path;                 // VM 模式的 Python 脚本路径
        lui::strc::Page* start_page = nullptr;

        // 四项平台服务 — 由 setServices() 注入，app_setup() / app_main() 中使用
        ldevice::Screen*          screen     = nullptr;
        lui::Render*                renderer   = nullptr;
        lui::CtrllerService*        controller = nullptr;
        lui::TranslatorService*     translator = nullptr;

        Application() {
            uni_id = lcore::IDGenerator<Application>::generate();
        }
        virtual ~Application() {
            lcore::IDGenerator<Application>::release(uni_id);
        }

        // 注入四项平台服务（在 registerApp 之后、startApp 之前调用）
        void setServices(ldevice::Screen* scr, lui::Render* ren,
                         lui::CtrllerService* ctl, lui::TranslatorService* trs) {
            screen     = scr;
            renderer   = ren;
            controller = ctl;
            translator = trs;
        }

        // App 线程启动时自动调用（在 app_main 之前），子类可选覆写
        virtual void app_setup() {}

        virtual void app_main() = 0;

        uint32_t getID() const { return uni_id; }
    };

} // namespace lapp

#endif //APSISUI2_APPLICATION_H
