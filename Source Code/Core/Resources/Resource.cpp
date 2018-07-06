#include "Headers/Resource.h"
#include "Headers/ResourceCache.h"
#include "Core/Headers/Application.h"

namespace Divide {

Resource::Resource(ResourceType type,
                   const stringImpl& name)
    : GUIDWrapper(),
      _resourceType(type),
      _name(name),
      _resourceState(ResourceState::RES_CREATED)
{
    _loadingCallbacks.fill(DELEGATE_CBK<>());
}

Resource::Resource(ResourceType type, 
                   const stringImpl& name,
                   const stringImpl& resourceLocation)
    : Resource(type, name)
{
    _resourceLocation = resourceLocation;
}

Resource::~Resource()
{
}

bool Resource::load() {
    setState(ResourceState::RES_LOADED);
    return true;
}

bool Resource::unload() {
    return true;
}

/// Name management
const stringImpl& Resource::getName() const {
    return _name;
}

/// Physical file location
const stringImpl& Resource::getResourceLocation() const {
    return _resourceLocation;
}

void Resource::setResourceLocation(const stringImpl& location) {
    _resourceLocation = location;
}

ResourceState Resource::getState() const {
    return _resourceState;
}

ResourceType Resource::getType() const {
    return _resourceType;
}

void Resource::setStateCallback(ResourceState targetState, DELEGATE_CBK<void> cbk) {
    WriteLock w_lock(_callbackLock);
    _loadingCallbacks[to_uint(targetState)] = cbk;
}

void Resource::setState(ResourceState currentState) {
    _resourceState = currentState;

    ReadLock r_lock(_callbackLock);
    DELEGATE_CBK<void>& cbk = _loadingCallbacks[to_uint(currentState)];
    if (cbk) {
        cbk();
    }
}

};  // namespace Divide