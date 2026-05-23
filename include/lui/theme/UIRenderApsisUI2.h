//
// Created by archeart on 2026/5/23.
//
// ApsisUI II 主题渲染器
// ============================================================================
// 继承自 lui::render::Render，实现 LUI 五层架构的完整渲染管线。
//
// 渲染管线（renderPage 调用顺序）：
//   1. 清屏 + 背景填充
//   2. 固定状态栏（屏幕顶部，不随滚动偏移）
//   3. 推进焦点动画状态机
//   4. 遍历区块（含滚动偏移裁剪），逐元素绘制
//   5. 焦点动画叠加层（屏幕空间，置顶）
//   6. 滚动条（右侧，按需显示）
//
// 双队列机制（继承自 Render）：
//   render_requests_pre — 等待队列：外部提交的渲染请求先进入此队列
//   render_requests_now — 进行队列：renderService() 处理 pre 队列后移入
//
// 文本渲染策略：
//   优先使用位图字体（lui::ext::font::FontBase，通过 Transor 逐像素绘制），
//   字体未注入时回退 EasyX 原生文本（Consolas + outtextxy）。
//
// 焦点动画：
//   聚焦切换时，边框从旧位置平滑插值到新位置（easeOut 三次缓出），
//   持续时长约 220ms，动画结束后自动停用。
// ============================================================================

#ifndef APSISUI2_UIRENDERAPSISUI2_H
#define APSISUI2_UIRENDERAPSISUI2_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "easyx.h"
#include "graphics.h"
#include "../base/UIRender.h"
#include "../base/UITransor.h"
#include "../extension/font/FontBase.h"

namespace lui {
    namespace render {

        // =====================================================================
        // theme_color — ApsisUI II 主题色板
        // =====================================================================
        // 黑色 + 青色基调，背景低饱和度。EasyX COLORREF 为 BGR 格式（0xBBGGRR）。
        // =====================================================================
        namespace theme_color {
            constexpr COLORREF BG_DARK       = 0x0A0A0A;  // 全局背景（纯黑）
            constexpr COLORREF BG_PANEL      = 0x121212;  // 面板 / 区块背景（微抬黑）
            constexpr COLORREF ACCENT        = 0x303000;  // 普通边框 / 分割线（暗青）
            constexpr COLORREF ACCENT_HIGH   = 0x606000;  // 高亮强调色（青色）
            constexpr COLORREF TEXT_PRIMARY  = 0xE0E0E0;  // 主文本色（浅灰白）
            constexpr COLORREF TEXT_DIM      = 0x808060;  // 次要文本色（青灰）
            constexpr COLORREF FOCUS_BORDER  = 0xCCCC00;  // 焦点框边框（亮青）
            constexpr COLORREF FOCUS_GLOW    = 0xDDDD22;  // 焦点框光晕（荧光青）
            constexpr COLORREF BUTTON_FILL   = 0x252500;  // 按钮默认填充（暗青）
            constexpr COLORREF BUTTON_HOVER  = 0x454500;  // 按钮聚焦时填充（青亮）
            constexpr COLORREF STATUS_BAR_BG = 0x060606;  // 状态栏背景（最深黑）
            constexpr COLORREF SCROLL_TRACK  = 0x040404;  // 滚动条轨道（近纯黑）
            constexpr COLORREF SCROLL_THUMB  = 0x353500;  // 滚动条滑块（暗青）
        }

        // =====================================================================
        // UIRenderApsisUI2 — ApsisUI II 主题渲染器
        // =====================================================================
        class UIRenderApsisUI2 : public Render {
        private:
            // Transor 转译器（平台层 EasyX 实现），用于底层绘制原语。
            transor::UITransorLinux7* ts;

            // 位图字体（可选）：注入后文本渲染将逐像素绘制，未注入则回退 EasyX。
            lui::ext::font::FontBase* font = nullptr;

            // =================================================================
            // FocusAnim — 焦点边框插值动画状态
            // =================================================================
            // 焦点从一个元素切换到另一个时：
            //   from_* — 动画起始坐标（上一帧的终点或旧聚焦元素的物理位置）
            //   to_*   — 动画目标坐标（新聚焦元素的物理位置）
            //   start_time — 动画起始时间点，用于计算插值进度
            //   duration_s — 动画总时长（秒），默认 0.22s
            //
            // 特殊情况：
            //   当 last_focused 从非零变为另一个 ID 时（跨元素跳转），
            //   from 将设为上一帧的 to（即当前视觉位置），避免突变。
            //   当页面首次聚焦或 last_focused 为 0 时，from = to（无动画）。
            // =================================================================
            struct FocusAnim {
                bool     active       = false;
                uint32_t last_focused = 0;     // 上一帧聚焦元素 ID（0 = 无）
                int      from_x1 = 0, from_y1 = 0, from_x2 = 0, from_y2 = 0;
                int      to_x1   = 0, to_y1   = 0, to_x2   = 0, to_y2   = 0;
                std::chrono::steady_clock::time_point start_time;
                float    duration_s = 0.22f;
            };
            FocusAnim anim;

            // 缓出函数（三次方）：t 从 0→1 时，速度从快到慢，视觉上自然停止。
            static float easeOut(float t) {
                if (t >= 1.0f) return 1.0f;
                return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            }

            // =================================================================
            // drawText — 统一文本绘制
            // =================================================================
            // 位图字体可用时：将 C 字符串转为 u16string，逐像素通过 Transor 绘制。
            // 位图字体不可用时：回退 EasyX settextstyle + outtextxy。
            // font_h 参数仅在回退时生效（指定 Consolas 字号）。
            // =================================================================
            void drawText(int x, int y, const char* str, COLORREF color, int font_h) {
                if (font) {
                    std::u16string u16;
                    for (const char* p = str; *p; ++p)
                        u16.push_back(static_cast<char16_t>(*p));
                    font->drawString(ts, static_cast<uint16_t>(x),
                                     static_cast<uint16_t>(y), color, u16);
                } else {
                    settextcolor(color);
                    settextstyle(font_h, 0, _T("Consolas"));
                    outtextxy(x, y, str);
                }
            }

            // =================================================================
            // textWidth — 统一文本宽度测量
            // =================================================================
            // 位图字体可用时：累加每个字符的字形宽度（从 FontInfo.width 读取）。
            // 位图字体不可用时：调用 EasyX textwidth()。
            // font_h 参数仅在回退时生效（需与 settextstyle 一致）。
            // =================================================================
            int textWidth(const char* str, int font_h) {
                if (font) {
                    std::u16string u16;
                    for (const char* p = str; *p; ++p)
                        u16.push_back(static_cast<char16_t>(*p));
                    return static_cast<int>(font->getStringWidth(u16));
                }
                settextstyle(font_h, 0, _T("Consolas"));
                return textwidth(str);
            }

        public:
            explicit UIRenderApsisUI2(transor::UITransorLinux7* translator)
                : ts(translator) {}

            // 注入位图字体（如 font_20），设为 nullptr 可回退 EasyX 文本渲染。
            void setFont(lui::ext::font::FontBase* f) { font = f; }

            // =================================================================
            // renderService — 处理渲染请求队列
            // =================================================================
            // 当前仅处理 REQ_CHANGE_PAGE（页面切换 → 清屏）。
            // 将 render_requests_pre 队列中所有请求消费完毕后返回。
            // =================================================================
            void renderService() override {
                while (!render_requests_pre.empty()) {
                    RenderRequest req = render_requests_pre.front();
                    render_requests_pre.pop();
                    switch (req.type) {
                        case REQ_CHANGE_PAGE:
                            if (ts) ts->ClearDeviceCmd();
                            break;
                        default: break;
                    }
                }
            }

            // =================================================================
            // renderPage — 主渲染入口（每帧调用）
            // =================================================================
            // 渲染顺序（由底到顶）：
            //   1. 背景：全屏填充 BG_DARK，调用 ClearDeviceCmd
            //   2. 状态栏：固定顶部，显示页面标题和滚动偏移量
            //   3. 焦点动画：推进动画时间线
            //   4. 区块遍历：含视口裁剪（跳过超出屏幕的区块/元素），应用滚动偏移
            //   5. 焦点叠加层：绘制动画中的焦点框（屏幕空间，不受滚动影响）
            //   6. 滚动条：按需绘制（仅当 max_scroll > 0）
            // =================================================================
            void renderPage(pge::Page* page) override {
                if (!page || !ts) return;

                int bar_h  = static_cast<int>(page->status_bar_thickness);
                int scr_w  = getwidth();
                int scr_h  = getheight();
                int scroll = static_cast<int>(page->scroll_y);

                // ---- 1. 背景 ----
                setbkcolor(theme_color::BG_DARK);
                ts->ClearDeviceCmd();

                // ---- 2. 固定状态栏 ----
                {
                    setfillcolor(theme_color::STATUS_BAR_BG);
                    setlinecolor(theme_color::STATUS_BAR_BG);
                    fillrectangle(0, 0, scr_w, bar_h);

                    char buf[64];
                    snprintf(buf, sizeof(buf), "LectOS 2 | %s | scroll: %d",
                             page->height_mode == pge::PAGE_FULL ? "FULL" : "HALF",
                             scroll);
                    drawText(8, 4, buf, theme_color::TEXT_DIM, 13);
                }

                // ---- 3. 推进焦点动画 ----
                advanceAnimation(page);

                int vp_top    = bar_h;      // 视口上边界 = 状态栏下方
                int vp_bottom = scr_h;       // 视口下边界 = 屏幕底部

                // ---- 4. 区块遍历 ----
                for (auto& blk : page->blocks) {
                    // 区块物理坐标（减去滚动偏移）
                    int by1 = static_cast<int>(blk.phys_y1) - scroll;
                    int by2 = static_cast<int>(blk.phys_y2) - scroll;

                    // 视口裁剪：区块完全在视口外则跳过
                    if (by2 < vp_top || by1 > vp_bottom) continue;

                    // 绘制区块背景（面板色填充 + 强调色边框）
                    ts->drawFilledRect(blk.phys_x1, by1, blk.phys_x2, by2,
                                       theme_color::BG_PANEL, theme_color::ACCENT);

                    // 区块 ID 标签（调试用，左上角小字）
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Block #%u", blk.getID());
                        drawText(static_cast<int>(blk.phys_x1) + 4, by1 + 2,
                                 buf, theme_color::TEXT_DIM, 11);
                    }

                    // 遍历子元素（同样含视口裁剪）
                    for (auto& el : blk.elements) {
                        int ey1 = static_cast<int>(el.phys_y1) - scroll;
                        int ey2 = static_cast<int>(el.phys_y2) - scroll;
                        if (ey2 < vp_top || ey1 > vp_bottom) continue;
                        drawElement(el, scroll);
                    }
                }

                // ---- 5. 焦点动画叠加层 ----
                drawFocusOverlay(scroll);

                // ---- 6. 滚动条 ----
                if (page->max_scroll > 0.0f) {
                    drawScrollbar(page);
                }
            }

        private:
            // =================================================================
            // advanceAnimation — 推进焦点动画状态机
            // =================================================================
            // 在 renderPage 中每次调用，检测聚焦元素是否变化：
            //   - 无聚焦元素 → 停用动画
            //   - 聚焦元素变化（last_focused != cur_id）：
            //       * 若之前曾有过另一个焦点（跨元素跳转）：from = 旧 to（保持视觉连续）
            //       * 若首次聚焦（last_focused == 0）：from = to = 当前元素位置（无动画）
            //     重置 start_time 为新起点
            //   - 聚焦元素未变 → 不重置，让动画继续插值到终点
            // =================================================================
            void advanceAnimation(pge::Page* page) {
                if (!page->current_focused) {
                    anim.active = false;
                    return;
                }

                uint32_t cur_id = page->current_focused->getID();

                if (!anim.active || anim.last_focused != cur_id) {
                    ele::Element* cur = page->current_focused;

                    // 跨元素跳转：from 取旧动画的终点（避免视觉跳变）
                    if (anim.last_focused != 0 && anim.last_focused != cur_id) {
                        anim.from_x1 = anim.to_x1;
                        anim.from_y1 = anim.to_y1;
                        anim.from_x2 = anim.to_x2;
                        anim.from_y2 = anim.to_y2;
                    } else {
                        // 首次聚焦或无旧动画：from = to（起始无动画）
                        anim.from_x1 = static_cast<int>(cur->phys_x1);
                        anim.from_y1 = static_cast<int>(cur->phys_y1);
                        anim.from_x2 = static_cast<int>(cur->phys_x2);
                        anim.from_y2 = static_cast<int>(cur->phys_y2);
                    }

                    anim.to_x1 = static_cast<int>(cur->phys_x1);
                    anim.to_y1 = static_cast<int>(cur->phys_y1);
                    anim.to_x2 = static_cast<int>(cur->phys_x2);
                    anim.to_y2 = static_cast<int>(cur->phys_y2);
                    anim.last_focused = cur_id;
                    anim.start_time   = std::chrono::steady_clock::now();
                    anim.active = true;
                }
            }

            // =================================================================
            // drawFocusOverlay — 绘制焦点动画叠加层
            // =================================================================
            // 在屏幕空间绘制动画中的焦点框（不受页面滚动影响）。
            // 双层结构：
            //   外层 — 光晕（FOCUS_GLOW，线宽 2px，外扩 3px），半透明视觉效果
            //   内层 — 主边框（FOCUS_BORDER，线宽 1px），精确贴合元素边界
            //
            // 动画进度 t 由 easeOut 缓出函数驱动：0→1 时速度递减，视觉自然。
            // 动画完成（t >= 1.0f）后自动将 anim.active 设为 false。
            // 如果动画框完全移出视口，提前终止。
            // =================================================================
            void drawFocusOverlay(int scroll) {
                if (!anim.active) return;

                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - anim.start_time).count();
                float t = std::min(elapsed / anim.duration_s, 1.0f);
                float e = easeOut(t);

                // 四角线性插值：from + (to - from) * e
                int cx1 = anim.from_x1 + static_cast<int>((anim.to_x1 - anim.from_x1) * e);
                int cy1 = anim.from_y1 + static_cast<int>((anim.to_y1 - anim.from_y1) * e);
                int cx2 = anim.from_x2 + static_cast<int>((anim.to_x2 - anim.from_x2) * e);
                int cy2 = anim.from_y2 + static_cast<int>((anim.to_y2 - anim.from_y2) * e);

                // 叠加层在屏幕空间，需补偿滚动偏移
                cy1 -= scroll;
                cy2 -= scroll;

                // 视口裁剪：完全不可见时退出
                int scr_h = getheight();
                if (cy2 < 0 || cy1 > scr_h) {
                    if (t >= 1.0f) anim.active = false;
                    return;
                }

                // 光晕（外层）— 外扩 3px，线宽 2px
                int g = 3;
                setlinecolor(theme_color::FOCUS_GLOW);
                setlinestyle(PS_SOLID, 2);
                rectangle(cx1 - g, cy1 - g, cx2 + g, cy2 + g);

                // 主焦点框（内层）— 精确位置，线宽 1px
                setlinecolor(theme_color::FOCUS_BORDER);
                rectangle(cx1, cy1, cx2, cy2);
                setlinestyle(PS_SOLID, 1);

                if (t >= 1.0f) anim.active = false;
            }

            // =================================================================
            // drawElement — 按类型分发元素绘制
            // =================================================================
            // 支持四种元素类型：
            //   ELE_BUTTON — 填充矩形 + 文本标签，聚焦时颜色高亮
            //   ELE_TEXTBOX — 面板背景矩形 + 文本标签
            //   ELE_LIST   — 同 TEXTBOX，额外绘制右侧滚动提示线
            //   ELE_EMPTY  — 虚线边框空矩形（调试/占位用）
            //
            // 聚焦元素使用 focus 色系（边框亮蓝 + 填充高亮），
            // 非聚焦元素使用 accent 色系（暗色边框）。
            // =================================================================
            void drawElement(ele::Element& el, int scroll) {
                int x1 = el.phys_x1, y1 = static_cast<int>(el.phys_y1) - scroll;
                int x2 = el.phys_x2, y2 = static_cast<int>(el.phys_y2) - scroll;

                // 根据聚焦状态选择颜色
                COLORREF border_c = el.focused ? theme_color::FOCUS_BORDER
                                               : theme_color::ACCENT;
                COLORREF fill_c   = el.focused ? theme_color::BUTTON_HOVER
                                               : theme_color::BUTTON_FILL;

                setlinestyle(PS_SOLID, 1);

                switch (el.type) {
                    case ele::ELE_BUTTON:
                        ts->drawFilledRect(x1, y1, x2, y2, fill_c, border_c);
                        drawElementLabel(el, scroll,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        break;
                    case ele::ELE_TEXTBOX:
                        ts->drawFilledRect(x1, y1, x2, y2,
                            theme_color::BG_PANEL, border_c);
                        drawElementLabel(el, scroll,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        break;
                    case ele::ELE_LIST:
                        ts->drawFilledRect(x1, y1, x2, y2,
                            theme_color::BG_PANEL, border_c);
                        drawElementLabel(el, scroll,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        {
                            // 右侧滚动提示竖线
                            int sx = x2 - 8;
                            setlinecolor(theme_color::TEXT_DIM);
                            line(sx, y1 + 4, sx, y2 - 4);
                        }
                        break;
                    case ele::ELE_EMPTY:
                        // 虚线矩形占位：仅边框，无填充，无文本
                        setlinecolor(border_c);
                        setlinestyle(PS_DOT, 1);
                        rectangle(x1, y1, x2, y2);
                        setlinestyle(PS_SOLID, 1);
                        break;
                }
            }

            // =================================================================
            // drawElementLabel — 元素文本标签（水平垂直居中）
            // =================================================================
            // 计算逻辑：
            //   cx = 元素水平中点
            //   cy = 元素垂直中点（减去滚动偏移）
            //   tw = 文本像素宽度（位图字体或 EasyX 测量）
            //   off_y = 垂直偏移（位图 20px 字体用 10，EasyX 14px 字体用 7）
            // 最终绘制位置 x = cx - tw/2, y = cy - off_y（居中显示）
            // =================================================================
            void drawElementLabel(ele::Element& el, int scroll, COLORREF color) {
                int cx = (el.phys_x1 + el.phys_x2) / 2;
                int cy = (static_cast<int>(el.phys_y1) +
                          static_cast<int>(el.phys_y2)) / 2 - scroll;

                const char* label = el.content.empty() ? " " : el.content.c_str();
                int font_h = font ? 20 : 14;
                int tw = textWidth(label, font_h);
                int off_y = font ? 10 : 7;
                drawText(cx - tw / 2, cy - off_y, label, color, font_h);
            }

            // =================================================================
            // drawScrollbar — 绘制垂直滚动条
            // =================================================================
            // 布局：屏幕右侧 8px 宽区域，状态栏下方至屏幕底部。
            //
            // 计算公式：
            //   track_h  = 屏幕高 - 状态栏厚度（轨道总高度）
            //   ratio    = track_h / (max_scroll + track_h)（滑块高度占比）
            //   thumb_h  = max(track_h * ratio, 20) （最小 20px 防止过小）
            //   thumb_y  = bar_h + (scroll_y / max_scroll) * (track_h - thumb_h)
            //
            // 滑块位置与 scroll_y 成正比，视觉上反映当前滚动位置。
            // =================================================================
            void drawScrollbar(pge::Page* page) {
                int scr_w  = getwidth();
                int scr_h  = getheight();
                int bar_h  = static_cast<int>(page->status_bar_thickness);
                int track_h = scr_h - bar_h;

                // 滑块高度按可见比例缩略
                float ratio = static_cast<float>(scr_h - bar_h) /
                              (page->max_scroll + scr_h - bar_h);
                int thumb_h = std::max(static_cast<int>(track_h * ratio), 20);
                int thumb_y = bar_h + static_cast<int>(
                    page->scroll_y / page->max_scroll * (track_h - thumb_h));

                int sx = scr_w - 8;  // 滚动条左边界

                // 轨道
                setfillcolor(theme_color::SCROLL_TRACK);
                setlinecolor(theme_color::SCROLL_TRACK);
                fillrectangle(sx, bar_h, sx + 6, scr_h);

                // 滑块
                setfillcolor(theme_color::SCROLL_THUMB);
                setlinecolor(theme_color::SCROLL_THUMB);
                fillrectangle(sx, thumb_y, sx + 6, thumb_y + thumb_h);
            }
        };
    }
}

#endif //APSISUI2_UIRENDERAPSISUI2_H
