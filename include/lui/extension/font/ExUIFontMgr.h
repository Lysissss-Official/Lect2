//
// Created by Admin on 2026/9/20.
//

#ifndef APSISUI2_EXUIFONTMGR_H
#define APSISUI2_EXUIFONTMGR_H

// TODO: 迁移 Font 管理器

#include "lcore/Path.h"
#include "lui/extension/font/ExUIFont.h"
#include "lcore/Log.h"

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace lui::ext::font {
    class FontMgr {
    private:
        std::unordered_map<uint32_t, std::unique_ptr<Font>> fonts_;
        std::unordered_map<std::string, uint32_t> path_table_;

    public:
        lcore::fs::Path registerFont(std::unique_ptr<Font> font, lcore::fs::Path path = "") {
            if (!font) {
                return "";
            }

            if (path.string() == "") {
                path = "/sys/font/" + font->getName()
                                 + "/" + font->getStyle()
                                 + "/" + std::to_string(font->getSize());
            }

            if (path_table_.find(path.string()) != path_table_.end())
                return "";

            const uint32_t id = font->getID();

            fonts_.emplace(id, std::move(font));
            path_table_.emplace(path.string(),id);

            return path;
        }

        // 根据 Path 找 Font (RO)
        const Font* getFont(const lcore::fs::Path& path) const {
            auto path_it = path_table_.find(path.string());

            if (path_it == path_table_.end())
                return nullptr;

            auto font_it = fonts_.find(path_it->second);

            if (font_it == fonts_.end())
                return nullptr;

            return font_it->second.get();
        }
    };
}

#endif //APSISUI2_EXUIFONTMGR_H
