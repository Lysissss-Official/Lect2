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
        logic_x1 = 0,
        logic_y1 = 1,
        logic_x2 = 2,
        logic_y2 = 3,
        phys_x1 = 4,
        phys_y1 = 5,
        phys_x2 = 6,
        phys_y2 = 7,
        fg_color = 8,
        bg_color = 9,
        bdr_color = 10,
        txt_color = 11,
        scroll_x = 12,
        scroll_y = 13,
        strc_type = 15
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
        SCREEN_BW   = 0,
        SCREEN_GRAY = 1,
        SCREEN_RGB565  = 2,
        SCREEN_RGB666 = 3,
        SCREEN_RGB888  = 4
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

            // 元素当前屏幕坐标（已减去滚动偏移）
            int32_t screen_y1 = el->getParam(ParamIndex::logic_y1) - el->parent->getParam(ParamIndex::scroll_y);
            int32_t screen_y2 = el->getParam(ParamIndex::logic_y2) - el->parent->getParam(ParamIndex::scroll_y);

            int32_t top_margin    = 0;
            int32_t bottom_margin = 0;

            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            // 元素上端被状态栏遮挡 → 向下滚动
            if (screen_y1 < top_margin) {
                el->params[static_cast<size_t>(ParamIndex::scroll_y)] -= (top_margin - screen_y1);
            }
            // 元素下端超出视口底部 → 向上滚动
            else if (screen_y2 > bottom_margin) {
                el->params[static_cast<size_t>(ParamIndex::scroll_y)] += (screen_y2 - bottom_margin);
            }

            // 钳位滚动范围
            if (el->getParam(ParamIndex::scroll_y) < 0) {
                el->params[static_cast<size_t>(ParamIndex::scroll_y)] = 0;
            }
            if (el->getParam(ParamIndex::scroll_y) > (1<<30)) {
                el->params[static_cast<size_t>(ParamIndex::scroll_y)] = (1<<30);
            }

            if (dynamic_cast<Element*>(el->getParent())) {
                scrollToShow(dynamic_cast<Element*>(el->getParent()));
            }
        }

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

        void rebuildFocusMap() const {
            std::vector<BasicItem*> all_elements;
            for (auto& child_el : children) {
                // strc_type 最低位：是否可被聚焦
                if (child_el->getParam(ParamIndex::strc_type) & 1) {
                    all_elements.push_back(child_el);
                }
            }
        }

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;
    };
}

#endif //APSISUI2_UISTRUCTURE_H
