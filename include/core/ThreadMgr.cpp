//
// Created by archeart on 2026/5/23.
//

#include "ThreadMgr.h"

using namespace core;

ThreadMgr::ThreadMgr() {
    current_fg = 0;
}

ThreadMgr::~ThreadMgr() {
    for(auto &it : runtime_table) {
        stopApp(it.first);
    }
}

void ThreadMgr::registerApp(lui::app::Application* app) {
    if(app == nullptr) {
        return;
    }

    AppRuntime runtime;

    runtime.app_id = app->getID();
    runtime.app_ptr = app;

    runtime.foreground = false;
    runtime.allow_input = false;
    runtime.allow_render = false;

    runtime.state = THREAD_STOPPED;

    runtime.native_thread = nullptr;

    runtime_table[app->getID()] = runtime;
}

void ThreadMgr::startApp(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return;
    }

    AppRuntime &runtime = it->second;

    if(runtime.native_thread != nullptr) {
        return;
    }

    runtime.state = THREAD_RUNNING;

    runtime.native_thread = new std::thread(
        [this, id]() {
            auto it = runtime_table.find(id);

            if(it == runtime_table.end()) {
                return;
            }

            AppRuntime &runtime = it->second;

            if(runtime.app_ptr != nullptr) {
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
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return;
    }

    AppRuntime &runtime = it->second;

    runtime.foreground = false;
    runtime.allow_input = false;
    runtime.allow_render = false;

    runtime.state = THREAD_STOPPED;

    if(runtime.native_thread != nullptr) {
        if(runtime.native_thread->joinable()) {
            runtime.native_thread->join();
        }

        delete runtime.native_thread;

        runtime.native_thread = nullptr;
    }
}

void ThreadMgr::suspendApp(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return;
    }

    it->second.state = THREAD_SUSPEND;
}

void ThreadMgr::resumeApp(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return;
    }

    it->second.state = THREAD_RUNNING;
}

void ThreadMgr::switchForeground(uint32_t id) {
    for(auto &it : runtime_table) {
        it.second.foreground = false;

        it.second.allow_input = false;
        it.second.allow_render = false;
    }

    auto target = runtime_table.find(id);

    if(target == runtime_table.end()) {
        return;
    }

    target->second.foreground = true;

    target->second.allow_input = true;
    target->second.allow_render = true;

    current_fg = id;
}

bool ThreadMgr::canInput(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return false;
    }

    return it->second.allow_input;
}

bool ThreadMgr::canRender(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return false;
    }

    return it->second.allow_render;
}

AppRuntime* ThreadMgr::getRuntime(uint32_t id) {
    auto it = runtime_table.find(id);

    if(it == runtime_table.end()) {
        return nullptr;
    }

    return &(it->second);
}