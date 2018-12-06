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

Resource::~Resource()
{
}

/// Name management
const stringImpl& Resource::resourceName() const {
    return _resourceName;
}

ResourceType Resource::getType() const {
    return _resourceType;
}

ResourceState Resource::getState() const {
    return _resourceState;
}

void Resource::setState(ResourceState currentState) {
    _resourceState = currentState;
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

CachedResource::~CachedResource()
{
}

bool CachedResource::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    setState(ResourceState::RES_LOADED);
    if (onLoadCallback) {
        onLoadCallback(shared_from_this());
    }

    return resourceLoadComplete();
}

bool CachedResource::unload() {
    return true;
}

bool CachedResource::resourceLoadComplete() {
    return true;
}

size_t CachedResource::getDescriptorHash() const {
    return _descriptorHash;
}

/// Physical file path
const stringImpl& CachedResource::assetLocation() const {
    return _assetLocation;
}

void CachedResource::assetLocation(const stringImpl& location) {
    _assetLocation = location;
}

/// Physical file name
const stringImpl& CachedResource::assetName() const {
    return _assetName;
}

void CachedResource::assetName(const stringImpl& name) {
    _assetName = name;
}

void CachedResource::setStateCallback(ResourceState targetState, const DELEGATE_CBK<void, Resource_wptr>& cbk) {
    UniqueLock w_lock(_callbackLock);
    _loadingCallbacks[to_U32(targetState)] = cbk;
}

void CachedResource::setState(ResourceState currentState) {
    Resource::setState(currentState);

    UniqueLock r_lock(_callbackLock);
    const DELEGATE_CBK<void, Resource_wptr>& cbk = _loadingCallbacks[to_U32(currentState)];
    if (cbk) {
        cbk(shared_from_this());
        _loadingCallbacks[to_U32(currentState)] = DELEGATE_CBK<void, Resource_wptr>();
    }
}

};  // namespace Divide