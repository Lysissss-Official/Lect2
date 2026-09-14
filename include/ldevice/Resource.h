//
// Created by archeart on 2026/9/14.
//

#ifndef APSISUI2_RESOURCE_H
#define APSISUI2_RESOURCE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "lcore/IDGenerator.h"

// TODO: Resource 用于替代 Device class

namespace ldevice {

    class Resource {
    private:
        uint32_t uni_id_;
        std::string name_;
        std::string type_;

        std::unordered_map<std::string, Resource*> dependencies_;

        using InfoValue = std::variant<
            bool,
            int64_t,
            uint64_t,
            double,
            std::string
        >;

        std::unordered_map<std::string, InfoValue> info_;

    public:
        Resource() : uni_id_(lcore::IDGenerator<Resource>::generate()) {
            ;
        }

        ~Resource() {
            lcore::IDGenerator<Resource>::release(uni_id_);
        }

        uint32_t getID() const {
            return uni_id_;
        }

        // ======== DEPENDENCIES ========

        void setDependency(std::string name, Resource* resource) {
            dependencies_[std::move(name)] = resource;
        }

        Resource* getDependency(const std::string& name) const {
            auto it = dependencies_.find(name);
            return it == dependencies_.end() ? nullptr : it->second;
        }

        const std::unordered_map<std::string, Resource*>& getDependencies() const {
            return dependencies_;
        }

        bool removeDependency(const std::string& name) {
            return dependencies_.erase(name) != 0;
        }

        void clearDependencies() {
            dependencies_.clear();
        }

        // ======== INFO ========

        template<typename T>
        void setInfo(std::string name, T&& value) {
            info_[std::move(name)] = std::forward<T>(value);
        }

        template<typename T>
        const T* getInfo(const std::string& name) const {
            auto it = info_.find(name);
            if (it == info_.end())
                return nullptr;

            return std::get_if<T>(&it->second);
        }

        bool removeInfo(const std::string& name) {
            return info_.erase(name) != 0;
        }

        void clearInfo() {
            info_.clear();
        }

        // ======== NAME ========

        const std::string& getName() const {
            return name_;
        };

        void setName(std::string name) {
            name_ = std::move(name);
        };

        // ======== TYPE ========

        const std::string& getType() const {
            return type_;
        };

        void setType(std::string type) {
            type_ = std::move(type);
        };

        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;
    };

}

#endif //APSISUI2_RESOURCE_H
