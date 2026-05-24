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

        // 执行一行 Python 代码（REPL 风格）。
        // 先尝试作为表达式求值（py_eval），失败则作为语句执行（EXEC_MODE）。
        // 返回 true 表示成功，output 填入结果字符串或 "OK"；
        // 返回 false 表示异常，output 填入格式化的异常信息。
        bool exec(const char* code, std::string& output) {
            // 先尝试表达式求值（如 1+1, print("hi")）
            bool ok = py_eval(code, main_mod);

            if (!ok) {
                // 表达式失败 → 可能是语句（如 x=5），清异常后改用 EXEC_MODE
                py_clearexc(NULL);
                ok = py_exec(code, "<stdin>", EXEC_MODE, main_mod);
            }

            if (ok) {
                // None 结果（如赋值语句）显示 OK
                if (py_isnone(py_retval())) {
                    output = "OK";
                    return true;
                }
                // 将返回值转为字符串：先拷贝 py_retval() 到栈上，
                // 再对栈上副本调用 py_str()（py_retval 不可作入参）
                py_TValue saved = *py_retval();
                py_str(&saved);
                const char* s = py_tostr(py_retval());
                output = s ? s : "";
                return true;
            }

            // 执行失败 → 格式化异常信息
            char* exc = py_formatexc();
            output = exc ? exc : "Unknown VM error";
            std::free(exc);
            py_clearexc(NULL);
            return false;
        }
    };

} // namespace lcore

#endif //APSISUI2_VMMGR_H
