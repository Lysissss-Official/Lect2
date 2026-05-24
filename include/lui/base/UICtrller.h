//
// Created by Shoiin on 2026/5/19.
//

#ifndef APSISUI2_UICTRLLER_H
#define APSISUI2_UICTRLLER_H

#include <cstdint>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "lcore/Log.h"

namespace lui {

        struct KeyboardState {
            std::vector<uint8_t> key;
        };

        struct SwitchState {
            float degree = -1.0f;   // Joystick angle: 0=right, 90=up, 180=left, 270=down, -1=neutral
        };

        class CtrllerService {
        protected:
            KeyboardState kb_state;
            SwitchState   sw_state;

        public:
            uint8_t getKB(uint16_t vk_code) {
                std::shared_lock lock(state_mutex_);
                if (vk_code < kb_state.key.size()) {
                    return kb_state.key[vk_code];
                }
                return 2;   // Out of range
            }

            float getSW() {
                std::shared_lock lock(state_mutex_);
                return sw_state.degree;
            }

            virtual void refreshStatus() = 0;

            // ---- 守护线程生命周期 ----

            void startDaemon() {
                if (daemon_running_.load(std::memory_order_acquire)) return;
                LOG("Ctrller daemon starting");
                daemon_running_.store(true, std::memory_order_release);
                daemon_thread_ = std::thread(&CtrllerService::daemonLoop, this);
            }

            void stopDaemon() {
                LOG("Ctrller daemon stopping");
                daemon_running_.store(false, std::memory_order_release);
                if (daemon_thread_.joinable()) {
                    daemon_thread_.join();
                    LOG("Ctrller daemon stopped");
                }
            }

        private:
            std::shared_mutex state_mutex_;
            std::thread daemon_thread_;
            std::atomic<bool> daemon_running_{false};

            void daemonLoop() {
                using namespace std::chrono;

                while (daemon_running_.load(std::memory_order_acquire)) {
                    {
                        std::unique_lock lock(state_mutex_);
                        refreshStatus();
                    }
                    std::this_thread::sleep_for(milliseconds(16));
                }
            }
        };
}

#endif //APSISUI2_UICTRLLER_H
