#include "Headers/Resource.h"

#include "Core/Headers/Application.h"

namespace Divide {
Resource::Resource(const stringImpl& name)
    : TrackedObject(),
        _name(name),
        _resourceState(ResourceState::RES_CREATED)
{
}

Resource::Resource(const stringImpl& name,
                   const stringImpl& resourceLocation)
    : Resource(name)
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

ResourceState Resource::getState() const {
    return _resourceState;
}

void Resource::setStateCallback(ResourceState targetState, DELEGATE_CBK<void> cbk) {
    std::lock_guard<std::mutex> lock(_callbackLock);
    _loadingCallbacks[to_uint(targetState)] = cbk;
}

void Resource::setState(ResourceState currentState) {
    _resourceState = currentState;
    std::lock_guard<std::mutex> lock(_callbackLock);
    DELEGATE_CBK<void>& cbk = _loadingCallbacks[to_uint(currentState)];
    if (cbk) {
        cbk();
    }
}

};  // namespace Divide