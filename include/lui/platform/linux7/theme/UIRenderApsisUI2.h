//
// Created by archeart on 2026/5/23.
//
// ApsisUI II 主题渲染器
// ============================================================================
// 继承自 lui::Render，实现 LUI 五层架构的完整渲染管线。
//
// 渲染管线（renderService 调用顺序）：
//   0. 推进滚动动画（覆写 page->scroll_y 为插值）
//   1. 清屏 + 背景填充
//   2. 固定状态栏（屏幕顶部，不随滚动偏移）
//   3. 推进焦点动画状态机
//   4. 遍历区块（含滚动偏移裁剪），逐元素绘制
//   5. 焦点动画叠加层（屏幕空间，置顶）
//   6. 滚动条（右侧，按需显示）
//   7. 动画保活：焦点/滚动动画进行中提交 keep-alive 请求
//
// 请求队列（继承自 Render）：
//   render_queue_ — 小根堆，按 time_end 排序，过期自动弹出
//   全部渲染由 requestReDraw + chrono 时间驱动，无外部旁路
//
// 文本渲染策略：
//   优先使用位图字体（lui::ext::FontBase，通过 Transor 逐像素绘制），
//   字体未注入时回退 EasyX 原生文本（Consolas + outtextxy）。
//
// 焦点动画：
//   聚焦切换时，边框从旧位置平滑插值到新位置（easeOut 三次缓出），
//   持续时长约 220ms，动画结束后自动停用。
//
// 滚动动画：
//   页面滚动偏移变化时（焦点切换触发 scrollToShow），从当前显示位置
//   平滑滚动到目标位置（easeOut 三次缓出），持续时长约 280ms。
// ============================================================================

#ifndef APSISUI2_UIRENDERAPSISUI2_H
#define APSISUI2_UIRENDERAPSISUI2_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "easyx.h"
#include "graphics.h"
#include "../../../base/UIRender.h"
#include "../../../base/UITransor.h"
#include "../../../extension/font/FontBase.h"

namespace lui {

        namespace theme_color {
            constexpr COLORREF BG_DARK       = 0x0A0A0A;
            constexpr COLORREF BG_PANEL      = 0x121212;
            constexpr COLORREF ACCENT        = 0x303000;
            constexpr COLORREF ACCENT_HIGH   = 0x606000;
            constexpr COLORREF TEXT_PRIMARY  = 0xE0E0E0;
            constexpr COLORREF TEXT_DIM      = 0x808060;
            constexpr COLORREF FOCUS_BORDER  = 0xCCCC00;
            constexpr COLORREF FOCUS_GLOW    = 0xDDDD22;
            constexpr COLORREF BUTTON_FILL   = 0x252500;
            constexpr COLORREF BUTTON_HOVER  = 0x454500;
            constexpr COLORREF STATUS_BAR_BG = 0x060606;
            constexpr COLORREF SCROLL_TRACK  = 0x040404;
            constexpr COLORREF SCROLL_THUMB  = 0x353500;
        }

        class UIRenderApsisUI2 : public Render {
        private:
            UITransorLinux7* ts;
            lui::ext::FontBase* font = nullptr;

            struct FocusAnim {
                bool     active       = false;
                uint32_t last_focused = 0;
                int      from_x1 = 0, from_y1 = 0, from_x2 = 0, from_y2 = 0;
                int      to_x1   = 0, to_y1   = 0, to_x2   = 0, to_y2   = 0;
                std::chrono::steady_clock::time_point start_time;
                float    duration_s = 0.22f;
            };
            FocusAnim anim;

            struct ScrollAnim {
                bool     active         = false;
                bool     initialized    = false;
                float    from_scroll    = 0.0f;
                float    to_scroll      = 0.0f;
                float    current_scroll = 0.0f;
                std::chrono::steady_clock::time_point start_time;
                float    duration_s     = 0.28f;
            };
            ScrollAnim scroll_anim;

            static float easeOut(float t) {
                if (t >= 1.0f) return 1.0f;
                return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            }

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
            explicit UIRenderApsisUI2(UITransorLinux7* translator)
                : ts(translator) {}

            void setFont(lui::ext::FontBase* f) { font = f; }

            // =============================================================
            // renderService — 唯一渲染入口，chrono 时间驱动
            // =============================================================
            // 处理 render_queue_ 中所有请求，执行完整渲染管线。
            // Page* 目标触发清屏 + 全页重绘。
            // 焦点/滚动动画进行中自动提交 keep-alive 请求保持 daemon 活跃。
            // =============================================================
            void renderService() override {
                if (!current_page_ || !ts) return;

                using namespace std::chrono;

                // 检查是否需要清屏（Page* 请求）
                bool clear_screen = false;
                for (auto& req : rendering_queue_) {
                    if (std::holds_alternative<Page*>(req.target_ptr)) {
                        clear_screen = true;
                        break;
                    }
                }

                Page* page = current_page_;

                BeginBatchDraw();

                advanceScrollAnimation(page);

                int bar_h  = static_cast<int>(page->status_bar_thickness);
                int scr_w  = getwidth();
                int scr_h  = getheight();
                int scroll = static_cast<int>(page->scroll_y);

                // 背景
                if (clear_screen) ts->ClearDeviceCmd();

                // 状态栏
                {
                    ts->drawFilledRect(0, 0, scr_w, bar_h,
                        theme_color::STATUS_BAR_BG, theme_color::STATUS_BAR_BG);

                    char buf[64];
                    snprintf(buf, sizeof(buf), "LectOS 2 | %s | scroll: %d",
                             page->height_mode == PAGE_FULL ? "FULL" : "HALF",
                             scroll);
                    drawText(8, 4, buf, theme_color::TEXT_DIM, 13);
                }

                advanceFocusAnimation(page);

                int vp_top    = bar_h;
                int vp_bottom = scr_h;

                // 区块遍历
                for (auto& blk : page->blocks) {
                    int by1 = static_cast<int>(blk.phys_y1) - scroll;
                    int by2 = static_cast<int>(blk.phys_y2) - scroll;

                    if (by2 < vp_top || by1 > vp_bottom) continue;

                    int clip_by1 = std::max(by1, vp_top);

                    ts->drawFilledRect(blk.phys_x1, clip_by1, blk.phys_x2, by2,
                                       theme_color::BG_PANEL, theme_color::ACCENT);

                    if (by1 >= vp_top) {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Block #%u", blk.getID());
                        drawText(static_cast<int>(blk.phys_x1) + 4, by1 + 2,
                                 buf, theme_color::TEXT_DIM, 11);
                    }

                    for (auto& el : blk.elements) {
                        int ey1 = static_cast<int>(el.phys_y1) - scroll;
                        int ey2 = static_cast<int>(el.phys_y2) - scroll;
                        if (ey2 < vp_top || ey1 > vp_bottom) continue;
                        drawElement(el, scroll, vp_top);
                    }
                }

                // 焦点叠加层
                drawFocusOverlay(scroll);

                // 滚动条
                if (page->max_scroll > 0.0f) {
                    drawScrollbar(page);
                }

                FlushBatchDraw();

                // 动画保活：提交 keep-alive 请求覆盖剩余时长
                auto now = steady_clock::now();
                float keep_s = 0.0f;
                if (anim.active) {
                    float e = duration<float>(now - anim.start_time).count();
                    keep_s = std::max(keep_s, anim.duration_s - e);
                }
                if (scroll_anim.active) {
                    float e = duration<float>(now - scroll_anim.start_time).count();
                    keep_s = std::max(keep_s, scroll_anim.duration_s - e);
                }
                if (keep_s > 0.0f) {
                    requestReDraw(std::monostate{}, now,
                                  duration<float, std::milli>(keep_s * 1000.0f));
                }
            }

        private:
            void advanceScrollAnimation(Page* page) {
                float target = page->scroll_y;

                if (!scroll_anim.initialized) {
                    scroll_anim.current_scroll = target;
                    scroll_anim.from_scroll    = target;
                    scroll_anim.to_scroll      = target;
                    scroll_anim.initialized    = true;
                    return;
                }

                if (target != scroll_anim.to_scroll
                    && target != scroll_anim.current_scroll) {
                    scroll_anim.from_scroll = scroll_anim.current_scroll;
                    scroll_anim.to_scroll   = target;
                    scroll_anim.start_time  = std::chrono::steady_clock::now();
                    scroll_anim.active      = true;
                }

                if (scroll_anim.active) {
                    auto now = std::chrono::steady_clock::now();
                    float elapsed = std::chrono::duration<float>(
                        now - scroll_anim.start_time).count();
                    float t = std::min(elapsed / scroll_anim.duration_s, 1.0f);
                    scroll_anim.current_scroll = scroll_anim.from_scroll +
                        (scroll_anim.to_scroll - scroll_anim.from_scroll) * easeOut(t);
                    page->scroll_y = scroll_anim.current_scroll;

                    if (t >= 1.0f) {
                        scroll_anim.active = false;
                        scroll_anim.current_scroll = scroll_anim.to_scroll;
                        page->scroll_y = scroll_anim.to_scroll;
                    }
                } else {
                    scroll_anim.current_scroll = target;
                }
            }

            void advanceFocusAnimation(Page* page) {
                if (!page->current_focused) {
                    anim.active = false;
                    return;
                }

                uint32_t cur_id = page->current_focused->getID();

                if (!anim.active || anim.last_focused != cur_id) {
                    Element* cur = page->current_focused;

                    if (anim.last_focused != 0 && anim.last_focused != cur_id) {
                        anim.from_x1 = anim.to_x1;
                        anim.from_y1 = anim.to_y1;
                        anim.from_x2 = anim.to_x2;
                        anim.from_y2 = anim.to_y2;
                    } else {
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

            void drawFocusOverlay(int scroll) {
                if (!anim.active) return;

                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(
                    now - anim.start_time).count();
                float t = std::min(elapsed / anim.duration_s, 1.0f);
                float e = easeOut(t);

                int cx1 = anim.from_x1 + static_cast<int>(
                    (anim.to_x1 - anim.from_x1) * e);
                int cy1 = anim.from_y1 + static_cast<int>(
                    (anim.to_y1 - anim.from_y1) * e);
                int cx2 = anim.from_x2 + static_cast<int>(
                    (anim.to_x2 - anim.from_x2) * e);
                int cy2 = anim.from_y2 + static_cast<int>(
                    (anim.to_y2 - anim.from_y2) * e);

                cy1 -= scroll;
                cy2 -= scroll;

                int scr_h = getheight();
                if (cy2 < 0 || cy1 > scr_h) {
                    if (t >= 1.0f) anim.active = false;
                    return;
                }

                int g = 3;
                // 外光晕 2px
                for (int i = 0; i < 2; i++) {
                    ts->drawLineCmd(cx1-g+i, cy1-g+i, cx2+g-i, cy1-g+i, theme_color::FOCUS_GLOW);
                    ts->drawLineCmd(cx1-g+i, cy2+g-i, cx2+g-i, cy2+g-i, theme_color::FOCUS_GLOW);
                    ts->drawLineCmd(cx1-g+i, cy1-g+i, cx1-g+i, cy2+g-i, theme_color::FOCUS_GLOW);
                    ts->drawLineCmd(cx2+g-i, cy1-g+i, cx2+g-i, cy2+g-i, theme_color::FOCUS_GLOW);
                }
                // 内边框 1px
                ts->drawLineCmd(cx1, cy1, cx2, cy1, theme_color::FOCUS_BORDER);
                ts->drawLineCmd(cx1, cy2, cx2, cy2, theme_color::FOCUS_BORDER);
                ts->drawLineCmd(cx1, cy1, cx1, cy2, theme_color::FOCUS_BORDER);
                ts->drawLineCmd(cx2, cy1, cx2, cy2, theme_color::FOCUS_BORDER);

                if (t >= 1.0f) anim.active = false;
            }

            void drawElement(Element& el, int scroll, int vp_top) {
                int x1 = el.phys_x1, y1 = static_cast<int>(el.phys_y1) - scroll;
                int x2 = el.phys_x2, y2 = static_cast<int>(el.phys_y2) - scroll;

                int clip_y1 = std::max(y1, vp_top);

                COLORREF border_c = el.focused ? theme_color::FOCUS_BORDER
                                               : theme_color::ACCENT;
                COLORREF fill_c   = el.focused ? theme_color::BUTTON_HOVER
                                               : theme_color::BUTTON_FILL;

                switch (el.type) {
                    case ELE_BUTTON:
                        ts->drawFilledRect(x1, clip_y1, x2, y2, fill_c, border_c);
                        drawElementLabel(el, scroll, vp_top,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        break;
                    case ELE_TEXTBOX:
                        ts->drawFilledRect(x1, clip_y1, x2, y2,
                            theme_color::BG_PANEL, border_c);
                        drawElementLabel(el, scroll, vp_top,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        break;
                    case ELE_LIST:
                        ts->drawFilledRect(x1, clip_y1, x2, y2,
                            theme_color::BG_PANEL, border_c);
                        drawElementLabel(el, scroll, vp_top,
                            el.focused ? WHITE : theme_color::TEXT_PRIMARY);
                        {
                            int sx = x2 - 8;
                            int ly1 = std::max(y1 + 4, vp_top);
                            ts->drawLineCmd(sx, ly1, sx, y2 - 4,
                                theme_color::TEXT_DIM);
                        }
                        break;
                    case ELE_EMPTY:
                        ts->drawLineCmd(x1, clip_y1, x2, clip_y1, border_c);
                        ts->drawLineCmd(x1, y2, x2, y2, border_c);
                        ts->drawLineCmd(x1, clip_y1, x1, y2, border_c);
                        ts->drawLineCmd(x2, clip_y1, x2, y2, border_c);
                        break;
                }
            }

            void drawElementLabel(Element& el, int scroll, int vp_top,
                                  COLORREF color) {
                int cy = (static_cast<int>(el.phys_y1) +
                          static_cast<int>(el.phys_y2)) / 2 - scroll;

                if (cy < vp_top) return;

                const char* label = el.content.empty() ? " " : el.content.c_str();
                int font_h = font ? 20 : 14;
                int off_y = font ? 10 : 7;

                if (el.type == ELE_TEXTBOX || el.type == ELE_LIST) {
                    int tx = static_cast<int>(el.phys_x1) + 6;
                    drawText(tx, cy - off_y, label, color, font_h);
                } else {
                    int cx = (el.phys_x1 + el.phys_x2) / 2;
                    int tw = textWidth(label, font_h);
                    drawText(cx - tw / 2, cy - off_y, label, color, font_h);
                }
            }

            void drawScrollbar(Page* page) {
                int scr_w  = getwidth();
                int scr_h  = getheight();
                int bar_h  = static_cast<int>(page->status_bar_thickness);
                int track_h = scr_h - bar_h;

                float ratio = static_cast<float>(scr_h - bar_h) /
                              (page->max_scroll + scr_h - bar_h);
                int thumb_h = std::max(static_cast<int>(track_h * ratio), 20);
                int thumb_y = bar_h + static_cast<int>(
                    page->scroll_y / page->max_scroll * (track_h - thumb_h));

                int sx = scr_w - 8;

                ts->drawFilledRect(sx, bar_h, sx + 6, scr_h,
                    theme_color::SCROLL_TRACK, theme_color::SCROLL_TRACK);

                ts->drawFilledRect(sx, thumb_y, sx + 6, thumb_y + thumb_h,
                    theme_color::SCROLL_THUMB, theme_color::SCROLL_THUMB);
            }
        };
}

#endif //APSISUI2_UIRENDERAPSISUI2_H
