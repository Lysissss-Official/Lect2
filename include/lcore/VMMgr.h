//
// VMMgr — pocketpy VM 管理器
// ============================================================================
// 封装 pocketpy 的初始化、代码执行和销毁。
// 用于 APP_VM 类型的应用（如 Python 控制台、脚本应用）。
//

#ifndef APSISUI2_VMMGR_H
#define APSISUI2_VMMGR_H

#include <cstdlib>
#include <string>
#include "pocketpy.h"

namespace lcore {

    class VMMgr {
        py_Ref main_mod;

    public:
        VMMgr() {
            py_initialize();
            main_mod = py_getmodule("__main__");
        }

        ~VMMgr() {
            py_finalize();
        }

        // 执行一行 Python 代码。
        // 返回 true 表示成功，output 填入表达式求值结果的字符串；
        // 返回 false 表示异常，output 填入格式化的异常信息。
        bool exec(const char* code, std::string& output) {
            const char* result = nullptr;
            bool ok = py_smarteval(code, main_mod, "s", &result);

            if (ok) {
                if (result && result[0])
                    output = result;
                else
                    output.clear();
                return true;
            }

            // 异常：py_formatexc 返回 malloc 的字符串
            char* exc = py_formatexc();
            output = exc ? exc : "Unknown VM error";
            std::free(exc);
            return false;
        }
    };

} // namespace lcore

#endif //APSISUI2_VMMGR_H
