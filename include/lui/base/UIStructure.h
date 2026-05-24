//
// Created by Shoiin / archeart on 2026/5/19.
//
// LUI 核心结构定义 — 五层 UI 层级体系
// ============================================================================
// 层级从低到高：
//   lui::Element    — 最小 UI 单元（文本框 / 按钮 / 列表 / 空）
//   lui::Block      — 元素容器，一组相关元素
//   lui::Page       — 区块容器，包含滚动、焦点导航、页高切换
//   lui::Screen     — 显示目标（宽高 + 色彩模式）
//   （Application 独立为 lapp 命名空间，见 include/lapp/Application.h）
//
// 坐标系：
//   逻辑坐标 (logic_*) — 百分比 0.0–100.0，与分辨率无关
//   物理坐标 (phys_*)  — 像素值，由 updatePhysical() 根据屏幕分辨率计算
//
// ID 体系：
//   每个层级实例都会通过 IDGenerator<Tag> 获得唯一 32 位 ID，
//   析构时自动回收。Tag 模板参数确保不同层级的 ID 空间隔离。
// ============================================================================

#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H

#include <cstdint>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace lui {

    // =========================================================================
    // IDGenerator<Tag> — 自动唯一 ID 生成器
    // =========================================================================
    // 每个 Tag 类型拥有独立的 ID 注册表（通过静态局部变量实现），
    // 保证不同层级（Element / Block / Page / Screen / Application）的 ID 空间隔离。
    //
    // 生成策略：
    //   1. 先取一个随机 32 位种子（std::random_device → mt19937）
    //   2. 若与已有 ID 碰撞，则线性递增探测下一个可用 ID
    //   3. ID=0 保留不用，递增到 0 时跳回 1
    //
    // release() 在对象析构时归还 ID，可被后续生成复用。
    // =========================================================================
    template<typename Tag>
    class IDGenerator {
    private:
        // 每个 Tag 实例化一份独立 registry（静态局部变量）
        static std::set<uint32_t>& registry() {
            static std::set<uint32_t> s;
            return s;
        }

    public:
        static uint32_t generate() {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);

            auto& used = registry();
            uint32_t id = dist(rng);
            while (used.count(id)) {
                id++;
                if (id == 0) id = 1;
            }
            used.insert(id);
            return id;
        }

        static void release(uint32_t id) {
            registry().erase(id);
        }
    };

    // =========================================================================
    // lui::ele — Element 层（最小 UI 单元）
    // =========================================================================

        // 元素类型枚举
        enum ElementType {
            ELE_TEXTBOX = 0,   // 文本框 — 纯文本显示
            ELE_BUTTON,        // 按钮   — 可交互控件
            ELE_LIST,          // 列表   — 可滚动选择列表
            ELE_EMPTY          // 空占位 — 调试 / 布局占位
        };

        enum ElementMovement {
            ELE_FLOAT = 0,    // 浮动   — 根据页面滚动
            ELE_FIXED         // 固定   — 占据固定位置 层级大于FLOAT
        };

        enum ElementAccess {
            ELE_REAL = 0,    // 真实块   — 可聚焦
            ELE_FAKE         // 虚假块   — 不可聚焦
        };

        // Element 是 LUI 层级中的最小渲染与交互单元。
        // 每个 Element 持有：
        //   - 相对于 Screen 的逻辑坐标（百分比）与物理坐标（像素）
        //   - 焦点导航四向指针（由 Page::recalculateFocus() 自动计算）
        //   - 内容字符串 content（供渲染器直接使用）
        class Element {
        private:
            uint32_t uni_id;  // 全局唯一 ID

        public:
            ElementType type;
            ElementMovement movement;
            ElementAccess access;

            // 逻辑坐标：百分比 0.0–100.0，独立于屏幕分辨率
            float logic_x1, logic_y1;
            float logic_x2, logic_y2;

            // 物理坐标：像素值，由 updatePhysical() 根据屏幕宽高换算
            uint16_t phys_x1, phys_y1;
            uint16_t phys_x2, phys_y2;

            // 焦点导航 — 四向邻居指针。
            // 由 Page::recalculateFocus() 根据逻辑坐标的几何关系自动填充。
            // 若某方向无合适邻居则为 nullptr。
            Element* focus_up    = nullptr;
            Element* focus_down  = nullptr;
            Element* focus_left  = nullptr;
            Element* focus_right = nullptr;

            bool focused = false;      // 当前是否获得焦点
            std::string content;       // 元素文本内容

            Element() : type(ELE_EMPTY), movement(ELE_FLOAT), access(ELE_REAL), logic_x1(0), logic_y1(0), logic_x2(0), logic_y2(0),
                        phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0) {
                uni_id = IDGenerator<Element>::generate();
            }

            ~Element() { IDGenerator<Element>::release(uni_id); }

            uint32_t getID() const {
                return uni_id;
            }

            // 将逻辑坐标（百分比）转换为物理坐标（像素）
            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                phys_x1 = static_cast<uint16_t>(logic_x1 / 100.0f * screen_width);
                phys_y1 = static_cast<uint16_t>(logic_y1 / 100.0f * screen_height);
                phys_x2 = static_cast<uint16_t>(logic_x2 / 100.0f * screen_width);
                phys_y2 = static_cast<uint16_t>(logic_y2 / 100.0f * screen_height);
            }

            // 判断逻辑坐标点 (x, y) 是否落在本元素内
            bool containsLogical(float x, float y) const {
                return x >= logic_x1 && x <= logic_x2 && y >= logic_y1 && y <= logic_y2;
            }

            // 判断两元素在 X 轴投影是否重叠（用于焦点导航时计算纵向邻居）
            bool overlapsHorizontally(const Element& other) const {
                return logic_x1 < other.logic_x2 && logic_x2 > other.logic_x1;
            }

            // 判断两元素在 Y 轴投影是否重叠（用于焦点导航时计算横向邻居）
            bool overlapsVertically(const Element& other) const {
                return logic_y1 < other.logic_y2 && logic_y2 > other.logic_y1;
            }
        };

    // =========================================================================
    // lui::blk — Block 层（元素容器）
    // =========================================================================

        // Block 将一组语义相关的 Element 组织在一起。
        // 典型用法：一个 Block 对应页面上的一个功能区（页眉、按钮组、数据面板等）。
        //
        // Block 维护 focused_element_index 以追踪内部聚焦元素。
        // 同时持有逻辑/物理坐标以支持流式布局。
        class Block {
        private:
            uint32_t uni_id;

        public:
            // 逻辑坐标（百分比）
            float logic_x1, logic_y1;
            float logic_x2, logic_y2;

            // 物理坐标（像素，由 updatePhysical 计算）
            uint16_t phys_x1, phys_y1;
            uint16_t phys_x2, phys_y2;

            std::vector<Element> elements;     // 子元素集合
            size_t focused_element_index = 0;       // 当前聚焦元素在 elements 中的索引

            Block() : logic_x1(0), logic_y1(0), logic_x2(0), logic_y2(0),
                      phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0) {
                uni_id = IDGenerator<Block>::generate();
            }

            ~Block() { IDGenerator<Block>::release(uni_id); }

            uint32_t getID() const {
                return uni_id;
            }

            // 递归更新自身及所有子元素的物理坐标
            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                phys_x1 = static_cast<uint16_t>(logic_x1 / 100.0f * screen_width);
                phys_y1 = static_cast<uint16_t>(logic_y1 / 100.0f * screen_height);
                phys_x2 = static_cast<uint16_t>(logic_x2 / 100.0f * screen_width);
                phys_y2 = static_cast<uint16_t>(logic_y2 / 100.0f * screen_height);
                for (auto& el : elements) {
                    el.updatePhysical(screen_width, screen_height);
                }
            }

            // 获取当前聚焦的元素指针，若无则返回 nullptr
            Element* getFocusedElement() {
                if (focused_element_index < elements.size())
                    return &elements[focused_element_index];
                return nullptr;
            }
        };

    // =========================================================================
    // lui::pge — Page 层（区块容器 + 焦点导航 + 滚动）
    // =========================================================================

        // 页高模式
        //   PAGE_FULL — 全屏页，状态栏较窄
        //   PAGE_HALF — 半屏页，状态栏加宽（用于弹出 / 分屏场景）
        enum PageHeight {
            PAGE_FULL = 0,
            PAGE_HALF
        };

        // Page 是渲染和焦点管理的核心。
        //
        // 职责：
        //   1. 持有 Block 集合
        //   2. 管理焦点元素（current_focused），维护四向导航图
        //   3. 支持垂直滚动：跟踪 scroll_y / max_scroll / viewport_h
        //   4. 自动滚动至焦点元素可见（scrollToShow）
        class Page {
        private:
            uint32_t uni_id;

            // 状态栏高度常量（像素）
            static constexpr uint16_t STATUS_BAR_FULL = 24;
            static constexpr uint16_t STATUS_BAR_HALF = 48;
            static constexpr uint16_t SCROLL_MARGIN   = 12;  // 滚动时留白边距

        public:
            PageHeight height_mode = PAGE_FULL;
            uint16_t status_bar_thickness = STATUS_BAR_FULL;

            std::vector<Block> blocks;          // 子区块集合
            Element* current_focused = nullptr;  // 当前获得焦点的元素

            // 滚动状态（均为像素单位）
            float    scroll_y    = 0.0f;   // 当前滚动偏移
            uint16_t viewport_h  = 0;      // 视口高度（= 屏幕高度）
            float    max_scroll  = 0.0f;   // 最大可滚动量

            // 页面状态互斥锁（页面锁）
            // App 线程修改页面状态（移动焦点等）和 renderd 线程绘制页面
            // 都会争用此锁。所有 public 写方法内部自动加锁，App 无需手动处理。
            mutable std::recursive_mutex state_mutex;

            Page() { uni_id = IDGenerator<Page>::generate(); }
            ~Page() { IDGenerator<Page>::release(uni_id); }

            uint32_t getID() const {
                return uni_id;
            }

            // 切换全屏 / 半屏模式，自动调整状态栏厚度
            void switchHeight() {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                height_mode = (height_mode == PAGE_FULL) ? PAGE_HALF : PAGE_FULL;
                status_bar_thickness = (height_mode == PAGE_FULL) ? STATUS_BAR_FULL : STATUS_BAR_HALF;
            }

            // 更新所有区块物理坐标，重算滚动范围和焦点图
            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                viewport_h = screen_height;
                for (auto& blk : blocks) {
                    blk.updatePhysical(screen_width, screen_height);
                }
                recomputeMaxScroll();
                recalculateFocus();
            }

            // 遍历所有元素，取 phys_y2 最大值，计算可滚动范围
            void recomputeMaxScroll() {
                float bottom = static_cast<float>(viewport_h);
                for (auto& blk : blocks) {
                    if (static_cast<float>(blk.phys_y2) > bottom)
                        bottom = static_cast<float>(blk.phys_y2);
                    for (auto& el : blk.elements) {
                        if (static_cast<float>(el.phys_y2) > bottom)
                            bottom = static_cast<float>(el.phys_y2);
                    }
                }
                // 可滚动量 = 内容总高 - 视口高 + 状态栏厚度
                max_scroll = bottom - viewport_h + status_bar_thickness;
                if (max_scroll < 0.0f) max_scroll = 0.0f;
                if (scroll_y > max_scroll) scroll_y = max_scroll;
                if (scroll_y < 0.0f) scroll_y = 0.0f;
            }

            // 自动滚动使 el 在视口内可见（保留 SCROLL_MARGIN 边距）
            void scrollToShow(Element* el) {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (!el) return;

                // 元素当前屏幕坐标（已减去滚动偏移）
                float screen_y1 = static_cast<float>(el->phys_y1) - scroll_y;
                float screen_y2 = static_cast<float>(el->phys_y2) - scroll_y;

                float top_margin    = static_cast<float>(status_bar_thickness) + SCROLL_MARGIN;
                float bottom_margin = static_cast<float>(viewport_h) - SCROLL_MARGIN;

                // 元素上端被状态栏遮挡 → 向下滚动
                if (screen_y1 < top_margin) {
                    scroll_y -= (top_margin - screen_y1);
                }
                // 元素下端超出视口底部 → 向上滚动
                else if (screen_y2 > bottom_margin) {
                    scroll_y += (screen_y2 - bottom_margin);
                }

                // 钳位滚动范围
                if (scroll_y < 0.0f) scroll_y = 0.0f;
                if (scroll_y > max_scroll) scroll_y = max_scroll;
            }

            // 重建焦点导航图
            // -----------------------------------------------------------------
            // 对页面内所有 Element 两两比较，按几何关系填充四向邻居指针：
            //
            //   focus_right — Y 轴投影重叠 且 b 在 a 右侧，取距离最近者
            //   focus_left  — Y 轴投影重叠 且 b 在 a 左侧，取距离最近者
            //   focus_down  — X 轴投影重叠 且 b 在 a 下方，取距离最近者
            //   focus_up    — X 轴投影重叠 且 b 在 a 上方，取距离最近者
            //
            // 若 current_focused 为空（首次加载），自动聚焦页面第一个元素。
            // -----------------------------------------------------------------
            void recalculateFocus() {
                // 收集所有元素，清空旧邻居指针
                std::vector<Element*> all;
                for (auto& blk : blocks) {
                    for (auto& el : blk.elements) {
                        el.focus_up    = nullptr;
                        el.focus_down  = nullptr;
                        el.focus_left  = nullptr;
                        el.focus_right = nullptr;
                        if (el.access == ELE_FAKE)
                            continue;
                        all.push_back(&el);
                    }
                }

                // O(n²) 两两比较 — 每个方向的邻居取几何距离最小者
                for (size_t i = 0; i < all.size(); ++i) {
                    Element* a = all[i];
                    float best_right = 1e9f, best_left = 1e9f;
                    float best_up = 1e9f, best_down = 1e9f;

                    for (size_t j = 0; j < all.size(); ++j) {
                        if (i == j) continue;
                        Element* b = all[j];

                        // 右邻居：b 的左边在 a 右边之右，且 Y 投影重叠
                        if (b->logic_x1 >= a->logic_x2 && a->overlapsVertically(*b)) {
                            float dist = b->logic_x1 - a->logic_x2;
                            if (dist < best_right) { best_right = dist; a->focus_right = b; }
                        }
                        // 左邻居：b 的右边在 a 左边之左，且 Y 投影重叠
                        if (b->logic_x2 <= a->logic_x1 && a->overlapsVertically(*b)) {
                            float dist = a->logic_x1 - b->logic_x2;
                            if (dist < best_left) { best_left = dist; a->focus_left = b; }
                        }
                        // 下邻居：b 的上边在 a 下边之下，且 X 投影重叠
                        if (b->logic_y1 >= a->logic_y2 && a->overlapsHorizontally(*b)) {
                            float dist = b->logic_y1 - a->logic_y2;
                            if (dist < best_down) { best_down = dist; a->focus_down = b; }
                        }
                        // 上邻居：b 的下边在 a 上边之上，且 X 投影重叠
                        if (b->logic_y2 <= a->logic_y1 && a->overlapsHorizontally(*b)) {
                            float dist = a->logic_y1 - b->logic_y2;
                            if (dist < best_up) { best_up = dist; a->focus_up = b; }
                        }
                    }
                }

                // 无焦点或者焦点无效
                if (!current_focused || current_focused->access== ELE_FAKE) {
                    current_focused = nullptr;
                    if (!all.empty())
                        setFocus(all.front());
                }
            }

            // 设置焦点至指定元素（取消旧焦点，设置新焦点，并自动滚动至可见）
            void setFocus(Element* el) {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (current_focused) current_focused->focused = false;
                current_focused = el;
                if (current_focused) {
                    current_focused->focused = true;
                    scrollToShow(current_focused);
                }
            }

            // 四向焦点移动（沿预先计算的邻居指针跳转）
            // 四向焦点移动。
            // 内部自动加锁保护页面状态，与 renderd 的逐帧绘制互斥。
            bool moveFocusRight() {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (!current_focused || !current_focused->focus_right) return false;
                setFocus(current_focused->focus_right);
                return true;
            }
            bool moveFocusLeft() {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (!current_focused || !current_focused->focus_left) return false;
                setFocus(current_focused->focus_left);
                return true;
            }
            bool moveFocusUp() {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (!current_focused || !current_focused->focus_up) return false;
                setFocus(current_focused->focus_up);
                return true;
            }
            bool moveFocusDown() {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (!current_focused || !current_focused->focus_down) return false;
                setFocus(current_focused->focus_down);
                return true;
            }
        };

    // =========================================================================
    // lui::scr — Screen 层（显示目标描述）
    // =========================================================================

        // 屏幕色彩模式
        //   SCREEN_BW   — 黑白（1 bit / pixel）
        //   SCREEN_GRAY — 灰度（常用于墨水屏）
        //   SCREEN_RGB  — 全彩
        enum ColorMode {
            SCREEN_BW   = 0,
            SCREEN_GRAY = 1,
            SCREEN_RGB  = 2
        };

        // Screen 描述目标显示器的物理属性和色彩能力。
        // 当前主要用于存储宽高和色彩模式，供 Page::updatePhysical() 换算坐标。
        class Screen {
        private:
            uint32_t uni_id;

        public:
            uint16_t width  = 0;
            uint16_t height = 0;
            ColorMode color_mode = SCREEN_RGB;

            Screen()          { uni_id = IDGenerator<Screen>::generate(); }
            ~Screen()         { IDGenerator<Screen>::release(uni_id); }
            uint32_t getID() const { return uni_id; }

            // 初始化显示设备（平台相关实现，定义在 src/screen.cpp）
            void init();

            // 关闭显示设备
            void close();
        };
}

#endif //APSISUI2_UISTRUCTURE_H
