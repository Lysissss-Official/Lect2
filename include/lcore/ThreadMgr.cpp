//
// Created by archeart on 2026/5/23.
//

#include "ThreadMgr.h"
#include "Log.h"

using namespace lcore;

static const char* svcName(ServiceType t) {
    switch (t) {
        case SERVICE_CTRLLER: return "Ctrller";
        case SERVICE_RENDER:  return "Render";
        default:              return "Unknown";
    }
}

ThreadMgr::ThreadMgr() {
    foreground_now = 0;
}

ThreadMgr::~ThreadMgr() {
    for (auto &it : service_table) {
        stopService(it.first);
    }
    for (auto &it : app_table) {
        stopApp(it.first);
    }
}

// =====================================================================
// App 操作
// =====================================================================

uint32_t ThreadMgr::registerApp(lapp::Application* app) {
    if (app == nullptr) {
        return 0;
    }

    AppRuntime runtime;

    runtime.app_id  = app_id_next_++;
    runtime.app_ptr = app;

    runtime.foreground = false;
    runtime.allow_input = false;
    runtime.allow_render = false;

    runtime.state = THREAD_STOPPED;

    runtime.native_thread = nullptr;

    app_table[runtime.app_id] = runtime;
    LOG("App registered  --  ID=" + std::to_string(runtime.app_id));

    return runtime.app_id;
}

void ThreadMgr::startApp(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return;
    }

    AppRuntime &runtime = it->second;

    if (runtime.native_thread != nullptr) {
        return;
    }

    LOG("App thread spawning  --  ID=" + std::to_string(id));

    runtime.state = THREAD_RUNNING;

    runtime.native_thread = new std::thread(
        [this, id]() {
            auto it = app_table.find(id);

            if (it == app_table.end()) {
                return;
            }

            AppRuntime &runtime = it->second;

            if (runtime.app_ptr != nullptr) {
                runtime.app_ptr->app_setup();
                runtime.app_ptr->app_main();
            }

            runtime.state = THREAD_STOPPED;

            runtime.foreground = false;
            runtime.allow_input = false;
            runtime.allow_render = false;
        }
    );
}

void ThreadMgr::stopApp(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return;
    }

    AppRuntime &runtime = it->second;

    runtime.foreground = false;
    runtime.allow_input = false;
    runtime.allow_render = false;

    runtime.state = THREAD_STOPPED;

    if (runtime.native_thread != nullptr) {
        LOG("App thread joining  --  ID=" + std::to_string(id));

        if (runtime.native_thread->joinable()) {
            runtime.native_thread->join();
        }

        delete runtime.native_thread;

        runtime.native_thread = nullptr;
        LOG("App thread joined  --  ID=" + std::to_string(id));
    }
}

void ThreadMgr::suspendApp(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return;
    }

    it->second.state = THREAD_SUSPEND;
}

void ThreadMgr::resumeApp(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return;
    }

    it->second.state = THREAD_RUNNING;
}

void ThreadMgr::switchForeground(uint32_t id) {
    for (auto &it : app_table) {
        it.second.foreground = false;

        it.second.allow_input = false;
        it.second.allow_render = false;
    }

    auto target = app_table.find(id);

    if (target == app_table.end()) {
        return;
    }

    target->second.foreground = true;

    target->second.allow_input = true;
    target->second.allow_render = true;

    foreground_now = id;

    // 前台 App 的 start_page 注入 Render，renderd 只渲染前台页面
    if (target->second.app_ptr && target->second.app_ptr->start_page) {
        for (auto& [sid, srt] : service_table) {
            if (srt.type == SERVICE_RENDER) {
                auto* ren = static_cast<lui::Render*>(srt.service_ptr);
                ren->setCurrentPage(target->second.app_ptr->start_page);
                break;
            }
        }
    }

    LOG("Foreground switched  --  App ID=" + std::to_string(id));
}

bool ThreadMgr::canInput(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return false;
    }

    return it->second.allow_input;
}

bool ThreadMgr::canRender(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return false;
    }

    return it->second.allow_render;
}

AppRuntime* ThreadMgr::getRuntime(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return nullptr;
    }

    return &(it->second);
}

void ThreadMgr::joinApp(uint32_t id) {
    auto it = app_table.find(id);

    if (it == app_table.end()) {
        return;
    }

    AppRuntime &runtime = it->second;

    if (runtime.native_thread != nullptr) {
        if (runtime.native_thread->joinable()) {
            runtime.native_thread->join();
        }
    }
}

// =====================================================================
// Service 操作
// =====================================================================

uint32_t ThreadMgr::registerService(ServiceType type, void* ptr) {
    if (ptr == nullptr) return 0;

    ServiceRuntime runtime;
    runtime.service_id  = service_id_next_++;
    runtime.type        = type;
    runtime.service_ptr = ptr;
    runtime.state       = THREAD_STOPPED;

    service_table[runtime.service_id] = runtime;
    LOG("Service registered  --  " + std::string(svcName(type))
        + " ID=" + std::to_string(runtime.service_id));
    return runtime.service_id;
}

void ThreadMgr::startService(uint32_t id) {
    auto it = service_table.find(id);
    if (it == service_table.end()) return;

    ServiceRuntime& rt = it->second;
    if (rt.state == THREAD_RUNNING) return;

    switch (rt.type) {
        case SERVICE_CTRLLER:
            static_cast<lui::CtrllerService*>(
                rt.service_ptr)->startDaemon();
            break;
        case SERVICE_RENDER:
            static_cast<lui::Render*>(
                rt.service_ptr)->startDaemon();
            break;
        default: return;
    }

    rt.state = THREAD_RUNNING;
    LOG("Service daemon started  --  " + std::string(svcName(rt.type))
        + " ID=" + std::to_string(id));
}

void ThreadMgr::stopService(uint32_t id) {
    auto it = service_table.find(id);
    if (it == service_table.end()) return;

    ServiceRuntime& rt = it->second;
    if (rt.state == THREAD_STOPPED) return;

    switch (rt.type) {
        case SERVICE_CTRLLER:
            static_cast<lui::CtrllerService*>(
                rt.service_ptr)->stopDaemon();
            break;
        case SERVICE_RENDER:
            static_cast<lui::Render*>(
                rt.service_ptr)->stopDaemon();
            break;
        default: return;
    }

    rt.state = THREAD_STOPPED;
    LOG("Service daemon stopped  --  " + std::string(svcName(rt.type))
        + " ID=" + std::to_string(id));
}
