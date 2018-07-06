#include "Headers/TrackedObject.h"
#include <cassert>

TrackedObject::TrackedObject() : _refCount(1)
{
    ///On creation, it only has a reference count of 1
}

TrackedObject::~TrackedObject(){
    ///Never call release here!.
    ///The smart pointer handling the object should call release upon delete until refCount reaches 0
    ///Only when refCount is 0 should the smart pointer delete the object
    _dependencyList.clear();
}

void TrackedObject::AddRef() {
    /// For each object in our dependency map
    std::list<TrackedObject *>::iterator it;
    for(it = _dependencyList.begin(); it != _dependencyList.end(); ++it) {
        /// We should never have a null object in our dependency map
        assert(*it != nullptr);
        /// Increase it's ref count
        (*it)->AddRef();
    }

    /// increase our ref count
    ++_refCount;
    refModifyCallback(true);
}

bool TrackedObject::SubRef(bool recursive) {
    /// For each object in our dependency map
    std::list<TrackedObject *>::iterator it;
    if(recursive)
        for(it = _dependencyList.begin(); it != _dependencyList.end(); ) {
            /// We should never have a null object in our dependency map
            assert(*it != nullptr);
            /// substract ref count for every sub object on the first tier only!
            if((*it)->SubRef(false)){
               /// if it's no longer used, remove it from the dependecy list
               /// it should be scheduled for deletion anyway
               it = _dependencyList.erase(it);
            }else{
                ++it;
            }
        }
    --_refCount;
    refModifyCallback(false);
    return (_refCount <= 0);

}

void TrackedObject::addDependency(TrackedObject* const obj){
    ///Some dependecies may be loaded later, so add null ones as well
    _dependencyList.push_back(obj);
}

void TrackedObject::removeDependency(TrackedObject* const obj){
    std::list<TrackedObject *>::iterator it;

    for(it = _dependencyList.begin(); it != _dependencyList.end(); ) {
        if(*it == obj){
            it = _dependencyList.erase(it);
        }else{
            ++it;
        }
    }
}

void TrackedObject::refModifyCallback(bool increase){
}