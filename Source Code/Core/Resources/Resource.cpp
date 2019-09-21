#include "stdafx.h"

#include "Headers/Resource.h"
#include "Headers/ResourceCache.h"
#include "Core/Headers/Application.h"

namespace Divide {

//---------------------------- Resource ------------------------------------------//
Resource::Resource(ResourceType type,
                   const stringImpl& resourceName)
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
                               const stringImpl& resourceName)
    : CachedResource(type, descriptorHash, resourceName, "", "")
{
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const stringImpl& resourceName,
                               const stringImpl& assetName)
    : CachedResource(type, descriptorHash, resourceName, assetName, "")
{
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const stringImpl& resourceName,
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
    return true;
}

bool CachedResource::unload() noexcept {
    return true;
}

size_t CachedResource::getDescriptorHash() const noexcept {
    return _descriptorHash;
}

/// Physical file path
const stringImpl& CachedResource::assetLocation() const noexcept {
    return _assetLocation;
}

void CachedResource::assetLocation(const stringImpl& location) {
    _assetLocation = location;
}

/// Physical file name
const stringImpl& CachedResource::assetName() const noexcept {
    return _assetName;
}

void CachedResource::assetName(const stringImpl& name) {
    _assetName = name;
}

void CachedResource::setStateCallback(ResourceState targetState, const DELEGATE_CBK<void, Resource_wptr>& cbk) {
    UniqueLock w_lock(_callbackLock);
    _loadingCallbacks[to_U32(targetState)] = cbk;
}

void CachedResource::setState(ResourceState currentState) noexcept {
    Resource::setState(currentState);

    UniqueLock r_lock(_callbackLock);
    const DELEGATE_CBK<void, Resource_wptr>& cbk = _loadingCallbacks[to_U32(currentState)];
    if (cbk) {
        cbk(shared_from_this());
        _loadingCallbacks[to_U32(currentState)] = DELEGATE_CBK<void, Resource_wptr>();
    }
}

};  // namespace Divide