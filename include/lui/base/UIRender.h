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
#include <unordered_map>

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

            uint32_t generation = 0;
        };

        // Event堆的比较函数
        struct CmpByTimeEnd {
            bool operator()(const RenderRequest& a, const RenderRequest& b) const {
                return a.time_end > b.time_end;
            }
        };

        // 已准备/进行处理的渲染请求队列
        std::vector<RenderRequest> rendering_queue_;

        // 未处理的渲染请求队列
        // 注：在渲染过程中锁 queue_mutex 长期被占用 中途加入的请求试图加入导致锁竞争
        //    取消锁限制会导致 vector 同读同写等 UB 产生!
        std::vector<RenderRequest> pending_rendering_queue_;

        // pending_rendering_queue_ 的保护锁
        std::recursive_mutex pending_queue_mutex_;

        // 对象-参数对
        struct AnimationKey {
            strc::BasicItem* target;
            strc::ParamIndex param;

            bool operator==(const AnimationKey& other) const {
                return target == other.target
                    && param == other.param;
            }
        };

        // 对象-参数对的哈希函数
        struct AnimationKeyHash {
            size_t operator()(const AnimationKey& key) const noexcept {
                const size_t h1 =
                    std::hash<strc::BasicItem*>{}(key.target);

                const size_t h2 =
                    std::hash<uint16_t>{}(
                        static_cast<uint16_t>(key.param)
                    );

                return h1 ^ (h2 << 1);
            }
        };

        // 请求代际更新表
        std::unordered_map<
            AnimationKey,
            uint32_t,
            AnimationKeyHash
        > animation_generations_;

        // animation_generations_ 的保护锁
        std::recursive_mutex animation_gen_mutex_;

        // 清除过期请求
        void popExpired() {
            std::lock_guard<std::recursive_mutex> lock(queue_mutex_);

            auto now = std::chrono::steady_clock::now();

            while (!rendering_queue_.empty() && rendering_queue_.front().time_end <= now) {

                const auto& req_0 = rendering_queue_.front();
                const bool is_single_frame = req_0.time_start == req_0.time_end;
                const bool can_remove = is_single_frame? req_0.rendered_once: (req_0.time_end <= now && req_0.final_rendered);

                if (!can_remove) {
                    break;
                }

                std::pop_heap(
                    rendering_queue_.begin(),
                    rendering_queue_.end(),
                    CmpByTimeEnd{}
                );

                rendering_queue_.pop_back();
            }
        }

        // 加入新请求
        void collectPending() {
            std::vector<RenderRequest> pending_q_temp;

            {
                std::lock_guard<std::recursive_mutex> lock(pending_queue_mutex_);
                pending_q_temp.swap(pending_rendering_queue_);
            }

            {
                std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
                for (auto& pending_req : pending_q_temp) {
                    rendering_queue_.push_back(std::move(pending_req));

                    std::push_heap(
                        rendering_queue_.begin(),
                        rendering_queue_.end(),
                        CmpByTimeEnd{}
                    );
                }
            }
        }

        // 当前渲染页面
        strc::Page* current_page_ = nullptr;

        ClipRect screen_clip_ = {
            0, 0, 0, 0
        };

        // 递归绘制
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
                .clip =
                    ClipRect::intersect(
                    ClipRect::intersect(current_clip,parent_clip),
                    screen_clip_
                ),
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

        explicit Render(ldevice::Screen* screen) {
            setScreen(screen);
        }

        void setScreen(ldevice::Screen* screen) {
            // TODO: screen_clip_/screen_加锁 可能的多线程同读写！（ETA：二次更新增加多屏幕/分屏支持时）
            screen_ = screen;

            if (screen_) {
                screen_clip_ = {
                    0,
                    0,
                    static_cast<int32_t>(screen_->getWidth()),
                    static_cast<int32_t>(screen_->getHeight())
                };
            }
            else {
                screen_clip_ = {
                    0, 0, 0, 0
                };
            }
        }

        void setTheme(lui::theme::Theme* theme) {
            // TODO: theme_加锁 可能的多线程同读写！（ETA：二次更新增加多屏幕/分屏支持时）
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
                // 加入缓冲队列
                std::lock_guard<std::recursive_mutex> lock(pending_queue_mutex_);
                pending_rendering_queue_.push_back(std::move(req));
                //std::push_heap(rendering_queue_.begin(), rendering_queue_.end(), CmpByTimeEnd{});
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
                // 更新代际表
                std::lock_guard<std::recursive_mutex> lock(animation_gen_mutex_);
                req.generation = ++animation_generations_[{target_ptr, target_param}];
            }

            {
                // 加入缓冲队列
                std::lock_guard<std::recursive_mutex> lock(pending_queue_mutex_);
                pending_rendering_queue_.push_back(std::move(req));
                //std::push_heap(pending_rendering_queue_.begin(), pending_rendering_queue_.end(), CmpByTimeEnd{});
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

    protected:
        std::recursive_mutex queue_mutex_;
        std::condition_variable_any cv_;
        std::thread daemon_thread_;
        std::atomic<bool> daemon_running_{false};

        void daemonLoop() {
            using namespace std::chrono;

            while (daemon_running_.load(std::memory_order_acquire)) {

                // 增减渲染队列新旧请求
                collectPending();
                popExpired();

                // 渲染队列为空挂起
                if (rendering_queue_.empty()) {
                    {
                        std::unique_lock<std::recursive_mutex> lock(pending_queue_mutex_);
                        cv_.wait(lock, [this] {
                            return !rendering_queue_.empty()
                                || !pending_rendering_queue_.empty()
                                || !daemon_running_.load(std::memory_order_acquire);
                        });
                    }

                    collectPending();
                    popExpired();
                }
                if (!daemon_running_.load(std::memory_order_acquire)) break;

                if (current_page_) {
                    std::lock_guard page_lock(current_page_->page_mutex);
                    std::lock_guard queue_lock(queue_mutex_);

                    const auto now = steady_clock::now();

                    //
                    static auto last_render = steady_clock::now();

                    const auto render_now = steady_clock::now();

                    const auto dt =
                        duration_cast<milliseconds>(
                            render_now - last_render
                        ).count();

                    if (dt >= 0) {
                        LOG(
                            "Render loop tick dt=" +
                            std::to_string(dt) +
                            "ms"
                        );

                        last_render = render_now;
                    }
                    //

                    /*
                    LOG(
                        "RR QUEUE size=" +
                        std::to_string(rendering_queue_.size())
                    );
                    */

                    const auto render_start = steady_clock::now();

                    for (auto& req : rendering_queue_) {
                        /*
                        LOG(
                            "RR: element=" +
                            std::to_string(reinterpret_cast<uintptr_t>(req.target_ptr)) +
                            " param=" +
                            (
                                req.target_param
                                    ? std::to_string(static_cast<uint16_t>(*req.target_param))
                                    : "none"
                            ) +
                            " gen=" +
                            std::to_string(req.generation) +
                            " start=" +
                            std::to_string(req.value_start) +
                            " end=" +
                            std::to_string(req.value_end)
                        );
                        */
                        // 空指针保护
                        if (!req.target_ptr) {
                            continue;
                        }


                        // 尚未开始判断
                        if (now < req.time_start) {
                            continue;
                        }


                        // 代际覆盖机制
                        // it -> first = key = {target_ptr, target_param}
                        // it -> second = generation
                        if (req.target_param) /* req.target_param 是 std::optional! */ {
                            const AnimationKey key {
                                req.target_ptr,
                                *req.target_param
                            };

                            std::lock_guard<std::recursive_mutex> lock(animation_gen_mutex_);

                            const auto it = animation_generations_.find(key);

                            // 被更新的动画取代
                            if (it != animation_generations_.end() && req.generation != it->second) {
                                /*
                                LOG(
                                    "RR: SKIP stale element=" +
                                    std::to_string(reinterpret_cast<uintptr_t>(req.target_ptr)) +
                                    " req_gen=" +
                                    std::to_string(req.generation) +
                                    " current_gen=" +
                                    std::to_string(it->second)
                                );
                                */
                                // 懒惰失效
                                req.final_rendered = true;
                                continue;
                            }
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

                            /*
                            LOG(
                                "RR: RENDER element=" +
                                std::to_string(reinterpret_cast<uintptr_t>(req.target_ptr)) +
                                " gen=" +
                                std::to_string(req.generation) +
                                " progress=" +
                                std::to_string(progress) +
                                " value=" +
                                std::to_string(value)
                            );
                            */

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
                                .clip =
                                    ClipRect::intersect(
                                        ClipRect::intersect(current_clip,parent_clip),
                                        screen_clip_
                                    ),
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

                    const auto render_end = steady_clock::now();

                    //
                    LOG(
                        "render=" +
                        std::to_string(
                        duration<float, std::milli>(
                            render_end - render_start
                            ).count()
                        ) +
                        "ms"
                    );
                    //
                }

                {
                    std::unique_lock<std::recursive_mutex> lock(queue_mutex_);
                    popExpired();
                    if (!rendering_queue_.empty()
                        && daemon_running_.load(std::memory_order_acquire)) {
                        auto nearest = rendering_queue_.front().time_end;
                        auto now = steady_clock::now();
                        if (nearest > now) {
                            auto wait = duration_cast<milliseconds>(nearest - now);

                            if (wait > milliseconds(1))
                                wait = milliseconds(1); // USE 1ms instead of 8ms.

                            //const auto wait_start = steady_clock::now();
                            cv_.wait_for(lock, wait);// Ohh, ITS BAD. ITS JUST BAD. We have a 22ms when waiting for 8ms.
                            //const auto wait_end = steady_clock::now();

                            /*
                            LOG(
                                "wait=" +
                                std::to_string(
                                    duration<float, std::milli>(
                                        wait_end - wait_start
                                    ).count()
                                ) +
                                "ms"
                            );
                            */
                        }
                    }
                }
            }
        }
    };
}

#endif //APSISUI2_UIRENDER_H
