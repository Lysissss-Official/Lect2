//
// Created by Shoiin / archeart on 2026/5/19.
//

#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H

#include <cstdint>
#include <string>
#include <vector>

namespace lui {

    // Forward declarations for cross-namespace references
    namespace render { class Render; }
    namespace ctrller { class CtrllerService; }
    namespace transor { class TranslatorService; }

    namespace ele {

        enum ElementType {
            ELE_TEXTBOX = 0,
            ELE_BUTTON,
            ELE_LIST,
            ELE_EMPTY    // Free draw via Render interface
        };

        class Element {
        private:
            uint32_t uni_id;

        public:
            ElementType type;

            // Logical position (percentage of screen, 0.0–100.0)
            float logic_x1, logic_y1;   // Top-left
            float logic_x2, logic_y2;   // Bottom-right

            // Physical position (pixels, computed at runtime via updatePhysical)
            uint16_t phys_x1, phys_y1;
            uint16_t phys_x2, phys_y2;

            // Focus navigation — auto-computed by Page::recalculateFocus()
            Element* focus_up    = nullptr;
            Element* focus_down  = nullptr;
            Element* focus_left  = nullptr;
            Element* focus_right = nullptr;

            bool focused = false;
            std::string content;

            Element() : type(ELE_EMPTY), logic_x1(0), logic_y1(0), logic_x2(0), logic_y2(0),
                        phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0), uni_id(0) {}

            uint32_t getID() const { return uni_id; }
            void setID(uint32_t id) { uni_id = id; }

            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                phys_x1 = static_cast<uint16_t>(logic_x1 / 100.0f * screen_width);
                phys_y1 = static_cast<uint16_t>(logic_y1 / 100.0f * screen_height);
                phys_x2 = static_cast<uint16_t>(logic_x2 / 100.0f * screen_width);
                phys_y2 = static_cast<uint16_t>(logic_y2 / 100.0f * screen_height);
            }

            bool containsLogical(float x, float y) const {
                return x >= logic_x1 && x <= logic_x2 && y >= logic_y1 && y <= logic_y2;
            }

            // Horizontal overlap check (for up/down neighbor search)
            bool overlapsHorizontally(const Element& other) const {
                return logic_x1 < other.logic_x2 && logic_x2 > other.logic_x1;
            }

            // Vertical overlap check (for left/right neighbor search)
            bool overlapsVertically(const Element& other) const {
                return logic_y1 < other.logic_y2 && logic_y2 > other.logic_y1;
            }
        };
    }

    namespace blk {

        class Block {
        private:
            uint32_t uni_id;

        public:
            // Logical position (percentage of screen, 0.0–100.0)
            float logic_x1, logic_y1;
            float logic_x2, logic_y2;

            // Physical position (pixels, computed)
            uint16_t phys_x1, phys_y1;
            uint16_t phys_x2, phys_y2;

            std::vector<ele::Element> elements;
            size_t focused_element_index = 0;

            Block() : logic_x1(0), logic_y1(0), logic_x2(0), logic_y2(0),
                      phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0), uni_id(0) {}

            uint32_t getID() const { return uni_id; }
            void setID(uint32_t id) { uni_id = id; }

            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                phys_x1 = static_cast<uint16_t>(logic_x1 / 100.0f * screen_width);
                phys_y1 = static_cast<uint16_t>(logic_y1 / 100.0f * screen_height);
                phys_x2 = static_cast<uint16_t>(logic_x2 / 100.0f * screen_width);
                phys_y2 = static_cast<uint16_t>(logic_y2 / 100.0f * screen_height);
                for (auto& el : elements) {
                    el.updatePhysical(screen_width, screen_height);
                }
            }

            ele::Element* getFocusedElement() {
                if (focused_element_index < elements.size())
                    return &elements[focused_element_index];
                return nullptr;
            }
        };
    }

    namespace pge {

        enum PageHeight {
            PAGE_FULL = 0,
            PAGE_HALF
        };

        class Page {
        private:
            uint32_t uni_id;

            static constexpr uint16_t STATUS_BAR_FULL = 24;   // Thin status bar for full-height pages
            static constexpr uint16_t STATUS_BAR_HALF = 48;   // Thick status bar for half-height pages

        public:
            PageHeight height_mode = PAGE_FULL;
            uint16_t status_bar_thickness = STATUS_BAR_FULL;

            std::vector<blk::Block> blocks;
            ele::Element* current_focused = nullptr;

            Page() : uni_id(0) {}

            uint32_t getID() const { return uni_id; }
            void setID(uint32_t id) { uni_id = id; }

            void switchHeight() {
                height_mode = (height_mode == PAGE_FULL) ? PAGE_HALF : PAGE_FULL;
                status_bar_thickness = (height_mode == PAGE_FULL) ? STATUS_BAR_FULL : STATUS_BAR_HALF;
            }

            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                for (auto& blk : blocks) {
                    blk.updatePhysical(screen_width, screen_height);
                }
                recalculateFocus();
            }

            // Rebuild focus neighbour pointers across ALL elements in the page
            void recalculateFocus() {
                // Collect all element pointers
                std::vector<ele::Element*> all;
                for (auto& blk : blocks) {
                    for (auto& el : blk.elements) {
                        el.focus_up    = nullptr;
                        el.focus_down  = nullptr;
                        el.focus_left  = nullptr;
                        el.focus_right = nullptr;
                        all.push_back(&el);
                    }
                }

                for (size_t i = 0; i < all.size(); ++i) {
                    ele::Element* a = all[i];
                    float best_right = 1e9f, best_left = 1e9f;
                    float best_up = 1e9f, best_down = 1e9f;

                    for (size_t j = 0; j < all.size(); ++j) {
                        if (i == j) continue;
                        ele::Element* b = all[j];

                        // Right: b is to the right of a, with vertical overlap
                        if (b->logic_x1 >= a->logic_x2 && a->overlapsVertically(*b)) {
                            float dist = b->logic_x1 - a->logic_x2;
                            if (dist < best_right) { best_right = dist; a->focus_right = b; }
                        }
                        // Left: b is to the left of a, with vertical overlap
                        if (b->logic_x2 <= a->logic_x1 && a->overlapsVertically(*b)) {
                            float dist = a->logic_x1 - b->logic_x2;
                            if (dist < best_left) { best_left = dist; a->focus_left = b; }
                        }
                        // Down: b is below a, with horizontal overlap
                        if (b->logic_y1 >= a->logic_y2 && a->overlapsHorizontally(*b)) {
                            float dist = b->logic_y1 - a->logic_y2;
                            if (dist < best_down) { best_down = dist; a->focus_down = b; }
                        }
                        // Up: b is above a, with horizontal overlap
                        if (b->logic_y2 <= a->logic_y1 && a->overlapsHorizontally(*b)) {
                            float dist = a->logic_y1 - b->logic_y2;
                            if (dist < best_up) { best_up = dist; a->focus_up = b; }
                        }
                    }
                }

                // Default focus to first element of first block if nothing focused
                if (!current_focused) {
                    if (!blocks.empty() && !blocks[0].elements.empty())
                        setFocus(&blocks[0].elements[0]);
                }
            }

            void setFocus(ele::Element* el) {
                if (current_focused) current_focused->focused = false;
                current_focused = el;
                if (current_focused) current_focused->focused = true;
            }

            bool moveFocusRight() {
                if (!current_focused || !current_focused->focus_right) return false;
                setFocus(current_focused->focus_right);
                return true;
            }
            bool moveFocusLeft() {
                if (!current_focused || !current_focused->focus_left) return false;
                setFocus(current_focused->focus_left);
                return true;
            }
            bool moveFocusUp() {
                if (!current_focused || !current_focused->focus_up) return false;
                setFocus(current_focused->focus_up);
                return true;
            }
            bool moveFocusDown() {
                if (!current_focused || !current_focused->focus_down) return false;
                setFocus(current_focused->focus_down);
                return true;
            }
        };
    }

    namespace scr {

        enum ColorMode {
            SCREEN_BW   = 0,
            SCREEN_GRAY = 1,
            SCREEN_RGB  = 2
        };

        class Screen {
        private:
            uint32_t uni_id;

        public:
            uint16_t width  = 0;
            uint16_t height = 0;
            ColorMode color_mode = SCREEN_RGB;

            Screen() : uni_id(0) {}

            uint32_t getID() const { return uni_id; }
            void setID(uint32_t id) { uni_id = id; }
        };
    }

    namespace app {

        enum ApplicationType {
            APP_NATIVE  = 0,
            APP_VM      = 1,
            APP_UNKNOWN = 2
        };

        class Application {
        private:
            uint32_t uni_id;

        public:
            ApplicationType type  = APP_UNKNOWN;
            std::string vm_path;                 // Script path for VM-type apps
            pge::Page* start_page = nullptr;     // Initial page

            // Runtime services (set before app_main)
            scr::Screen*                screen     = nullptr;
            render::Render*             renderer   = nullptr;
            ctrller::CtrllerService*    controller = nullptr;
            transor::TranslatorService* translator = nullptr;

            Application() : uni_id(0) {}

            virtual void app_main() = 0;
            virtual ~Application() = default;

            uint32_t getID() const { return uni_id; }
            void setID(uint32_t id) { uni_id = id; }
        };
    }
}

#endif //APSISUI2_UISTRUCTURE_H
