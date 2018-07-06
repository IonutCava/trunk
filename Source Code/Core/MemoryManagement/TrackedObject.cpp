#include "stdafx.h"

#include "Headers/TrackedObject.h"

namespace Divide {

TrackedObject::TrackedObject() noexcept
    : GUIDWrapper()
{
    /// On creation, it only has a reference count of 1
    _refCount.store(1);
}

TrackedObject::~TrackedObject() {
    /// Never call release here!.
    /// The smart pointer handling the object should call release upon delete
    /// until refCount reaches 0
    /// Only when refCount is 0 should the smart pointer delete the object
    _dependencyList.clear();
}

void TrackedObject::AddRef() {
    /// For each object in our dependency map
    for (TrackedObject* obj : _dependencyList) {
        /// We should never have a null object in our dependency map
        assert(obj != nullptr);
        /// Increase it's ref count
        obj->AddRef();
    }

    /// increase our ref count
    _refCount.fetch_add(1u);
}

bool TrackedObject::SubRef() {
    assert(_refCount.load() > 0);

    /// For each object in our dependency map
    std::list<TrackedObject*>::iterator it;
    for (it = std::begin(_dependencyList); it != std::end(_dependencyList);) {
        /// We should never have a null object in our dependency map
        assert(*it != nullptr);
        /// subtract ref count for every sub object
        if ((*it)->SubRef()) {
            /// if it's no longer used, remove it from the dependecy list
            /// it should be scheduled for deletion anyway
            it = _dependencyList.erase(it);
        } else {
            ++it;
        }
    }

    return _refCount.fetch_sub(1u) <= 1;
}

void TrackedObject::REGISTER_TRACKED_DEPENDENCY(TrackedObject* const obj) {
    assert(obj != nullptr);
    /// Some dependencies may be loaded later, so add null ones as well
    _dependencyList.push_back(obj);
}

void TrackedObject::UNREGISTER_TRACKED_DEPENDENCY(TrackedObject* const obj) {
    if (!obj) {
        return;
    }
    I64 targetGUID = obj->getGUID();
    _dependencyList.erase(
        std::remove_if(std::begin(_dependencyList), std::end(_dependencyList),
                       [&targetGUID](TrackedObject* tObj)
                           -> bool { return tObj->getGUID() == targetGUID; }),
        std::end(_dependencyList));
}

};  // namespace Divide