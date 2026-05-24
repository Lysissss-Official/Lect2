//
// Created by archeart on 2026/5/23.
//

#ifndef APSISUI2_THREADMGR_H
#define APSISUI2_THREADMGR_H

#include <cstdint>
#include <map>
#include <thread>

#include "../lapp/Application.h"
#include "../lui/base/UICtrller.h"
#include "../lui/base/UIRender.h"

namespace lcore {

    enum ThreadState {
        THREAD_STOPPED = 0,
        THREAD_RUNNING,
        THREAD_SUSPEND,
        THREAD_BLOCKED
    };

    struct AppRuntime {
        uint32_t app_id;

        bool foreground;
        bool allow_input;
        bool allow_render;

        ThreadState state;

        std::thread* native_thread;
        lapp::Application* app_ptr;

        AppRuntime()
        {
            app_id = 0;

            foreground = false;
            allow_input = false;
            allow_render = false;

            state = THREAD_STOPPED;

            native_thread = nullptr;
            app_ptr = nullptr;
        }
    };

    enum ServiceType {
        SERVICE_CTRLLER = 0,
        SERVICE_RENDER  = 1,
        SERVICE_UNKNOWN = 100
    };

    struct ServiceRuntime {
        uint32_t    service_id;
        ServiceType type;
        ThreadState state;
        void*       service_ptr;

        ServiceRuntime()
        {
            service_id = 0;
            type       = SERVICE_UNKNOWN;
            state      = THREAD_STOPPED;
            service_ptr = nullptr;
        }
    };

    class ThreadMgr {

    private:
        std::map<uint32_t, AppRuntime>     app_table;
        std::map<uint32_t, ServiceRuntime> service_table;
        uint32_t foreground_now;

    public:
        ThreadMgr();
        ~ThreadMgr();

        // ---- App 操作 ----

        uint32_t registerApp(
            lapp::Application* app
        );

        void startApp(
            uint32_t id
        );

        void stopApp(
            uint32_t id
        );

        void suspendApp(
            uint32_t id
        );

        void resumeApp(
            uint32_t id
        );

        void switchForeground(
            uint32_t id
        );

        bool canInput(
            uint32_t id
        );

        bool canRender(
            uint32_t id
        );

        AppRuntime*
        getRuntime(
            uint32_t id
        );

        void joinApp(
            uint32_t id
        );

        // ---- Service 操作 ----

        uint32_t registerService(
            ServiceType type,
            void* ptr
        );

        void startService(
            uint32_t id
        );

        void stopService(
            uint32_t id
        );

    private:
        uint32_t service_id_next_ = 1;
        uint32_t app_id_next_     = 4097;
    };

}

#endif //APSISUI2_THREADMGR_H
