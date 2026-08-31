//
// Created by archeart on 2026/8/16.
//

#ifndef APSISUI2_UITHEME_H
#define APSISUI2_UITHEME_H

#include "UIStructure.h"

namespace lui::theme {
    class Theme {
    public:
        virtual ~Theme() = default;
        virtual bool drawFuncCall( lui::DrawContext& context ) = 0;
        virtual float timeFuncCall(
            std::chrono::time_point<std::chrono::steady_clock> start,
            std::chrono::time_point<std::chrono::steady_clock> end,
            std::chrono::time_point<std::chrono::steady_clock> now
            )
        {
            return 1.0f;
        }
    };
}



#endif //APSISUI2_UITHEME_H
