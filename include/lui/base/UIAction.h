//
// Created by archeart on 2026/7/24.
//

#ifndef APSISUI2_UIACTION_H
#define APSISUI2_UIACTION_H

#include "UIStructure.h"
#include "UIRender.h"

#include "lcore/Log.h"

#include <cstdint>

namespace lui::action {

    // TODO: 加 namespace "strc::~"

    inline bool scrollToShow(strc::Element* el, Render* re) {
        if (!el || !re) return false;

        auto* el_parent = el->getParent();
        if (!el_parent) return false;

        //std::lock_guard<std::recursive_mutex> lock(page_mutex);

        int32_t target_scroll_y = el_parent->getParam(strc::ParamIndex::scroll_y);

        // 元素当前屏幕坐标（已减去滚动偏移）
        int32_t screen_y1 = el->getParam(strc::ParamIndex::rel_y1) - el_parent->getParam(strc::ParamIndex::scroll_y);
        int32_t screen_y2 = el->getParam(strc::ParamIndex::rel_y2) - el_parent->getParam(strc::ParamIndex::scroll_y);

        const int32_t viewport_height = el_parent->getParam(strc::ParamIndex::rel_y2) - el_parent->getParam(strc::ParamIndex::rel_y1);
        const int32_t element_height = el->getParam(strc::ParamIndex::rel_y2) - el->getParam(strc::ParamIndex::rel_y1);

        constexpr int32_t top_margin = 0;
        constexpr int32_t bottom_margin = 0;

        const int32_t visible_top = top_margin;
        const int32_t visible_bottom = viewport_height - bottom_margin;
        const int32_t visible_height = visible_bottom - visible_top;

        if (element_height <= visible_height) {
            if (screen_y1 < visible_top) {
                target_scroll_y -= (visible_top - screen_y1);
            }
            else if (screen_y2 > visible_bottom) {
                target_scroll_y += (screen_y2 - visible_bottom);
            }
        }
        else {
            if (screen_y1 >= visible_bottom) {
                target_scroll_y = el->getParam(strc::ParamIndex::rel_y1) - visible_top;
            }
            else if (screen_y2 <= visible_top) {
                target_scroll_y = el->getParam(strc::ParamIndex::rel_y2) - visible_bottom;
            }
            else {
                ;
            }
        }

        // 钳位滚动范围
        if (target_scroll_y < 0) {
            target_scroll_y = 0;
        }
        if (target_scroll_y > (1<<30)) {
            target_scroll_y = (1<<30);
        }

        // TODO: 加入 Page 全局 Margin 统筹 删除递归处理 Scroll

        /*
        LOG(
            "element=" +
            std::to_string(el->getID()) +
            " parent=" +
            std::to_string(el_parent->getID()) +
            " rel_y=(" +
            std::to_string(el->getParam(strc::ParamIndex::rel_y1)) +
            "," +
            std::to_string(el->getParam(strc::ParamIndex::rel_y2)) +
            ")" +
            " parent_rel_y=(" +
            std::to_string(el_parent->getParam(strc::ParamIndex::rel_y1)) +
            "," +
            std::to_string(el_parent->getParam(strc::ParamIndex::rel_y2)) +
            ")" +
            " parent_scroll=" +
            std::to_string(
                el_parent->getParam(strc::ParamIndex::scroll_y)
            ) +
            " target_scroll=" +
            std::to_string(
                target_scroll_y
            ) +
            " screen_y=(" +
            std::to_string(screen_y1) +
            "," +
            std::to_string(screen_y2) +
            ")" +
            " viewport=" +
            std::to_string(viewport_height)
        );
        */

        re->requestAnimate(
            el_parent,
            strc::ParamIndex::scroll_y,
            target_scroll_y,
            std::chrono::duration<float, std::milli>(500)
        );

        if (auto* parent_element =
            dynamic_cast<strc::Element*>(el->getParent())
        ) {
            scrollToShow(parent_element, re);
        }

        return true;
    }

    inline bool moveSetFocus(strc::Page* pa, strc::Element* el, Render* re) {
        if (!pa->focus) {
            return false;
        }

        // TODO: 如果超大元素不能完整显示，优先 Scroll 该元素而不是切换焦点

        strc::Element* next = el;

        if (!next) {
            return false;
        }

        if (pa->setFocus(next)) {
            return scrollToShow(next, re);
        }
        else {
            return false;
        }
    }

    inline bool moveNextFocus(strc::Page* pa, strc::DirecIndex direc, Render* re) {
        if (!pa->focus) {
            return false;
        }

        // TODO: 如果超大元素不能完整显示，优先 Scroll 该元素而不是切换焦点

        strc::Element* next =
            pa->focus->focus_next[static_cast<size_t>(direc)];

        if (!next) {
            return false;
        }

        if (pa->setFocus(next)) {
            return scrollToShow(next, re);
        }
        else {
            return false;
        }
    }
}

#endif //APSISUI2_UIACTION_H
