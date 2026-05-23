//
// Created by archeart on 2026/5/23.
//
// ApsisUI II theme renderer.
// Implements renderService() and renderPage() using the LUI element hierarchy.
// Supports vertical scrolling and animated focus transitions.
//

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

namespace lui {
    namespace render {

        namespace theme_color {
            constexpr COLORREF BG_DARK       = 0x1A1A2E;
            constexpr COLORREF BG_PANEL      = 0x16213E;
            constexpr COLORREF ACCENT        = 0x0F3460;
            constexpr COLORREF ACCENT_HIGH   = 0x533483;
            constexpr COLORREF TEXT_PRIMARY  = 0xE0E0E0;
            constexpr COLORREF TEXT_DIM      = 0x888888;
            constexpr COLORREF FOCUS_BORDER  = 0x00AAFF;
            constexpr COLORREF FOCUS_GLOW    = 0x33BBFF;
            constexpr COLORREF BUTTON_FILL   = 0x1A3A5C;
            constexpr COLORREF BUTTON_HOVER  = 0x2A5A8C;
            constexpr COLORREF STATUS_BAR_BG = 0x0D1B3E;
            constexpr COLORREF SCROLL_TRACK  = 0x0A0A20;
            constexpr COLORREF SCROLL_THUMB  = 0x333366;
        }

        class UIRenderApsisUI2 : public Render {
        private:
            transor::UITransorLinux7* ts;

            // Focus animation state
            struct FocusAnim {
                bool     active       = false;
                uint32_t last_focused = 0;
                int      from_x1 = 0, from_y1 = 0, from_x2 = 0, from_y2 = 0;
                int      to_x1   = 0, to_y1   = 0, to_x2   = 0, to_y2   = 0;
                std::chrono::steady_clock::time_point start_time;
                float    duration_s = 0.22f;
            };
            FocusAnim anim;

            static float easeOut(float t) {
                if (t >= 1.0f) return 1.0f;
                return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            }

        public:
            explicit UIRenderApsisUI2(transor::UITransorLinux7* translator)
                : ts(translator) {}

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

            void renderPage(pge::Page* page) override {
                if (!page || !ts) return;

                int bar_h  = static_cast<int>(page->status_bar_thickness);
                int scr_w  = getwidth();
                int scr_h  = getheight();
                int scroll = static_cast<int>(page->scroll_y);

                // Background
                setbkcolor(theme_color::BG_DARK);
                ts->ClearDeviceCmd();

                // Fixed status bar
                {
                    setfillcolor(theme_color::STATUS_BAR_BG);
                    setlinecolor(theme_color::STATUS_BAR_BG);
                    fillrectangle(0, 0, scr_w, bar_h);

                    char buf[64];
                    snprintf(buf, sizeof(buf), "LectOS 2 | %s | scroll: %d",
                             page->height_mode == pge::PAGE_FULL ? "FULL" : "HALF",
                             scroll);
                    settextcolor(theme_color::TEXT_DIM);
                    settextstyle(13, 0, _T("Consolas"));
                    outtextxy(8, 4, buf);
                }

                // Advance focus animation
                advanceAnimation(page);

                int vp_top    = bar_h;
                int vp_bottom = scr_h;

                // Blocks (scrolled)
                for (auto& blk : page->blocks) {
                    int by1 = static_cast<int>(blk.phys_y1) - scroll;
                    int by2 = static_cast<int>(blk.phys_y2) - scroll;
                    if (by2 < vp_top || by1 > vp_bottom) continue;

                    ts->drawFilledRect(blk.phys_x1, by1, blk.phys_x2, by2,
                                       theme_color::BG_PANEL, theme_color::ACCENT);

                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Block #%u", blk.getID());
                        settextcolor(theme_color::TEXT_DIM);
                        settextstyle(11, 0, _T("Consolas"));
                        outtextxy(static_cast<int>(blk.phys_x1) + 4, by1 + 2, buf);
                    }

                    for (auto& el : blk.elements) {
                        int ey1 = static_cast<int>(el.phys_y1) - scroll;
                        int ey2 = static_cast<int>(el.phys_y2) - scroll;
                        if (ey2 < vp_top || ey1 > vp_bottom) continue;
                        drawElement(el, scroll);
                    }
                }

                // Animated focus overlay (on top, screen space)
                drawFocusOverlay(scroll);

                // Scrollbar
                if (page->max_scroll > 0.0f) {
                    drawScrollbar(page);
                }
            }

        private:
            void advanceAnimation(pge::Page* page) {
                if (!page->current_focused) {
                    anim.active = false;
                    return;
                }

                uint32_t cur_id = page->current_focused->getID();

                if (!anim.active || anim.last_focused != cur_id) {
                    ele::Element* cur = page->current_focused;

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
                float elapsed = std::chrono::duration<float>(now - anim.start_time).count();
                float t = std::min(elapsed / anim.duration_s, 1.0f);
                float e = easeOut(t);

                int cx1 = anim.from_x1 + static_cast<int>((anim.to_x1 - anim.from_x1) * e);
                int cy1 = anim.from_y1 + static_cast<int>((anim.to_y1 - anim.from_y1) * e);
                int cx2 = anim.from_x2 + static_cast<int>((anim.to_x2 - anim.from_x2) * e);
                int cy2 = anim.from_y2 + static_cast<int>((anim.to_y2 - anim.from_y2) * e);

                cy1 -= scroll;
                cy2 -= scroll;

                int scr_h = getheight();
                if (cy2 < 0 || cy1 > scr_h) {
                    if (t >= 1.0f) anim.active = false;
                    return;
                }

                // Glow ring (outer)
                int g = 3;
                setlinecolor(theme_color::FOCUS_GLOW);
                setlinestyle(PS_SOLID, 2);
                rectangle(cx1 - g, cy1 - g, cx2 + g, cy2 + g);

                // Main focus ring (inner)
                setlinecolor(theme_color::FOCUS_BORDER);
                rectangle(cx1, cy1, cx2, cy2);
                setlinestyle(PS_SOLID, 1);

                if (t >= 1.0f) anim.active = false;
            }

            void drawElement(ele::Element& el, int scroll) {
                int x1 = el.phys_x1, y1 = static_cast<int>(el.phys_y1) - scroll;
                int x2 = el.phys_x2, y2 = static_cast<int>(el.phys_y2) - scroll;

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
                            int sx = x2 - 8;
                            setlinecolor(theme_color::TEXT_DIM);
                            line(sx, y1 + 4, sx, y2 - 4);
                        }
                        break;
                    case ele::ELE_EMPTY:
                        setlinecolor(border_c);
                        setlinestyle(PS_DOT, 1);
                        rectangle(x1, y1, x2, y2);
                        setlinestyle(PS_SOLID, 1);
                        break;
                }
            }

            void drawElementLabel(ele::Element& el, int scroll, COLORREF color) {
                int cx = (el.phys_x1 + el.phys_x2) / 2;
                int cy = (static_cast<int>(el.phys_y1) +
                          static_cast<int>(el.phys_y2)) / 2 - scroll;
                settextcolor(color);
                settextstyle(14, 0, _T("Consolas"));

                const char* label = el.content.empty() ? " " : el.content.c_str();
                int tw = textwidth(label);
                outtextxy(cx - tw / 2, cy - 7, label);
            }

            void drawScrollbar(pge::Page* page) {
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

                setfillcolor(theme_color::SCROLL_TRACK);
                setlinecolor(theme_color::SCROLL_TRACK);
                fillrectangle(sx, bar_h, sx + 6, scr_h);

                setfillcolor(theme_color::SCROLL_THUMB);
                setlinecolor(theme_color::SCROLL_THUMB);
                fillrectangle(sx, thumb_y, sx + 6, thumb_y + thumb_h);
            }
        };
    }
}

#endif //APSISUI2_UIRENDERAPSISUI2_H
