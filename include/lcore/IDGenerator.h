//
// Created by archeart on 2026/7/14.
//

#ifndef APSISUI2_IDGENERATOR_H
#define APSISUI2_IDGENERATOR_H

#include <cstdint>
#include <random>
#include <set>
#include <atomic>

namespace lcore {
    template<typename Tag>
    class IDGenerator {
    private:
        // ID 计数器 从 1 开始
        static std::atomic<uint32_t>& counter() {
            static std::atomic<uint32_t> value{1};
            return value;
        }

    public:
        static uint32_t generate() {
            uint32_t current =
                counter().load(std::memory_order_relaxed);

            while (current != 0) {
                uint32_t next = current + 1;

                if (counter().compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                )) {
                    return current;
                }
            }

            return 0; // 0 是非法 ID！
        }

        static void release(uint32_t) {
            // 兼容接口 顺序分发 不回收ID
        }
    };

    template<typename Tag>
    class IDGeneratorLegacy {
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
