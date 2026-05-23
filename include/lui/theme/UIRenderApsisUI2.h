//
// Created by archeart on 2026/5/23.
//
// ApsisUI II theme renderer.
// Implements renderService() and renderPage() using the LUI element hierarchy.
//

#ifndef APSISUI2_UIRENDERAPSISUI2_H
#define APSISUI2_UIRENDERAPSISUI2_H

#include <cstdio>

#include "easyx.h"
#include "graphics.h"
#include "../base/UIRender.h"
#include "../base/UITransor.h"

namespace lui {
    namespace render {

        // Theme colour constants (EasyX format: 0xBBGGRR)
        namespace theme_color {
            constexpr uint16_t BG_DARK       = 0x1A1A2E;
            constexpr uint16_t BG_PANEL      = 0x16213E;
            constexpr uint16_t ACCENT        = 0x0F3460;
            constexpr uint16_t ACCENT_HIGH   = 0x533483;
            constexpr uint16_t TEXT_PRIMARY  = 0xE0E0E0;
            constexpr uint16_t TEXT_DIM      = 0x888888;
            constexpr uint16_t FOCUS_BORDER  = 0x00AAFF;
            constexpr uint16_t BUTTON_FILL   = 0x1A3A5C;
            constexpr uint16_t BUTTON_HOVER  = 0x2A5A8C;
            constexpr uint16_t STATUS_BAR_BG = 0x0D1B3E;
        }

        class UIRenderApsisUI2 : public Render {
        private:
            transor::UITransorLinux7* ts;

        public:
            explicit UIRenderApsisUI2(transor::UITransorLinux7* translator)
                : ts(translator) {}

            void renderService() override {
                while (!render_requests_pre.empty()) {
                    RenderRequest req = render_requests_pre.front();
                    render_requests_pre.pop();

                    // Execute the request
                    switch (req.type) {
                        case REQ_CHANGE_PAGE:
                            if (ts) ts->ClearDeviceCmd();
                            break;
                        case REQ_CHANGE_ELEMENT:
                            // Element-level changes would be dispatched here
                            break;
                        case REQ_DRAW_ELEMENT:
                            break;
                    }
                }
            }

            void renderPage(pge::Page* page) override {
                if (!page || !ts) return;

                // Draw background
                setbkcolor(static_cast<COLORREF>(theme_color::BG_DARK));
                ts->ClearDeviceCmd();

                uint16_t bar_h = page->status_bar_thickness;

                // Draw status bar
                {
                    int sw = getwidth();
                    setfillcolor(static_cast<COLORREF>(theme_color::STATUS_BAR_BG));
                    setlinecolor(static_cast<COLORREF>(theme_color::STATUS_BAR_BG));
                    fillrectangle(0, 0, sw, static_cast<int>(bar_h));

                    char buf[64];
                    snprintf(buf, sizeof(buf), "LectOS 2 | %s page",
                             page->height_mode == pge::PAGE_FULL ? "FULL" : "HALF");
                    settextcolor(static_cast<COLORREF>(theme_color::TEXT_DIM));
                    settextstyle(14, 0, _T("Consolas"));
                    outtextxy(8, 4, buf);
                }

                // Draw each block and its elements
                for (auto& blk : page->blocks) {

                    // Block background panel
                    ts->drawFilledRect(
                        blk.phys_x1, blk.phys_y1,
                        blk.phys_x2, blk.phys_y2,
                        theme_color::BG_PANEL,
                        theme_color::ACCENT);

                    // Block label
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Block #%u", blk.getID());
                        settextcolor(static_cast<COLORREF>(theme_color::TEXT_DIM));
                        settextstyle(11, 0, _T("Consolas"));
                        outtextxy(static_cast<int>(blk.phys_x1) + 4,
                                  static_cast<int>(blk.phys_y1) + 2, buf);
                    }

                    // Draw each element
                    for (auto& el : blk.elements) {
                        drawElement(el, page);
                    }
                }
            }

        private:
            void drawElement(ele::Element& el, pge::Page* /*page*/) {
                uint16_t border_c = el.focused ? theme_color::FOCUS_BORDER
                                               : theme_color::ACCENT;
                uint16_t fill_c   = el.focused ? theme_color::BUTTON_HOVER
                                               : theme_color::BUTTON_FILL;

                switch (el.type) {
                    case ele::ELE_BUTTON:
                        ts->drawFilledRect(el.phys_x1, el.phys_y1,
                                           el.phys_x2, el.phys_y2,
                                           fill_c, border_c);
                        drawElementLabel(el);
                        break;

                    case ele::ELE_TEXTBOX:
                        ts->drawFilledRect(el.phys_x1, el.phys_y1,
                                           el.phys_x2, el.phys_y2,
                                           theme_color::BG_PANEL, border_c);
                        drawElementLabel(el);
                        break;

                    case ele::ELE_LIST:
                        ts->drawFilledRect(el.phys_x1, el.phys_y1,
                                           el.phys_x2, el.phys_y2,
                                           theme_color::BG_PANEL, border_c);
                        drawElementLabel(el);
                        // Draw scrollbar hint
                        {
                            int sx = el.phys_x2 - 8;
                            setlinecolor(static_cast<COLORREF>(theme_color::TEXT_DIM));
                            line(sx, el.phys_y1 + 4, sx, el.phys_y2 - 4);
                        }
                        break;

                    case ele::ELE_EMPTY:
                        // Just draw a dashed border hint — app handles content
                        setlinecolor(static_cast<COLORREF>(border_c));
                        rectangle(el.phys_x1, el.phys_y1,
                                  el.phys_x2, el.phys_y2);
                        break;
                }
            }

            void drawElementLabel(ele::Element& el) {
                int cx = (el.phys_x1 + el.phys_x2) / 2;
                int cy = (el.phys_y1 + el.phys_y2) / 2;
                settextcolor(static_cast<COLORREF>(
                    el.focused ? WHITE : theme_color::TEXT_PRIMARY));
                settextstyle(14, 0, _T("Consolas"));

                const char* label = el.content.empty() ? " " : el.content.c_str();
                int tw = textwidth(label);
                outtextxy(cx - tw / 2, cy - 7, label);
            }
        };
    }
}

#endif //APSISUI2_UIRENDERAPSISUI2_H
