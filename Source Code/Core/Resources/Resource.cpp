#include "stdafx.h"

#include "Headers/Resource.h"
#include "Headers/ResourceCache.h"
#include "Core/Headers/Application.h"

namespace Divide {

//---------------------------- Resource ------------------------------------------//
Resource::Resource(ResourceType type,
                   const stringImpl& name)
    : GUIDWrapper(),
      _resourceType(type),
      _name(name),
      _resourceState(ResourceState::RES_CREATED)
{
}

Resource::~Resource()
{
}

/// Name management
const stringImpl& Resource::name() const {
    return _name;
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
                               const stringImpl& name)
    : Resource(type, name),
      _descriptorHash(descriptorHash)
{
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const stringImpl& name,
                               const stringImpl& resourceName)
    : CachedResource(type, descriptorHash, name)
{
    _resourceName = resourceName;
}

CachedResource::CachedResource(ResourceType type,
                               size_t descriptorHash,
                               const stringImpl& name,
                               const stringImpl& resourceName,
                               const stringImpl& resourceLocation)
    : CachedResource(type, descriptorHash, name, resourceName)
{
    _resourceLocation = resourceLocation;
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
const stringImpl& CachedResource::getResourceLocation() const {
    return _resourceLocation;
}

void CachedResource::setResourceLocation(const stringImpl& location) {
    _resourceLocation = location;
}

/// Physical file name
const stringImpl& CachedResource::getResourceName() const {
    return _resourceName;
}

void CachedResource::setResourceName(const stringImpl& name) {
    _resourceName = name;
}

void CachedResource::setStateCallback(ResourceState targetState, const DELEGATE_CBK<void, Resource_wptr>& cbk) {
    WriteLock w_lock(_callbackLock);
    _loadingCallbacks[to_U32(targetState)] = cbk;
}

void CachedResource::setState(ResourceState currentState) {
    Resource::setState(currentState);

    ReadLock r_lock(_callbackLock);
    const DELEGATE_CBK<void, Resource_wptr>& cbk = _loadingCallbacks[to_U32(currentState)];
    if (cbk) {
        cbk(shared_from_this());
        _loadingCallbacks[to_U32(currentState)] = DELEGATE_CBK<void, Resource_wptr>();
    }
}

};  // namespace Divide