#include "stdafx.h"

#include "Headers/Resource.h"
#include "Headers/ResourceCache.h"
#include "Core/Headers/Application.h"

namespace Divide {

//---------------------------- Resource ------------------------------------------//
Resource::Resource(ResourceType type, const Str256& resourceName)
    : GUIDWrapper(),
      _resourceType(type),
      _resourceName(resourceName),
      _resourceState(ResourceState::RES_CREATED)
{
}

ResourceState Resource::getState() const noexcept {
    return _resourceState.load(std::memory_order_relaxed);
}

void Resource::setState(ResourceState currentState) noexcept {
    _resourceState.store(currentState, std::memory_order_relaxed);
}

//---------------------------- Cached Resource ------------------------------------//
CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const Str256& resourceName)
    : CachedResource(type, descriptorHash, resourceName, "", "")
{
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const Str256& resourceName,
                               const stringImpl& assetName)
    : CachedResource(type, descriptorHash, resourceName, assetName, "")
{
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const Str256& resourceName,
                               const stringImpl& assetName,
                               const stringImpl& assetLocation)
    : Resource(type, resourceName),
      _descriptorHash(descriptorHash),
      _assetName(assetName),
      _assetLocation(assetLocation)
{
}

bool CachedResource::load() {
    setState(ResourceState::RES_LOADED);
    flushStateCallbacks();
    return true;
}

bool CachedResource::unload() {
    return true;
}

void CachedResource::addStateCallback(ResourceState targetState, const DELEGATE<void, CachedResource*>& cbk) {
    {
        UniqueLock<Mutex> w_lock(_callbackLock);
        _loadingCallbacks[to_U32(targetState)].push_back(cbk);
    }
    if (getState() == ResourceState::RES_LOADED) {
        flushStateCallbacks();
    }
}

void CachedResource::setState(ResourceState currentState) noexcept {
    Resource::setState(currentState);
    flushStateCallbacks();
}

void CachedResource::flushStateCallbacks() {
    const ResourceState currentState = getState();
    for (U8 i = 0; i < to_base(currentState) + 1; ++i) {
        const ResourceState tempState = static_cast<ResourceState>(i);
        CachedResource* ptr = nullptr;
        if (tempState != ResourceState::RES_UNKNOWN && tempState != ResourceState::RES_UNLOADING) {
            ptr = this;
        }
        UniqueLock<Mutex> r_lock(_callbackLock);
        CallbackList& cbks = _loadingCallbacks[to_U32(tempState)];
        for (auto& cbk : cbks) {
            cbk(ptr);
        }
        cbks.clear();
    }
}

};  // namespace Divide