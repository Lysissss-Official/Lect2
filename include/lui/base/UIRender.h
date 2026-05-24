//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UIRENDER_H
#define APSISUI2_UIRENDER_H

#include <cstdint>
#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "UIStructure.h"
#include "lcore/Log.h"

namespace lui {

        // 动画缓动函数类型
        enum class AnimFunc : uint32_t {
            ANIM_LINEAR      = 0,  // 线性插值，匀速
            ANIM_EASE_IN     = 1,  // 三次缓入，慢→快
            ANIM_EASE_OUT    = 2,  // 三次缓出，快→慢
            ANIM_EASE_IN_OUT = 3,  // 三次缓入缓出，慢→快→慢
            ANIM_SINE_IN     = 4,  // 正弦缓入
            ANIM_SINE_OUT    = 5,  // 正弦缓出
            ANIM_NONE        = 6   // 无动画，瞬时
        };

        class Render {
        protected:
            enum RequestType {
                REQ_CHANGE_ELEMENT = 1,  // 元素属性变更: target_id=元素ID, args={"key",val...}
                REQ_CHANGE_PAGE    = 2,  // 页面切换:   target_id=页面ID, 渲染器清屏+切页
                REQ_DRAW_ELEMENT   = 3   // 强制重绘:   target_id=元素ID, 立即重绘指定元素
            };

            struct RenderRequest {
                uint32_t cpu_time_start = 0;
                uint32_t cpu_time_use   = 0;
                RequestType type        = REQ_CHANGE_PAGE;
                uint32_t target_id      = 0;
                AnimFunc   anime_func   = AnimFunc::ANIM_NONE;
                std::vector<std::pair<std::string, uint32_t>> args;
            };

            std::queue<RenderRequest> render_requests_pre_;
            std::list<RenderRequest>  render_requests_now_;

            void pushRequest(RequestType type, uint32_t target_id,
                             AnimFunc anim = AnimFunc::ANIM_NONE) {
                RenderRequest req;
                req.type      = type;
                req.target_id = target_id;
                req.anime_func = anim;
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    render_requests_pre_.push(std::move(req));
                }
                cv_.notify_one();
            }

        public:
            // ---- App 可调用的请求接口（不暴露 RenderRequest） ----

            void requestPageChange(uint32_t page_id) {
                LOG("Render request: ChangePage  --  page ID="
                    + std::to_string(page_id));
                pushRequest(REQ_CHANGE_PAGE, page_id);
            }

            void requestElementRedraw(uint32_t element_id,
                                      AnimFunc anim = AnimFunc::ANIM_NONE) {
                LOG("Render request: ElementRedraw  --  element ID="
                    + std::to_string(element_id));
                pushRequest(REQ_CHANGE_ELEMENT, element_id, anim);
            }

            // ---- 渲染目标页 ----

            void setCurrentPage(Page* page) {
                current_page_ = page;
                LOG("Render target page set  --  page=0x"
                    + std::to_string(reinterpret_cast<uintptr_t>(page)));
                cv_.notify_one();  // 触发首帧渲染
            }

            // ---- 守护线程生命周期 ----

            void startDaemon() {
                if (daemon_running_.load(std::memory_order_acquire)) return;
                LOG("Render daemon starting");
                daemon_running_.store(true, std::memory_order_release);
                daemon_thread_ = std::thread(&Render::daemonLoop, this);
            }

            void stopDaemon() {
                LOG("Render daemon stopping");
                daemon_running_.store(false, std::memory_order_release);
                cv_.notify_one();  // 唤醒 daemon 以便退出
                if (daemon_thread_.joinable()) {
                    daemon_thread_.join();
                    LOG("Render daemon stopped");
                }
            }

            // ---- 子类覆写 ----

            virtual void renderService() {}
            virtual void renderPage(Page* page) {}

            // 子类覆写以指示是否有待处理工作（如动画进行中），
            // 返回 true 时 daemon 以短间隔轮询，false 时无限阻塞等待请求。
            virtual bool hasPendingWork() { return false; }

        private:
            std::mutex queue_mutex_;        // 保护渲染请求队列(pre/now)访问的互斥锁（队列锁）
            std::condition_variable cv_;    // 线程挂起的条件变量
            std::thread daemon_thread_;     // 守护线程本体
            std::atomic<bool> daemon_running_{false};   // 线程安全的守护线程运行状态
            Page* current_page_ = nullptr;  // 当前渲染页面

            void daemonLoop() {             // Renderer_d 守护线程
                using namespace std::chrono;

                while (daemon_running_.load(std::memory_order_acquire)) {
                    // ---- 等待：有请求到达或动画需要下一帧 ----
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        // 今日有事可做
                        if (hasPendingWork()) {
                            cv_.wait_for(
                                lock,             // 睡小憩（）等待最多8ms；wait期间自动释放queue_mutex_，唤醒后重新加锁
                                milliseconds(8), // 避免CPU空转 超时后渲染下一帧 理论fps = 1000ms / rtime(ms)
                                [this] {     // 队列有新任务或者线程结束提前结束等待
                                    return !render_requests_pre_.empty()
                                    || !daemon_running_.load(std::memory_order_acquire);
                                }
                            );
                        }
                        // 今日无事可做
                        else {
                            cv_.wait(
                                lock,             // 睡大觉（）无限等待；wait期间自动释放queue_mutex_，唤醒后重新加锁
                                [this] {     // 队列有新任务或者线程结束等待
                                return !render_requests_pre_.empty()
                                    || !daemon_running_.load(std::memory_order_acquire);
                            });
                        }
                        // 消费 pre 队列 → now 列表
                        // 双缓冲 将主线程提交的请求转移到当前处理列表 在渲染时不占有队列锁时间
                        while (!render_requests_pre_.empty()) {
                            render_requests_now_.push_back(
                                std::move(render_requests_pre_.front()));
                            render_requests_pre_.pop();
                        }
                    } // queue_mutex_ 作用域

                    // 线程结束信号发出 释放cv_后 退出守护循环
                    if (!daemon_running_.load(std::memory_order_acquire)) break;

                    // ---- 处理渲染队列的请求 ----
                    if (!render_requests_now_.empty()) {
                        renderService();
                        render_requests_now_.clear();
                    }

                    // ---- 渲染新页面 ----
                    // 绘制时持有页面锁，与 App 线程的页面修改互斥
                    if (current_page_ != nullptr) {
                        std::lock_guard lock(current_page_->state_mutex);
                        renderPage(current_page_);
                    }
                }
            }
        };
}

#endif //APSISUI2_UIRENDER_H
