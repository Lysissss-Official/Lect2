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
#include "UITransor.h"
#include "lcore/Log.h"

namespace lui {

    class Render {
    public:
        struct ClipRect {
            int32_t x1 = 0;
            int32_t y1 = 0;
            int32_t x2 = 0;
            int32_t y2 = 0;

            bool empty() const {
                return x2 <= x1 || y2 <= y1;
            }

            static ClipRect unite(const ClipRect& a, const ClipRect& b) {
                if (a.empty()) return b;
                if (b.empty()) return a;

                return {
                    std::min(a.x1, b.x1),
                    std::min(a.y1, b.y1),
                    std::max(a.x2, b.x2),
                    std::max(a.y2, b.y2)
                };
            }

            static ClipRect intersect(const ClipRect& a, const ClipRect& b) {
                ClipRect result = {
                    std::max(a.x1, b.x1),
                    std::max(a.y1, b.y1),
                    std::min(a.x2, b.x2),
                    std::min(a.y2, b.y2)
                };

                if (result.empty()) {
                    return {};
                }

                return result;
            }
        };

        // 绘制函数必备上下文参数
        struct DrawContext {
            strc::BasicItem& target;
            TranslatorService& transor;
            ClipRect clip;
            float progress = 1.0f;
        };

        // 绘制函数传参表
        using DrawFunction = void (*)(
            DrawContext& context,
            void* extra_data
        );

    protected:
        TranslatorService* transor_ = nullptr;

        struct RenderRequest {
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now();
            std::chrono::time_point<std::chrono::steady_clock> time_end   = std::chrono::steady_clock::now();

            strc::BasicItem* target_ptr;
            std::optional<strc::ParamIndex> target_param;

            int32_t value_start = 0;
            int32_t value_end = 0;

            // 绘制函数
            DrawFunction draw_func = nullptr;
            // 文字、图片、公式树等额外数据
            void* draw_ex_data = nullptr;

            // 时间函数
            std::function<float(float)> time_func = [](float x){ return 1.0f; };

            bool is_rf = true;
            bool rendered_once = false;
            bool final_rendered = false;
        };

        // Event堆的比较函数
        struct CmpByTimeEnd {
            bool operator()(const RenderRequest& a, const RenderRequest& b) const {
                return a.time_end > b.time_end;
            }
        };

        std::vector<RenderRequest> render_queue_;

        void popExpired() {
            auto now = std::chrono::steady_clock::now();
            while (!render_queue_.empty() && render_queue_.front().time_end <= now) {
                const auto& req_0 = render_queue_.front();
                const bool is_single_frame = req_0.time_start == req_0.time_end;
                const bool can_remove = is_single_frame? req_0.rendered_once: (req_0.time_end <= now && req_0.final_rendered);

                if (!can_remove) {
                    break;
                }

                std::pop_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
                render_queue_.pop_back();
            }
        }

        strc::Page* current_page_ = nullptr;

    public:
        Render() = default;

        explicit Render(TranslatorService* transor)
            : transor_(transor) {}

        void setTransor(TranslatorService* transor) {
            transor_ = transor;
        }

        // 兼容接口
        void requestReDraw(strc::BasicItem* target_ptr,
            DrawFunction draw_func = nullptr,
            void* draw_ex_data = nullptr,
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now(),
            bool is_rf = true)
        {
            if (!target_ptr) {
                return;
            }

            RenderRequest req;
            req.target_ptr     = target_ptr;

            req.draw_func = draw_func;
            req.draw_ex_data = draw_ex_data;

            req.time_start     = time_start;
            req.time_end       = time_start;

            req.is_rf          = is_rf;

            {
                std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
                render_queue_.push_back(std::move(req));
                std::push_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
            }
            cv_.notify_one();
        }

        // 支持动画的新接口
        void requestAnimate(
            strc::BasicItem* target_ptr,
            strc::ParamIndex target_param,
            int32_t value_end,
            DrawFunction draw_func = nullptr,
            void* draw_ex_data = nullptr,
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now(),
            std::chrono::duration<float, std::milli> time_use = std::chrono::duration<float, std::milli>(0),
            std::function<float(float)> animation_func = [](float x) { return 1.0f; },
            bool is_rf = true)
        {
            if (!target_ptr) {
                return;
            }

            RenderRequest req;

            req.target_ptr = target_ptr;
            req.target_param = target_param;

            req.value_start =target_ptr->getParam(target_param);
            req.value_end = value_end;

            req.draw_func = draw_func;
            req.draw_ex_data = draw_ex_data;

            req.time_start = time_start;
            req.time_end =time_start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(time_use);
            req.time_func = std::move(animation_func);

            req.is_rf = is_rf;

            {
                std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
                render_queue_.push_back(std::move(req));
                std::push_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
            }

            cv_.notify_one();
        }

        void setCurrentPage(strc::Page* page) {
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
                    std::lock_guard page_lock(current_page_->page_mutex);
                    std::lock_guard queue_lock(queue_mutex_);

                    const auto now = steady_clock::now();

                    for (auto& req : render_queue_) {
                        // 尚未开始判断
                        if (now < req.time_start) {
                            continue;
                        }

                        // 单帧刷新判断
                        const bool single_frame =
                            req.time_start == req.time_end;

                        // 真实进程占比 (0~1)
                        float raw_progress = 1.0f;

                        if (!single_frame && now < req.time_end) {
                            raw_progress =
                                duration<float>(now - req.time_start).count() /
                                duration<float>(
                                    req.time_end - req.time_start
                                ).count();

                            raw_progress =
                                std::clamp(raw_progress, 0.0f, 1.0f);
                        }

                        const float progress =
                            req.time_func(raw_progress);

                        // 可选的参数变化
                        if (req.target_param.has_value()) {
                            const int64_t delta =
                                static_cast<int64_t>(req.value_end) -
                                req.value_start;

                            const int32_t value =
                                req.value_start +
                                static_cast<int32_t>(
                                    static_cast<float>(delta) * progress
                                );

                            req.target_ptr->getParam(req.target_param.value()) = value;
                        }

                        // 可选的 Theme/自绘函数
                        if (req.draw_func && transor_) {
                            DrawContext context {
                                .target = *req.target_ptr,
                                .transor = *transor_,
                                .clip = {
                                    req.target_ptr->getParam(
                                        strc::ParamIndex::phys_x1
                                    ),
                                    req.target_ptr->getParam(
                                        strc::ParamIndex::phys_y1
                                    ),
                                    req.target_ptr->getParam(
                                        strc::ParamIndex::phys_x2
                                    ),
                                    req.target_ptr->getParam(
                                        strc::ParamIndex::phys_y2
                                    )
                                },
                                .progress = progress
                            };

                            req.draw_func(
                                context,
                                req.draw_ex_data
                            );
                        }

                        // 生命周期标记
                        if (single_frame) {
                            req.rendered_once = true;
                        }
                        else if (now >= req.time_end) {
                            req.final_rendered = true;
                        }
                    }
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
