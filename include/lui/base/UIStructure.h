//
// Created by Admin on 2026/5/19.
//

#ifndef APSISUI2_UISTRUCTURE_H
#define APSISUI2_UISTRUCTURE_H
#include <cstdint>
#include <string>

namespace lui {
    namespace ele {
        class Element {
        private:
            uint32_t uni_id;
        public:
            Element() {
                uni_id = 0;
            }
        };
    }

    namespace blk {
        class Block {
        private:
            uint32_t uni_id;
        public:
            Block() {
                uni_id = 0;
            }
        };
    }

    namespace pge {
        class Page {
        private:
            uint32_t uni_id;
        public:
            Page() {
                uni_id = 0;
            }
        };
    }

    namespace scr {
        enum ColorMode {
            SCREEN_BW = 0,
            SCREEN_GRAY,
            SCREEN_RGB
        };

        class Screen {
        private:
            uint32_t uni_id;
        public:
            uint16_t width;
            uint16_t height;
            ColorMode color_mode;

            uint32_t getID() const
            {
                return uni_id;
            }
            void setID(uint32_t id)
            {
                uni_id = id;
            }

            Screen()
            {
                uni_id = 0;
                width = 0;
                height = 0;
                color_mode = SCREEN_RGB;
            }
        };
    }

    namespace app {
        enum ApplicationType {
            APP_NATIVE = 0,
            APP_VM,
            APP_UNKNOWN
        };

        class Application {
        private:
            uint32_t uni_id;
        public:
            ApplicationType type;
            std::string vm_path;

            virtual void app_main() = 0;
            virtual ~Application() = default;

            uint32_t getID() const
            {
                return uni_id;
            }
            void setID(uint32_t id)
            {
                uni_id = id;
            }

            Application()
            {
                uni_id = 0;
                type = APP_UNKNOWN;
            }
        };
    }
}

#endif //APSISUI2_UISTRUCTURE_H
