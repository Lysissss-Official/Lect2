//
// Created by WTGAdmin0 2026/06/03.
//
// ===================
// UIStructure 重置版 2
// ===================
#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H

#include <cstdint>
#include <mutex>
#include <random>
#include <set>
#include <variant>
#include <vector>
#include <array>

namespace lui {
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
}

namespace lui::strc {

    class Element;
    class Page;
    class Screen;

    /* -------------
     * strc_type 定义
     * -------------
     * 最高位
     * P
     * O
     * N
     * M
     * L
     * K
     * J
     * I
     * ----
     * H
     * G
     * F
     * E
     * D 该元素是否可被点击
     * C 该元素（及其子元素）是否显示
     * B 该元素（及其子元素）是否悬浮
     * A 该元素是否可被聚焦
     * 最低位
     */

    enum class ParamIndex : uint16_t {
        logic_x1 = 0,   // 逻辑X1坐标
        logic_y1 = 1,   // 逻辑Y1坐标
        logic_x2 = 2,   // 逻辑X2坐标
        logic_y2 = 3,   // 逻辑Y2坐标
        phys_x1 = 4,    // 物理X1坐标
        phys_y1 = 5,    // 物理Y1坐标
        phys_x2 = 6,    // 物理X2坐标
        phys_y2 = 7,    // 物理Y2坐标
        fg_color = 8,   // 前景颜色
        bg_color = 9,   // 背景颜色
        bdr_color = 10, // 边框颜色
        txt_color = 11, // 文字颜色
        scroll_x = 12,  // X轴方向滚动逻辑量
        scroll_y = 13,  // Y轴方向滚动逻辑量
        strc_type = 15  // 控件类型
    };

    enum class DirecIndex : uint8_t {
        right = 0,
        up = 1,
        left = 2,
        down = 3,
        up_right = 4,
        up_left = 5,
        down_left = 6,
        down_right = 7
    };

    enum class ColorMode : uint8_t {
        SCREEN_BW   = 0,    // 黑白双色
        SCREEN_GRAY = 1,    // 灰度模式
        SCREEN_RGB565  = 2, // 16位色模式
        SCREEN_RGB666 = 3,  // 18位色模式
        SCREEN_RGB888  = 4  // 24位色模式
    };

    class Screen {
    private:
        uint32_t uni_id;
        uint32_t width;
        uint32_t height;
        ColorMode color_mode;
    public:
        Screen() : width(0), height(0), color_mode(ColorMode::SCREEN_RGB565) {
            uni_id = IDGenerator<Screen>::generate();
        }
        virtual ~Screen() {
            IDGenerator<Screen>::release(uni_id);
        }

        void init();
        void close();

        uint32_t getID() const {
            return uni_id;
        }

        uint32_t& getWidth() {return width;}
        const uint32_t& getWidth() const {return width;}

        uint32_t& getHeight() {return height;}
        const uint32_t& getHeight() const {return height;}

        ColorMode& getColorMode() {return color_mode;}
        const ColorMode& getColorMode() const {return color_mode;}

        Screen(const Screen&) = delete;
        Screen& operator=(const Screen&) = delete;
    };

    class BasicItem {
    protected:
        uint32_t uni_id = 0;
        std::vector<int32_t> params;

        BasicItem* parent = nullptr;
        std::vector<BasicItem*> children;

    public:
        virtual ~BasicItem() = default;

        int32_t& getParam(ParamIndex idx) {
            return params[static_cast<size_t>(idx)];
        }
        const int32_t& getParam(ParamIndex idx) const {
            return params[static_cast<size_t>(idx)];
        }
        uint32_t getID() const {
            return uni_id;
        }

        void updatePhysical(
            uint16_t screen_width, uint16_t screen_height, uint32_t coordinate_scale,
            bool recursive = true, int32_t delta_logic_x = 0, int32_t delta_logic_y = 0
        ) {

            if (screen_width > 10000 || screen_height > 10000 || coordinate_scale == 0) {
                return;
            }

            int32_t scroll_x = 0;
            int32_t scroll_y = 0;

            if (parent) {
                scroll_x = parent->getParam(ParamIndex::scroll_x);
                scroll_y = parent->getParam(ParamIndex::scroll_y);
            }

            int32_t base_logic_x = delta_logic_x - scroll_x;
            int32_t base_logic_y = delta_logic_y - scroll_y;

            params[static_cast<size_t>(ParamIndex::phys_x1)] =
                static_cast<int32_t>(
                    (base_logic_x + getParam(ParamIndex::logic_x1)) * screen_width / coordinate_scale
                );

            params[static_cast<size_t>(ParamIndex::phys_y1)] =
                static_cast<int32_t>(
                    (base_logic_y + getParam(ParamIndex::logic_y1)) * screen_height / coordinate_scale
                );

            params[static_cast<size_t>(ParamIndex::phys_x2)] =
                static_cast<int32_t>(
                    (base_logic_x + getParam(ParamIndex::logic_x2)) * screen_width / coordinate_scale
                );

            params[static_cast<size_t>(ParamIndex::phys_y2)] =
                static_cast<int32_t>(
                    (base_logic_y + getParam(ParamIndex::logic_y2)) * screen_height / coordinate_scale
                );

            if (recursive) {
                for (auto& child_el : children) {
                    int32_t dlt_lgc_x_next = base_logic_x + getParam(ParamIndex::logic_x1);
                    int32_t dlt_lgc_y_next = base_logic_y + getParam(ParamIndex::logic_y1);
                    child_el->updatePhysical(screen_width, screen_height, coordinate_scale, recursive, dlt_lgc_x_next, dlt_lgc_y_next);
                }
            }
        }

        BasicItem* getParent() const { return parent; }

        const std::vector<BasicItem*>& getChildren() const {
            return children;
        }

        bool addChild(BasicItem* child) {
            if (!child || child == this || child->parent) {
                return false;
            }

            child->parent = this;
            children.push_back(child);
            return true;
        }
    };

    class Element : public BasicItem {
        friend class Page;
    public:
        bool focused;
        std::vector<Element*> focus_next;

        Element() : focused(false){
            uni_id = IDGenerator<Element>::generate();
            params.resize(16);
            focus_next.resize(8);
        }
        ~Element() override{
            IDGenerator<Element>::release(uni_id);
        }

        bool containsLogical(uint32_t x, uint32_t y) const {
            return x >= getParam(ParamIndex::logic_x1)
                && x <= getParam(ParamIndex::logic_x2)
                && y >= getParam(ParamIndex::logic_y1)
                && y <= getParam(ParamIndex::logic_y2);
        }

        Element(const Element&) = delete;
        Element& operator=(const Element&) = delete;
    };

    class Page : public BasicItem {
    public:
        mutable std::recursive_mutex page_mutex;
        Element* focus;

        Page() : focus(nullptr){
            uni_id = IDGenerator<Page>::generate();
            params.resize(16);
        }
        ~Page() override {
            IDGenerator<Page>::release(uni_id);
        }

        void scrollToShow(Element* el) {
            if (!el) return;

            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            // 元素当前屏幕坐标（已减去滚动偏移）
            int32_t screen_y1 = el->getParam(ParamIndex::logic_y1) - el->parent->getParam(ParamIndex::scroll_y);
            int32_t screen_y2 = el->getParam(ParamIndex::logic_y2) - el->parent->getParam(ParamIndex::scroll_y);

            const int32_t viewport_height = el->parent->getParam(ParamIndex::logic_y2) - el->parent->getParam(ParamIndex::logic_y1);
            const int32_t element_height = el->getParam(ParamIndex::logic_y2) - el->getParam(ParamIndex::logic_y1);

            constexpr int32_t top_margin = 0;
            constexpr int32_t bottom_margin = 0;

            const int32_t visible_top = top_margin;
            const int32_t visible_bottom = viewport_height - bottom_margin;
            const int32_t visible_height = visible_bottom - visible_top;

            if (element_height <= visible_height) {
                if (screen_y1 < visible_top) {
                    el->parent->getParam(ParamIndex::scroll_y) -= (visible_top - screen_y1);
                }
                else if (screen_y2 > visible_bottom) {
                    el->parent->getParam(ParamIndex::scroll_y) += (screen_y2 - visible_bottom);
                }
            }
            else {
                if (screen_y1 >= visible_bottom) {
                    el->parent->getParam(ParamIndex::scroll_y) = el->getParam(ParamIndex::logic_y1) - visible_top;
                }
                else if (screen_y2 <= visible_top) {
                    el->parent->getParam(ParamIndex::scroll_y) = el->getParam(ParamIndex::logic_y2) - visible_bottom;
                }
                else {
                    ;
                }
            }

            // 钳位滚动范围
            if (el->parent->getParam(ParamIndex::scroll_y) < 0) {
                el->parent->getParam(ParamIndex::scroll_y) = 0;
            }
            if (el->parent->getParam(ParamIndex::scroll_y) > (1<<30)) {
                el->parent->getParam(ParamIndex::scroll_y) = (1<<30);
            }

            // TODO: 加入 Page 全局 Margin 统筹 删除递归处理 Scroll

            if (dynamic_cast<Element*>(el->getParent())) {
                scrollToShow(dynamic_cast<Element*>(el->getParent()));
            }
        }

        // TODO: 增加对超大元素的元素内滚动支持函数 scrollwithDirection(DirectIndex direction)

        bool setFocus(Element* el) {
            if (el == nullptr || el == focus) {
                return false;
            }
            std::lock_guard<std::recursive_mutex> lock(page_mutex);
            if (focus) {focus->focused = false;}
            focus = el;
            focus->focused = true;
            scrollToShow(focus);
            return true;
        }

        bool moveFocus(DirecIndex direction) {
            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            if (!focus) {
                return false;
            }

            // TODO: 如果超大元素不能完整显示，优先 Scroll 该元素而不是切换焦点

            Element* next =
                focus->focus_next[static_cast<size_t>(direction)];

            if (!next) {
                return false;
            }

            return setFocus(next);
        }

        /* ------------------------
         * FocusMap/focus_next 定义
         * ------------------------
         * FocusMap (a.k.a. focus_next数组) 存储八个方向上距离当前元素距离评分（标准见下正方向和斜方向）最近的节点
         * LCA为上层同一节点的节点 记作同层节点
         * 不在同层的节点不能直接通过 FocusMap 连接 需要前往当前元素的父节点或子节点直至达到同层
         * 方向顺序编号见 DirectIndex
         */

        void rebuildFocusMap() {
            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            auto rebuildLayer = [&](auto&& self, BasicItem* container) -> void {
                if (!container) return;

                std::vector<Element*> layer_elements;

                // 只收集 container 的直接子 Element  每个 container 对应一张独立的 FocusMap
                for (BasicItem* child : container->getChildren()) {
                    auto* child_element = dynamic_cast<Element*>(child);
                    if (!child_element) {
                        continue;
                    }

                    // 无论当前是否可聚焦都先清除旧焦点映射
                    for (Element*& next : child_element->focus_next) {
                        next = nullptr;
                    }

                    // strc_type 最低位：是否可被聚焦（见上定义）
                    if (
                        child_element->getParam(ParamIndex::strc_type) &
                        0x01
                    ) {
                        layer_elements.push_back(child_element);
                    }
                }

                // 为同一父节点下的 Element 建立焦点网络
                for (Element* current : layer_elements) {
                    std::array<int64_t, 8> best_score;
                    best_score.fill(std::numeric_limits<int64_t>::max());

                    // 使用矩形中心进行方向判断 注意放缩了1倍 没有除以2不是真实 Logic 位置！
                    const int64_t current_center_x =
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::logic_x1)
                        ) +
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::logic_x2)
                        );

                    const int64_t current_center_y =
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::logic_y1)
                        ) +
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::logic_y2)
                        );

                    for (Element* candidate : layer_elements) {
                        if (candidate == current) {
                            continue;
                        }

                        const int64_t candidate_center_x =
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::logic_x1)
                            ) +
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::logic_x2)
                            );

                        const int64_t candidate_center_y =
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::logic_y1)
                            ) +
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::logic_y2)
                            );

                        const int64_t delta_x =
                            candidate_center_x - current_center_x;

                        const int64_t delta_y =
                            candidate_center_y - current_center_y;

                        if (delta_x == 0 && delta_y == 0) {
                            continue;
                        }

                        const int64_t abs_x =
                            delta_x >= 0 ? delta_x : -delta_x;

                        const int64_t abs_y =
                            delta_y >= 0 ? delta_y : -delta_y;

                        auto trySet = [&](DirecIndex direction, int64_t score) {
                            const size_t index =
                                static_cast<size_t>(direction);

                            if (score < best_score[index]) {
                                best_score[index] = score;
                                current->focus_next[index] = candidate;
                            }
                        };

                        /*
                         * 四个正方向：
                         * 主方向距离正常计分 垂直于主方向的偏移给予更高权重
                         *
                         * 优先选择方向更正的元素
                         * 同方向中优先选择距离近且更对齐的元素
                         */

                        if (delta_x > 0) {
                            const int64_t score =
                                delta_x * delta_x +
                                4 * delta_y * delta_y;

                            trySet(DirecIndex::right, score);
                        }

                        if (delta_x < 0) {
                            const int64_t score =
                                delta_x * delta_x +
                                4 * delta_y * delta_y;

                            trySet(DirecIndex::left, score);
                        }

                        if (delta_y < 0) {
                            const int64_t score =
                                delta_y * delta_y +
                                4 * delta_x * delta_x;

                            trySet(DirecIndex::up, score);
                        }

                        if (delta_y > 0) {
                            const int64_t score =
                                delta_y * delta_y +
                                4 * delta_x * delta_x;

                            trySet(DirecIndex::down, score);
                        }

                        /*
                         * 四个斜方向：
                         *
                         * 优先选择方向更45度的元素
                         * 同方向中优先选择距离近且更对齐的元素
                         */
                        if (delta_x > 0 && delta_y < 0) {
                            const int64_t imbalance = abs_x - abs_y;

                            const int64_t score =
                                delta_x * delta_x +
                                delta_y * delta_y +
                                2 * imbalance * imbalance;

                            trySet(DirecIndex::up_right, score);
                        }

                        if (delta_x < 0 && delta_y < 0) {
                            const int64_t imbalance = abs_x - abs_y;

                            const int64_t score =
                                delta_x * delta_x +
                                delta_y * delta_y +
                                2 * imbalance * imbalance;

                            trySet(DirecIndex::up_left, score);
                        }

                        if (delta_x < 0 && delta_y > 0) {
                            const int64_t imbalance = abs_x - abs_y;

                            const int64_t score =
                                delta_x * delta_x +
                                delta_y * delta_y +
                                2 * imbalance * imbalance;

                            trySet(DirecIndex::down_left, score);
                        }

                        if (delta_x > 0 && delta_y > 0) {
                            const int64_t imbalance = abs_x - abs_y;

                            const int64_t score =
                                delta_x * delta_x +
                                delta_y * delta_y +
                                2 * imbalance * imbalance;

                            trySet(DirecIndex::down_right, score);
                        }
                    }
                }

                // 递归处理所有子容器 存到不同 FocusMap
                for (BasicItem* child : container->getChildren()) {
                    self(self, child);
                }
            };

            rebuildLayer(rebuildLayer, this);
        }

        bool enterFocusLayer() {
            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            if (!focus) {
                return false;
            }

            for (BasicItem* child : focus->getChildren()) {
                auto* child_element = dynamic_cast<Element*>(child);

                if (!child_element) {
                    continue;
                }

                // strc_type 最低位：可聚焦
                if (child_element->getParam(ParamIndex::strc_type) & 0x01) {
                    return setFocus(child_element);
                }
            }

            return false;
        }
        
        bool leaveFocusLayer() {
            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            if (!focus) {
                return false;
            }

            auto* parent_element = dynamic_cast<Element*>(focus->getParent());

            if (!parent_element) {
                return false;
            }

            return setFocus(parent_element);
        }

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;
    };
}

#endif //APSISUI2_UISTRUCTURE_H
