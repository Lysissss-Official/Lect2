//
// Created by archeart on 2026/5/23.
//

#ifndef APSISUI2_THREADMGR_H
#define APSISUI2_THREADMGR_H

#include <cstdint>
#include <map>
#include <thread>

#include "../lui/base/UIStructure.h"

namespace core {

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
        lui::app::Application* app_ptr;

        AppRuntime()
        {
            app_id = 0;

            foreground =false;
            allow_input =false;
            allow_render =false;

            state =THREAD_STOPPED;

            native_thread = nullptr;
            app_ptr = nullptr;
        }
    };

    class ThreadMgr {

    private:
        std::map<uint32_t,AppRuntime> runtime_table;
        uint32_t current_fg;

    public:
        ThreadMgr()
        {
            current_fg = 0;
        }

        void registerApp(
            lui::app::Application* app
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
    };

}

#endif //APSISUI2_THREADMGR_H
