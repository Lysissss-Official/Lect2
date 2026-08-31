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
#include "UITheme.h"
#include "lcore/Log.h"

namespace lui {

    class Render {
    public:
        //

    protected:
        ldevice::Screen* screen_ = nullptr;
        lui::theme::Theme* theme_ = nullptr;

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
            // void* draw_ex_data = nullptr;
            // (已经由 lui::strc 下的 OptionalParam 系统代替)

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

        void drawRecursive(strc::BasicItem* item, ClipRect parent_clip) {
            if (!item) {
                return;
            }

            ClipRect current_clip = {
                abs2Phys(item->getParam(strc::ParamIndex::abs_x1), screen_->getWidth()),
                abs2Phys(item->getParam(strc::ParamIndex::abs_y1), screen_->getWidth()),
                abs2Phys(item->getParam(strc::ParamIndex::abs_x2), screen_->getWidth()),
                abs2Phys(item->getParam(strc::ParamIndex::abs_y2), screen_->getWidth())
            };

            DrawContext context {
                .target = *item,
                .screen = *screen_,
                .clip = ClipRect::intersect(current_clip, parent_clip),
                .progress = 1.0f
            };

            if (theme_) {
                theme_->drawFuncCall(context);
            }

            for (const auto& child : item->getChildren()) {
                drawRecursive(child.get(), current_clip);
            }
        }

    public:
        Render() = default;

        explicit Render(ldevice::Screen* screen)
            : screen_(screen) {}

        void setScreen(ldevice::Screen* screen) {
            screen_ = screen;
        }

        void setTheme(lui::theme::Theme* theme) {
            theme_ = theme;
        }

        // 兼容接口
        void requestReDraw(strc::BasicItem* target_ptr,
            DrawFunction draw_func = nullptr,
            //void* draw_ex_data = nullptr,
            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now(),
            bool is_rf = true)
        {
            if (!target_ptr) {
                return;
            }

            RenderRequest req;
            req.target_ptr     = target_ptr;

            req.draw_func = draw_func;
            //req.draw_ex_data = draw_ex_data;

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
            // --- 必选参数 ---
            strc::BasicItem* target_ptr,    // 目标对象
            strc::ParamIndex target_param,  // 目标对象的目标修改参数
            int32_t value_end,              // 被修改参数的终值

            // --- 可选参数(留空由Theme自动管理) ---
            std::chrono::duration<float, std::milli> time_use = std::chrono::duration<float, std::milli>(0),
            std::function<float(float)> time_func = nullptr,
            DrawFunction draw_func = nullptr,
            //void* draw_ex_data = nullptr,

            std::chrono::time_point<std::chrono::steady_clock> time_start = std::chrono::steady_clock::now(),

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
            //req.draw_ex_data = draw_ex_data;

            req.time_start = time_start;
            req.time_end =time_start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(time_use);
            req.time_func = std::move(time_func);

            req.is_rf = is_rf;

            {
                std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
                render_queue_.push_back(std::move(req));
                std::push_heap(render_queue_.begin(), render_queue_.end(), CmpByTimeEnd{});
            }

            cv_.notify_one();
        }

        /*void requestAnimate(
            strc::BasicItem* target_ptr,
            strc::ParamIndex target_param,
            int32_t value_end,
            //FunctionSet w,

            bool is_rf = true
        ) {
            // TODO: 加入聚合参数支持
        }*/

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

                    static auto last_render = steady_clock::now();

                    const auto render_now = steady_clock::now();

                    const auto dt =
                        duration_cast<milliseconds>(
                            render_now - last_render
                        ).count();

                    if (dt >= 0) {
                        LOG(
                            "Render tick dt=" +
                            std::to_string(dt) +
                            "ms"
                        );

                        last_render = render_now;
                    }

                    for (auto& req : render_queue_) {
                        if (!req.target_ptr) {
                            continue;
                        }

                        // 尚未开始判断
                        if (now < req.time_start) {
                            continue;
                        }

                        // 单帧刷新判断
                        const bool single_frame =
                            req.time_start == req.time_end;

                        // 真实进程占比计算 (0~1)

                        // 新接口 由Theme管理
                        float progress;
                        if (!req.time_func && theme_) {
                            progress =
                                theme_->timeFuncCall(req.time_start, req.time_end, now);
                        }
                        // 旧接口/自定义接口
                        else if (req.time_func){
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

                            progress =
                                req.time_func(raw_progress);
                        }
                        // 降级
                        else {
                            progress = 1.0f;
                        }


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
                        if (screen_) {
                            // 刷新 abs 坐标
                            if (req.target_ptr->getParent()) {
                                req.target_ptr->updateAbsolute(
                                    req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_x1),
                                    req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_y1)
                                );
                            }
                            else {
                                current_page_->updateAbsolute();
                            }

                            ClipRect current_clip = {
                                abs2Phys(req.target_ptr->getParam(strc::ParamIndex::abs_x1), screen_->getWidth()),
                                abs2Phys(req.target_ptr->getParam(strc::ParamIndex::abs_y1), screen_->getWidth()),
                                abs2Phys(req.target_ptr->getParam(strc::ParamIndex::abs_x2), screen_->getWidth()),
                                abs2Phys(req.target_ptr->getParam(strc::ParamIndex::abs_y2), screen_->getWidth())
                            };

                            ClipRect parent_clip = current_clip;

                            if (req.target_ptr->getParent()) {
                                parent_clip = {
                                    abs2Phys(req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_x1), screen_->getWidth()),
                                    abs2Phys(req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_y1), screen_->getWidth()),
                                    abs2Phys(req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_x2), screen_->getWidth()),
                                    abs2Phys(req.target_ptr->getParent()->getParam(strc::ParamIndex::abs_y2), screen_->getWidth())
                                };
                            }

                            // 绘制所需的上下文内容
                            DrawContext context {
                                .target = *req.target_ptr,
                                .screen = *screen_,
                                .clip = ClipRect::intersect(current_clip,parent_clip),
                                .progress = progress
                            };

                            // 绘制接口调用
                            // 新接口 由Theme管理
                            if (!req.draw_func && theme_) {
                                theme_->drawFuncCall(context);
                                if (req.is_rf) {
                                    for (const auto& child : req.target_ptr->getChildren()) {
                                        drawRecursive(child.get(), current_clip);
                                    }
                                }
                            }
                            // 旧接口/自定义接口
                            else if (req.draw_func) {
                                req.draw_func(context);
                            }
                            // 降级
                            else {
                                // ...?
                            }
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
                            const auto wait_start = steady_clock::now();
                            //cv_.wait_for(lock, wait); Ohh, ITS BAD. ITS JUST BAD. We have a 22ms instead of 8.
                            const auto wait_end = steady_clock::now();
                            LOG(
                                "wait=" +
                                std::to_string(
                                    duration<float, std::milli>(
                                        wait_end - wait_start
                                    ).count()
                                ) +
                                "ms"
                            );
                        }
                    }
                }
            }
        }
    };
}

#endif //APSISUI2_UIRENDER_H
