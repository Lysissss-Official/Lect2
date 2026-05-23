//
// Created by Shoiin / archeart on 2026/5/19.
//

#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H

#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace lui {

    // =========================================================================
    // Auto ID generator — per-type tag to keep id spaces independent.
    // Uses random seed + linear probe on collision.
    // =========================================================================
    template<typename Tag>
    class IDGenerator {
    private:
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

    // Forward declarations for cross-namespace references
    namespace render { class Render; }
    namespace ctrller { class CtrllerService; }
    namespace transor { class TranslatorService; }

    namespace ele {

        enum ElementType {
            ELE_TEXTBOX = 0,
            ELE_BUTTON,
            ELE_LIST,
            ELE_EMPTY
        };

        class Element {
        private:
            uint32_t uni_id;

        public:
            ElementType type;

            // Logical position (percentage of screen, 0.0–100.0)
            float logic_x1, logic_y1;
            float logic_x2, logic_y2;

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
                        phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0) {
                uni_id = IDGenerator<Element>::generate();
            }

            ~Element() { IDGenerator<Element>::release(uni_id); }

            uint32_t getID() const { return uni_id; }

            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                phys_x1 = static_cast<uint16_t>(logic_x1 / 100.0f * screen_width);
                phys_y1 = static_cast<uint16_t>(logic_y1 / 100.0f * screen_height);
                phys_x2 = static_cast<uint16_t>(logic_x2 / 100.0f * screen_width);
                phys_y2 = static_cast<uint16_t>(logic_y2 / 100.0f * screen_height);
            }

            bool containsLogical(float x, float y) const {
                return x >= logic_x1 && x <= logic_x2 && y >= logic_y1 && y <= logic_y2;
            }

            bool overlapsHorizontally(const Element& other) const {
                return logic_x1 < other.logic_x2 && logic_x2 > other.logic_x1;
            }

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
                      phys_x1(0), phys_y1(0), phys_x2(0), phys_y2(0) {
                uni_id = IDGenerator<Block>::generate();
            }

            ~Block() { IDGenerator<Block>::release(uni_id); }

            uint32_t getID() const { return uni_id; }

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

            static constexpr uint16_t STATUS_BAR_FULL = 24;
            static constexpr uint16_t STATUS_BAR_HALF = 48;
            static constexpr uint16_t SCROLL_MARGIN   = 12;

        public:
            PageHeight height_mode = PAGE_FULL;
            uint16_t status_bar_thickness = STATUS_BAR_FULL;

            std::vector<blk::Block> blocks;
            ele::Element* current_focused = nullptr;

            // Scroll support
            float    scroll_y    = 0.0f;
            uint16_t viewport_h  = 0;
            float    max_scroll  = 0.0f;

            Page() { uni_id = IDGenerator<Page>::generate(); }
            ~Page() { IDGenerator<Page>::release(uni_id); }

            uint32_t getID() const { return uni_id; }

            void switchHeight() {
                height_mode = (height_mode == PAGE_FULL) ? PAGE_HALF : PAGE_FULL;
                status_bar_thickness = (height_mode == PAGE_FULL) ? STATUS_BAR_FULL : STATUS_BAR_HALF;
            }

            void updatePhysical(uint16_t screen_width, uint16_t screen_height) {
                viewport_h = screen_height;
                for (auto& blk : blocks) {
                    blk.updatePhysical(screen_width, screen_height);
                }
                recomputeMaxScroll();
                recalculateFocus();
            }

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
                max_scroll = bottom - viewport_h + status_bar_thickness;
                if (max_scroll < 0.0f) max_scroll = 0.0f;
                if (scroll_y > max_scroll) scroll_y = max_scroll;
                if (scroll_y < 0.0f) scroll_y = 0.0f;
            }

            void scrollToShow(ele::Element* el) {
                if (!el) return;

                float screen_y1 = static_cast<float>(el->phys_y1) - scroll_y;
                float screen_y2 = static_cast<float>(el->phys_y2) - scroll_y;

                float top_margin    = static_cast<float>(status_bar_thickness) + SCROLL_MARGIN;
                float bottom_margin = static_cast<float>(viewport_h) - SCROLL_MARGIN;

                if (screen_y1 < top_margin) {
                    scroll_y -= (top_margin - screen_y1);
                } else if (screen_y2 > bottom_margin) {
                    scroll_y += (screen_y2 - bottom_margin);
                }

                if (scroll_y < 0.0f) scroll_y = 0.0f;
                if (scroll_y > max_scroll) scroll_y = max_scroll;
            }

            // Rebuild focus neighbour pointers across ALL elements in the page
            void recalculateFocus() {
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

                        if (b->logic_x1 >= a->logic_x2 && a->overlapsVertically(*b)) {
                            float dist = b->logic_x1 - a->logic_x2;
                            if (dist < best_right) { best_right = dist; a->focus_right = b; }
                        }
                        if (b->logic_x2 <= a->logic_x1 && a->overlapsVertically(*b)) {
                            float dist = a->logic_x1 - b->logic_x2;
                            if (dist < best_left) { best_left = dist; a->focus_left = b; }
                        }
                        if (b->logic_y1 >= a->logic_y2 && a->overlapsHorizontally(*b)) {
                            float dist = b->logic_y1 - a->logic_y2;
                            if (dist < best_down) { best_down = dist; a->focus_down = b; }
                        }
                        if (b->logic_y2 <= a->logic_y1 && a->overlapsHorizontally(*b)) {
                            float dist = a->logic_y1 - b->logic_y2;
                            if (dist < best_up) { best_up = dist; a->focus_up = b; }
                        }
                    }
                }

                if (!current_focused) {
                    if (!blocks.empty() && !blocks[0].elements.empty())
                        setFocus(&blocks[0].elements[0]);
                }
            }

            void setFocus(ele::Element* el) {
                if (current_focused) current_focused->focused = false;
                current_focused = el;
                if (current_focused) {
                    current_focused->focused = true;
                    scrollToShow(current_focused);
                }
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

            Screen() { uni_id = IDGenerator<Screen>::generate(); }
            ~Screen() { IDGenerator<Screen>::release(uni_id); }

            uint32_t getID() const { return uni_id; }
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
            std::string vm_path;
            pge::Page* start_page = nullptr;

            scr::Screen*                screen     = nullptr;
            render::Render*             renderer   = nullptr;
            ctrller::CtrllerService*    controller = nullptr;
            transor::TranslatorService* translator = nullptr;

            Application() { uni_id = IDGenerator<Application>::generate(); }
            virtual ~Application() { IDGenerator<Application>::release(uni_id); }

            virtual void app_main() = 0;

            uint32_t getID() const { return uni_id; }
        };
    }
}

#endif //APSISUI2_UISTRUCTURE_H
