#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H

#include <cstdint>
#include <mutex>
#include <random>
#include <set>
#include <string>
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
        ~Screen() {
            IDGenerator<Screen>::release(uni_id);
        }
    };

    class Page {
    private:
        uint32_t uni_id;
        std::vector<uint32_t> params;
        std::vector<Element*> children;

    public:
        mutable std::recursive_mutex state_mutex;
        Element* focus;

        uint32_t getID() const {
            return uni_id;
        }
        bool setFocus(Element* el) {
            if (el == nullptr || el == focus) {
                return false;
            }
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            if (focus) focus->focused = false;
            focus = el;
            if (focus) {
                focus->focused = true;
                //scrollToShow(focus);
            }
            return true;
        }
        Page() : focus(nullptr){
            uni_id = IDGenerator<Page>::generate();
        }
        ~Page() {
            IDGenerator<Page>::release(uni_id);
        }
    };

    class Element {
    private:
        uint32_t uni_id;  // 全局唯一 ID
        std::vector<uint32_t> params;
        std::vector<Element*> children;
        std::variant<Page*, Element*> father;

    public:
        bool focused;
        uint32_t& getParam(ParamIndex idx) {
            return params[static_cast<size_t>(idx)];
        }

        const uint32_t& getParam(ParamIndex idx) const {
            return params[static_cast<size_t>(idx)];
        }

        uint32_t getID() const {
            return uni_id;
        }

        Element() : focused(false){
            uni_id = IDGenerator<Element>::generate();
            params.resize(16);
        }
        ~Element() {
            IDGenerator<Element>::release(uni_id);
        }
    };
}

#endif //APSISUI2_UISTRUCTURE_H
