//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UIRENDER_H
#define APSISUI2_UIRENDER_H
#include <cstdint>
#include <list>
#include <queue>

namespace lui {
    namespace render {
        class Render {
        protected:
            struct RenderRequest {
                uint32_t cpu_time_start;
                uint32_t cpu_time_use;
                struct RenderRequestData {
                    enum RenderRequestDataType {
                        CHANGE_ELEMENT = 1,
                        CHANGE_PAGE = 2
                    };
                    RenderRequestDataType type;
                    std::vector<std::pair<std::string, uint32_t>> args;
                    uint32_t anime_func;
                    uint32_t target_id;
                };
            };
            std::queue<RenderRequest> render_requests_pre;  // 等待渲染队列
            std::list<RenderRequest> render_requests_now;   // 正在渲染项

        public:
            virtual void renderService() {
                throw std::runtime_error("LUI Render: Attempted to call renderService(), but it is not implemented.");
            };
        };
    }
}

#endif //APSISUI2_UIRENDER_H
