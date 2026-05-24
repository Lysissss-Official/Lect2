//
// 线程安全日志工具 — 格式: [HH:MM:SS.mmm] FUNCTION: message
//

#ifndef APSISUI2_LOG_H
#define APSISUI2_LOG_H

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace lcore {
namespace log {

    inline void write(const char* func, const std::string& msg) {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto t   = system_clock::to_time_t(now);
        auto ms  = duration_cast<milliseconds>(
                       now.time_since_epoch()) % 1000;

        std::tm tm_buf;
#if defined(_WIN32) || defined(__MINGW32__)
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif

        // 整条日志一次输出，避免多线程交错
        static std::mutex log_mtx;
        std::lock_guard lock(log_mtx);

        std::printf("[%02d:%02d:%02d.%03lld] %s: %s\n",
                    tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                    static_cast<long long>(ms.count()),
                    func, msg.c_str());
        std::fflush(stdout);
    }

} // namespace log
} // namespace lcore

#define LOG(msg) lcore::log::write(__func__, msg)

#endif //APSISUI2_LOG_H
