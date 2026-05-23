//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UIRENDER_H
#define APSISUI2_UIRENDER_H

#include <cstdint>
#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>

#include "UIStructure.h"

namespace lui {
    namespace render {

        class Render {
        protected:
            enum RequestType {
                REQ_CHANGE_ELEMENT = 1,
                REQ_CHANGE_PAGE    = 2,
                REQ_DRAW_ELEMENT   = 3
            };

            struct RenderRequest {
                uint32_t cpu_time_start = 0;
                uint32_t cpu_time_use   = 0;
                RequestType type        = REQ_CHANGE_PAGE;
                uint32_t target_id      = 0;
                uint32_t anime_func     = 0;
                std::vector<std::pair<std::string, uint32_t>> args;
            };

            std::queue<RenderRequest> render_requests_pre;
            std::list<RenderRequest>  render_requests_now;

        public:
            virtual void renderService() {
                throw std::runtime_error(
                    "LUI Render: renderService() not implemented.");
            }

            virtual void renderPage(pge::Page* page) {
                throw std::runtime_error(
                    "LUI Render: renderPage() not implemented.");
            }
        };
    }
}

#endif //APSISUI2_UIRENDER_H
