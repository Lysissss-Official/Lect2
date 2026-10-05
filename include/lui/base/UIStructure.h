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
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <limits>
#include <concepts>
#include <utility>

#include "lcore/IDGenerator.h"
#include "ldevice/screen/DevScreen.h"


namespace lui::strc {

    class CanvasNode;
    class Easel;

    /* -------------
     * item_cfg 定义
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

    // 基本参数索引
    // (x1, y1) = 左上角坐标
    // (x2, y2) = 右下角坐标

    // 注：以下rel和abs坐标都基于Logic坐标系（屏幕空间=>0-10000）
    //     映射为Phys坐标系（屏幕空间=>屏幕分辨率）见Abs2Phys函数

    enum class ParamIndex : uint16_t {
        rel_x1 = 0,   // 相对X1坐标（相对父节点左上角坐标）
        rel_y1 = 1,   // 相对Y1坐标
        rel_x2 = 2,   // 相对X2坐标
        rel_y2 = 3,   // 相对Y2坐标
        abs_x1 = 4,    // 绝对X1坐标（相对页面左上角坐标）
        abs_y1 = 5,    // 绝对Y1坐标
        abs_x2 = 6,    // 绝对X2坐标
        abs_y2 = 7,    // 绝对Y2坐标
        scroll_x = 8,  // X轴方向滚动逻辑量
        scroll_y = 9,  // Y轴方向滚动逻辑量
        item_typ = 10,  // 控件类型
        item_cfg = 11   // 控件配置 (a.k.a. strc_type)
    };

    // 拓展参数索引
    // TODO: 完善 OptionalParamIndex 使用 void* ?
    enum class OptionalParamIndex : uint16_t {
        none = 0,
        draw_func = 1,
        text = 2,
        icon = 3,
        image = 4,
        audio = 5,
        video = 6,
        user_data = 10086
    };

    // 方向索引
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


    struct OptionalParam {
        OptionalParamIndex index;
        uint32_t value;
    };


    struct ItemStyle {
        int32_t lx1 = 0;
        int32_t ly1 = 0;
        int32_t lx2 = 0;
        int32_t ly2 = 0;

        int32_t fg_clr = 0;
        int32_t bg_clr = 0;
        int32_t bdr_clr = 0;
        int32_t txt_clr = 0;

        int32_t scroll_x = 0;
        int32_t scroll_y = 0;

        int32_t item_typ = 0;
        int32_t item_cfg = 0;
    };

    class BasicNode {
    protected:
        uint32_t uni_id = 0;
        std::array<int32_t, 12> params{};
        std::unique_ptr<std::vector<OptionalParam>> optional_params;

        BasicNode* parent = nullptr;
        std::vector<std::unique_ptr<BasicNode>> children;

    public:
        virtual ~BasicNode() = default;


        // -------------------------------
        // ============ Param ============

        int32_t& getParam(ParamIndex idx) {
            return params[static_cast<size_t>(idx)];
        }

        [[nodiscard]] const int32_t& getParam(ParamIndex idx) const {
            return params[static_cast<size_t>(idx)];
        }


        // ---------------------------------------
        // ============ OptionalParam ============

        bool addOptionalParam(const OptionalParam& opt_param) {
            if (!optional_params) {
                optional_params = std::make_unique<std::vector<OptionalParam>>();
            }

            optional_params->push_back(opt_param);
            return true;
        }

        bool removeOptionalParam(OptionalParam* opt_param) {
            if (!optional_params || !opt_param) {
                return false;
            }

            bool removed = false;

            auto it = optional_params->begin();

            while (it != optional_params->end()) {

                if (it->value == opt_param->value &&
                    it->index == opt_param->index)
                {
                    it = optional_params->erase(it);
                    removed = true;
                }
                else {
                    ++it;
                }
            }

            return removed;
        }
        bool removeOptionalParam(size_t nth) {
            if (!optional_params || nth>=optional_params->size()) {
                return false;
            }

            optional_params->erase(optional_params->begin() + static_cast<std::ptrdiff_t>(nth));

            return true;
        }

        OptionalParam* getOptionalParam(size_t nth) {
            if (!optional_params || nth>=optional_params->size()) {
                return nullptr;
            }

            return &(*optional_params)[nth];
        }

        [[nodiscard]]
        const OptionalParam* getOptionalParam(size_t nth) const{
            if (!optional_params || nth>=optional_params->size()) {
                return nullptr;
            }

            return &(*optional_params)[nth];
        }

        OptionalParam* getOptionalParam(OptionalParamIndex index, size_t nth) {
            if (!optional_params) {
                return nullptr;
            }

            size_t cnt = 0;
            for (auto& param : *optional_params) {
                if (param.index == index) {
                    if (cnt == nth) {
                        return &param;
                    } else {
                        cnt++;
                    }
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        const OptionalParam* getOptionalParam(OptionalParamIndex index, size_t nth) const {
            if (!optional_params) {
                return nullptr;
            }

            size_t cnt = 0;
            for (const auto& param : *optional_params) {
                if (param.index == index) {
                    if (cnt == nth) {
                        return &param;
                    } else {
                        cnt++;
                    }
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        const std::vector<OptionalParam>& getOptionalParams() const {
            static const std::vector<OptionalParam> empty;

            if (!optional_params) {
                return empty;
            }

            return *optional_params;
        }


        // -------------------------------
        // ============ Basic ============

        [[nodiscard]] uint32_t getID() const {
            return uni_id;
        }

        void updateAbsolute(
            int32_t delta_abs_x = 0,
            int32_t delta_abs_y = 0,
            bool recursive = true
        ) {
            int32_t scroll_x = 0;
            int32_t scroll_y = 0;

            if (parent) {
                scroll_x = parent->getParam(ParamIndex::scroll_x);
                scroll_y = parent->getParam(ParamIndex::scroll_y);
            }

            int32_t base_x = delta_abs_x - scroll_x;
            int32_t base_y = delta_abs_y - scroll_y;

            params[static_cast<size_t>(ParamIndex::abs_x1)] =
                static_cast<int32_t>(
                    (base_x + getParam(ParamIndex::rel_x1))
                );

            params[static_cast<size_t>(ParamIndex::abs_y1)] =
                static_cast<int32_t>(
                    (base_y + getParam(ParamIndex::rel_y1))
                );

            params[static_cast<size_t>(ParamIndex::abs_x2)] =
                static_cast<int32_t>(
                    (base_x + getParam(ParamIndex::rel_x2))
                );

            params[static_cast<size_t>(ParamIndex::abs_y2)] =
                static_cast<int32_t>(
                    (base_y + getParam(ParamIndex::rel_y2))
                );

            if (recursive) {
                for (auto& child_el : children) {
                    const int32_t dlt_abs_x_next =
                        base_x + getParam(ParamIndex::rel_x1);

                    const int32_t dlt_abs_y_next =
                        base_y + getParam(ParamIndex::rel_y1);

                    child_el->updateAbsolute(
                        dlt_abs_x_next,
                        dlt_abs_y_next,
                        recursive
                    );
                }
            }
        }

        [[nodiscard]] BasicNode* getParent() const { return parent; }

        [[nodiscard]] size_t getChildCount() const {
            return children.size();
        }

        [[nodiscard]] BasicNode* getChild(size_t index) const {
            return index < children.size() ? children[index].get() : nullptr;
        }

        [[nodiscard]] const std::vector<std::unique_ptr<BasicNode>>& getChildren() const {
            return children;
        }

        template<typename T, typename... Args>
        // requires std::derived_from<T, CanvasNode> // C++20
        T* createChild(Args&&... args);

        bool addChild(std::unique_ptr<CanvasNode> child);

        bool destroyChild(BasicNode* child) {
            if (!child) {
                return false;
            }

            const auto iterator =
                std::find_if(
                    children.begin(),
                    children.end(),
                    [child](const std::unique_ptr<BasicNode>& item) {
                        return item.get() == child;
                    }
                );

            if (iterator == children.end()) {
                return false;
            }

            children.erase(iterator);
            return true;
        }

        bool clearChildren() {
            children.clear();
            return true;
        }
    };

    class CanvasNode : public BasicNode {
        friend class Easel;
    public:
        bool focused;
        std::vector<CanvasNode*> focus_next;

        CanvasNode() : focused(false){
            uni_id = lcore::IDGenerator<CanvasNode>::generate();
            //params.resize(12);
            focus_next.resize(8);
        }

        CanvasNode(const ItemStyle& style) : focused(false){
            uni_id = lcore::IDGenerator<CanvasNode>::generate();
            //params.resize(12);
            focus_next.resize(8);
            applyStyle(style);
        }

        ~CanvasNode() override{
            lcore::IDGenerator<CanvasNode>::release(uni_id);
        }

        void applyStyle(const ItemStyle& style) {
            getParam(ParamIndex::rel_x1) = style.lx1;
            getParam(ParamIndex::rel_y1) = style.ly1;
            getParam(ParamIndex::rel_x2) = style.lx2;
            getParam(ParamIndex::rel_y2) = style.ly2;

            //getParam(ParamIndex::fg_color) = style.fg_clr;
            //getParam(ParamIndex::bg_color) = style.bg_clr;
            //getParam(ParamIndex::bdr_color) = style.bdr_clr;
            //getParam(ParamIndex::txt_color) = style.txt_clr;

            getParam(ParamIndex::scroll_x) = style.scroll_x;
            getParam(ParamIndex::scroll_y) = style.scroll_y;
            getParam(ParamIndex::item_typ) = style.item_typ;
            getParam(ParamIndex::item_cfg) = style.item_cfg;
        }

        [[nodiscard]] bool containsLogical(uint32_t x, uint32_t y) const {
            return x >= getParam(ParamIndex::rel_x1)
                && x <= getParam(ParamIndex::rel_x2)
                && y >= getParam(ParamIndex::rel_y1)
                && y <= getParam(ParamIndex::rel_y2);
        }

        CanvasNode(const CanvasNode&) = delete;
        CanvasNode& operator=(const CanvasNode&) = delete;
    };

    class Easel : public BasicNode {
    public:
        mutable std::recursive_mutex page_mutex;
        CanvasNode* focus;

        Easel() : focus(nullptr){
            uni_id = lcore::IDGenerator<Easel>::generate();
            //params.resize(12);
        }
        ~Easel() override {
            lcore::IDGenerator<Easel>::release(uni_id);
        }

        // TODO: 增加对超大元素的元素内滚动支持函数 scrollwithDirection(DirectIndex direction)

        bool setFocus(CanvasNode* el) {
            if (el == nullptr || el == focus) {
                return false;
            }
            std::lock_guard<std::recursive_mutex> lock(page_mutex);
            if (focus) {focus->focused = false;}
            focus = el;
            focus->focused = true;
            //scrollToShow(focus);
            return true;
        }

        [[nodiscard]] CanvasNode* nextFocus(DirecIndex direction) const {
            if (!focus) {
                return nullptr;
            }

            return focus->focus_next[static_cast<size_t>(direction)];
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

            auto rebuildLayer = [&](auto&& self, BasicNode* container) -> void {
                if (!container) return;

                std::vector<CanvasNode*> layer_elements;

                // 只收集 container 的直接子 Element  每个 container 对应一张独立的 FocusMap
                for (const auto& child : container->getChildren()) {
                    auto* child_element = dynamic_cast<CanvasNode*>(child.get());
                    if (!child_element) {
                        continue;
                    }

                    // 无论当前是否可聚焦都先清除旧焦点映射
                    for (CanvasNode*& next : child_element->focus_next) {
                        next = nullptr;
                    }

                    // strc_type 最低位：是否可被聚焦（见上定义）
                    if (
                        child_element->getParam(ParamIndex::item_cfg) &
                        0x01
                    ) {
                        layer_elements.push_back(child_element);
                    }
                }

                // 为同一父节点下的 Element 建立焦点网络
                for (CanvasNode* current : layer_elements) {
                    std::array<int64_t, 8> best_score{};
                    best_score.fill(std::numeric_limits<int64_t>::max());

                    // 使用矩形中心进行方向判断 注意放缩了1倍 没有除以2不是真实 Logic 位置！
                    const int64_t current_center_x =
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::rel_x1)
                        ) +
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::rel_x2)
                        );

                    const int64_t current_center_y =
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::rel_y1)
                        ) +
                        static_cast<int64_t>(
                            current->getParam(ParamIndex::rel_y2)
                        );

                    for (CanvasNode* candidate : layer_elements) {
                        if (candidate == current) {
                            continue;
                        }

                        const int64_t candidate_center_x =
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::rel_x1)
                            ) +
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::rel_x2)
                            );

                        const int64_t candidate_center_y =
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::rel_y1)
                            ) +
                            static_cast<int64_t>(
                                candidate->getParam(ParamIndex::rel_y2)
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
                for (const auto& child : container->getChildren()) {
                    self(self, child.get());
                }
            };

            rebuildLayer(rebuildLayer, this);
        }

        bool enterFocusLayer() {
            std::lock_guard<std::recursive_mutex> lock(page_mutex);

            if (!focus) {
                return false;
            }

            for (const auto& child : focus->getChildren()) {
                auto* child_element = dynamic_cast<CanvasNode*>(child.get());

                if (!child_element) {
                    continue;
                }

                // strc_type 最低位：可聚焦
                if (child_element->getParam(ParamIndex::item_cfg) & 0x01) {
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

            auto* parent_element = dynamic_cast<CanvasNode*>(focus->getParent());

            if (!parent_element) {
                return false;
            }

            return setFocus(parent_element);
        }

        Easel(const Easel&) = delete;
        Easel& operator=(const Easel&) = delete;
    };

    template<typename T, typename... Args>
    // requires std::derived_from<T, CanvasNode> // C++20
    inline T* BasicNode::createChild(Args&&... args) {
        static_assert(
            std::is_base_of<CanvasNode, T>::value,
            "T must derive from CanvasNode"
        );

        auto child = std::make_unique<T>(
            std::forward<Args>(args)...
        );

        T* result = child.get();

        if (!addChild(std::move(child))) {
            return nullptr;
        }

        return result;
    }

    inline bool BasicNode::addChild(std::unique_ptr<CanvasNode> child) {
        if (!child || child.get() == this || child->parent) {
            return false;
        }

        child->parent = this;
        children.push_back(std::move(child));
        return true;
    }
}

namespace lui {
    struct ClipRect {
        int32_t phys_x1 = 0;
        int32_t phys_y1 = 0;
        int32_t phys_x2 = 0;
        int32_t phys_y2 = 0;

        bool empty() const {
            return phys_x2 <= phys_x1 || phys_y2 <= phys_y1;
        }

        static ClipRect unite(const ClipRect& a, const ClipRect& b) {
            if (a.empty()) return b;
            if (b.empty()) return a;

            return {
                std::min(a.phys_x1, b.phys_x1),
                std::min(a.phys_y1, b.phys_y1),
                std::max(a.phys_x2, b.phys_x2),
                std::max(a.phys_y2, b.phys_y2)
            };
        }

        static ClipRect intersect(const ClipRect& a, const ClipRect& b) {
            ClipRect result = {
                std::max(a.phys_x1, b.phys_x1),
                std::max(a.phys_y1, b.phys_y1),
                std::min(a.phys_x2, b.phys_x2),
                std::min(a.phys_y2, b.phys_y2)
            };

            if (result.empty()) {
                return {};
            }

            return result;
        }
    };

    // 绘制函数必备上下文参数
    struct DrawContext {
        strc::BasicNode& target;
        ldevice::Screen& screen;
        ClipRect clip;
        float progress = 1.0f;
    };

    // 绘制函数传参表
    using DrawFunction = void (*)(
        DrawContext& context
    );

    // 放缩Logic坐标系到Phys坐标系
    // abs_pos: 原始Logic坐标系 abs坐标
    // coordinate_scale: 10000(Logic)对应的屏幕像素(Phys)
    inline int32_t abs2Phys(int32_t abs_pos, uint32_t coordinate_scale) {
        return static_cast<int32_t>(
            static_cast<float>(abs_pos) *
            static_cast<float>(coordinate_scale) /
            10000.0f
        );
    }
}

#endif //APSISUI2_UISTRUCTURE_H
