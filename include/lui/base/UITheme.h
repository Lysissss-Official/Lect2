//
// Created by archeart on 2026/8/16.
//

#ifndef APSISUI2_UITHEME_H
#define APSISUI2_UITHEME_H

#include "UIRender.h"



namespace lui::theme {
    class Theme {
    public:
        virtual ~Theme() = default;
        virtual bool drawFuncCall( Render::DrawContext& context ) = 0;
    };
}



#endif //APSISUI2_UITHEME_H
