//
// Created by archeart on 2026/9/14.
//

#ifndef APSISUI2_RESOURCEMGR_H
#define APSISUI2_RESOURCEMGR_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "Resource.h"

namespace ldevice {

    class ResourceMgr {

    private:
        std::map<uint32_t, std::unique_ptr<Resource>> resource_table_;
        std::map<std::string, uint32_t> resource_name_table_;

    public:
        ResourceMgr() = default;
        ~ResourceMgr() = default;

        // ---- Resource 操作 ----

        Resource* createResource(
            std::string name,
            std::string type
        );

        Resource* getResource(
            uint32_t id
        );

        Resource* getResource(
            const std::string& name
        );

        bool removeResource(
            uint32_t id
        );

        bool removeResource(
            const std::string& name
        );

        void clearResources();

        const std::map<uint32_t, std::unique_ptr<Resource>>&
        getResources() const {
            return resource_table_;
        }
    };

}

#endif //APSISUI2_RESOURCEMGR_H
