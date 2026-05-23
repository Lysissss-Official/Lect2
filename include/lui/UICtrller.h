//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UICTRLLER_H
#define APSISUI2_UICTRLLER_H

#include <stdint.h>

namespace lui {
    namespace ctrller {
        class CtrllerService {
        private:
            bool keyboard_status[100];
            double swtich_degree;
        public:
            bool getKB(uint16_t no) {return keyboard_status[no];}
            double getSW(){return swich_degree;}
            virtual refreshStatus() = 0;
        };
    }
}

#endif //APSISUI2_UICTRLLER_H
