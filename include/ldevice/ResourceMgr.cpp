#include "ResourceMgr.h"

namespace ldevice {

    Resource* ResourceMgr::createResource(
        std::string name,
        std::string type
    ) {
        if (resource_name_table_.contains(name))
            return nullptr;

        auto resource = std::make_unique<Resource>();

        resource->setName(std::move(name));
        resource->setType(std::move(type));

        Resource* ptr = resource.get();

        resource_table_.emplace(
            ptr->getID(),
            std::move(resource)
        );

        resource_name_table_[ptr->getName()] = ptr->getID();

        return ptr;
    }

    Resource* ResourceMgr::getResource(uint32_t id) {
        auto it = resource_table_.find(id);

        if (it == resource_table_.end())
            return nullptr;

        return it->second.get();
    }

    Resource* ResourceMgr::getResource(
        const std::string& name
    ) {
        auto name_it = resource_name_table_.find(name);

        if (name_it == resource_name_table_.end())
            return nullptr;

        return getResource(name_it->second);
    }

    bool ResourceMgr::removeResource(uint32_t id) {
        auto it = resource_table_.find(id);

        if (it == resource_table_.end())
            return false;

        // TODO: 加入依赖检查

        resource_name_table_.erase(
            it->second->getName()
        );

        resource_table_.erase(it);

        return true;
    }

    bool ResourceMgr::removeResource(
        const std::string& name
    ) {
        auto name_it = resource_name_table_.find(name);

        if (name_it == resource_name_table_.end())
            return false;

        // TODO: 加入依赖检查
        
        return removeResource(name_it->second);
    }

    void ResourceMgr::clearResources() {
        resource_name_table_.clear();
        resource_table_.clear();
    }

}