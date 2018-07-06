#include "Headers/TrackedObject.h"
#include <assert.h>

TrackedObject::TrackedObject() : _refCount(1),
								 _shouldDelete(false)
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
	for(std::list<TrackedObject *>::iterator it = _dependencyList.begin();
		                                     it != _dependencyList.end();
										   ++it) {
		/// We should never have a null object in our dependency map
		assert(*it != NULL);
		/// Increase it's ref count
		(*it)->AddRef();
	}
	/// increase our ref count
	++_refCount;
	/// if AddRef is called, then deletion should be canceled no matter what
	cancelDeletion();
}

bool TrackedObject::SubRef() {
	/// For each object in our dependency map
	for(std::list<TrackedObject *>::iterator it = _dependencyList.begin();
		                                     it != _dependencyList.end(); ) {
		/// We should never have a null object in our dependency map
	    assert(*it != NULL);
		/// substract ref count for every sub object
		if((*it)->SubRef()){
			/// if it's no longer used, remove it from the dependecy list
			/// it should be scheduled for deletion anyway
			it = _dependencyList.erase(it);
			continue;
		}
		++it;
	}
	//r_lock.unlock();
	///Substract ref count and check if object is still used
	if(--_refCount <= 0){
		/// if not, schedule it's deletion
		scheduleDeletion();
		/// and return true
		return true;
	}
	/// object is not ready for deletion yet
	return false;
}

void TrackedObject::addDependency(TrackedObject* const obj){
	///Some dependecies may be loaded later, so add null ones as well
	_dependencyList.push_back(obj);
	obj->addParent(this);
}

void TrackedObject::removeDependency(TrackedObject* const obj){
	std::list<TrackedObject *>::iterator it;

	for(it = _dependencyList.begin(); it != _dependencyList.end(); ) {
		if(*it == obj){
			it = _dependencyList.erase(it);
			obj->removeParent(this);
		}else{
			++it;
		}
	}
}

void TrackedObject::addParent(TrackedObject* const obj){
	_parentList.push_back(obj);
}

void TrackedObject::removeParent(TrackedObject* const obj){
	std::list<TrackedObject *>::iterator it;

	for(it = _parentList.begin(); it != _parentList.end(); ) {
		if(*it == obj){
			it = _parentList.erase(it);
		}else{
			++it;
		}
	}
}