//
// Created by archeart on 2026/7/14.
//

#ifndef APSISUI2_IDGENERATOR_H
#define APSISUI2_IDGENERATOR_H

#include <cstdint>
#include <random>
#include <set>

namespace lcore {
    template<typename Tag>
    class IDGenerator {
    private:
        // 每个 Tag 实例化一份独立 registry（静态局部变量）
        static std::set<uint32_t>& registry() {
            static std::set<uint32_t> s;
            return s;
        }

    public:
        static uint32_t generate() {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);

            auto& used = registry();
            uint32_t id = dist(rng);
            while (used.count(id)) {
                id++;
                if (id == 0) id = 1;
            }
            used.insert(id);
            return id;
        }

        static void release(uint32_t id) {
            registry().erase(id);
        }
    };
}

#endif //APSISUI2_IDGENERATOR_H
