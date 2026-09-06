//
// Created by archeart on 2026/9/6.
//

#ifndef APSISUI2_PATH_H
#define APSISUI2_PATH_H

#include <string>
#include <string_view>
#include <utility>

namespace lcore::fs {

    class Path {
    private:
        std::string path_;

    public:
        Path() = default;

        Path(const char* path)
            : path_(path ? path : "") {}

        Path(std::string path)
            : path_(std::move(path)) {}

        Path(std::string_view path)
            : path_(path) {}

        const std::string& string() const {
            return path_;
        }

        bool empty() const {
            return path_.empty();
        }

        // 纯字符串拼接
        Path operator+(std::string_view suffix) const {
            return Path(path_ + std::string(suffix));
        }

        // 路径拼接
        Path operator/(std::string_view child) const {
            if (path_.empty()) {
                return Path(std::string(child));
            }

            if (child.empty()) {
                return *this;
            }

            if (path_.back() == '/') {
                return Path(path_ + std::string(child));
            }

            return Path(path_ + "/" + std::string(child));
        }

        // 返回上一级
        Path& operator--() {
            if (path_.empty() || path_ == "/") {
                return *this;
            }

            const auto pos = path_.find_last_of('/');

            if (pos == std::string::npos) {
                path_.clear();
            }
            else if (pos == 0) {
                path_ = "/";
            }
            else {
                path_.erase(pos);
            }

            return *this;
        }
    };

}

#endif //APSISUI2_PATH_H
