//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UIRENDER_H
#define APSISUI2_UIRENDER_H

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <variant>
#include <algorithm>

#include "UIStructure.h"
#include "lcore/Log.h"

namespace lui {

    class Render {
    protected:
        struct RenderRequest {
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now();
            std::chrono::time_point<std::chrono::steady_clock> time_end   = std::chrono::steady_clock::now();
            std::variant<std::monostate, Page*, Block*, Element*> target_ptr;
            std::function<float(float)> animation_func = [](float x){ return 1.0f; };
            bool is_rf = true;
        };

        struct CmpByTimeEnd {
            bool operator()(const RenderRequest& a, const RenderRequest& b) const {
                return a.time_end > b.time_end;
            }
        };

        std::vector<RenderRequest> render_queue_;

        void popExpired() {
            auto now = std::chrono::steady_clock::now();
            while (!render_queue_.empty() && render_queue_.front().time_end <= now) {
                std::pop_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
                render_queue_.pop_back();
            }
        }

        Page* current_page_ = nullptr;

    public:
        void requestReDraw(std::variant<std::monostate, Page*, Block*, Element*> target_ptr,
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now(),
            std::chrono::duration<float, std::milli> time_use = std::chrono::duration<float, std::milli>(0),
            std::function<float(float)> animation_func = [](float x){ return 1.0f; },
            bool is_rf = true)
        {
            RenderRequest req;
            req.target_ptr     = target_ptr;
            req.time_start     = time_start;
            req.time_end       = time_start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(time_use);
            req.animation_func = std::move(animation_func);
            req.is_rf          = is_rf;
            {
                std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
                render_queue_.push_back(std::move(req));
                std::push_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
            }
            cv_.notify_one();
        }

        void setCurrentPage(Page* page) {
            current_page_ = page;
            LOG("Render target page set  --  page=0x"
                + std::to_string(reinterpret_cast<uintptr_t>(page)));
            requestReDraw(page);
        }

        void startDaemon() {
            if (daemon_running_.load(std::memory_order_acquire)) return;
            LOG("Render daemon starting");
            daemon_running_.store(true, std::memory_order_release);
            daemon_thread_ = std::thread(&Render::daemonLoop, this);
        }

        void stopDaemon() {
            LOG("Render daemon stopping");
            daemon_running_.store(false, std::memory_order_release);
            cv_.notify_one();
            if (daemon_thread_.joinable()) {
                daemon_thread_.join();
                LOG("Render daemon stopped");
            }
        }

        virtual void renderService() {}

    private:
        std::recursive_mutex queue_mutex_;
        std::condition_variable_any cv_;
        std::thread daemon_thread_;
        std::atomic<bool> daemon_running_{false};

        void daemonLoop() {
            using namespace std::chrono;

            while (daemon_running_.load(std::memory_order_acquire)) {
                {
                    std::unique_lock<std::recursive_mutex> lock(queue_mutex_);
                    popExpired();
                    if (render_queue_.empty()) {
                        cv_.wait(lock, [this] {
                            return !render_queue_.empty()
                                || !daemon_running_.load(std::memory_order_acquire);
                        });
                        popExpired();
                    }
                }
                if (!daemon_running_.load(std::memory_order_acquire)) break;

                if (current_page_) {
                    std::lock_guard lock(current_page_->state_mutex);
                    std::lock_guard qlock(queue_mutex_);
                    renderService();
                }

                {
                    std::unique_lock<std::recursive_mutex> lock(queue_mutex_);
                    popExpired();
                    if (!render_queue_.empty()
                        && daemon_running_.load(std::memory_order_acquire)) {
                        auto nearest = render_queue_.front().time_end;
                        auto now = steady_clock::now();
                        if (nearest > now) {
                            auto wait = duration_cast<milliseconds>(nearest - now);
                            if (wait > milliseconds(8))
                                wait = milliseconds(8);
                            cv_.wait_for(lock, wait);
                        }
                    }
                }
            }
        }
    };
}

#endif //APSISUI2_UIRENDER_H
