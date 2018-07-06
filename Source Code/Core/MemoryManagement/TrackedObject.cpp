#include "Headers/TrackedObject.h"

TrackedObject::TrackedObject() : _refCount(1), _shouldDelete(false) {
}

TrackedObject::~TrackedObject(){
	///Never call release here!.
	///The smart pointer handling the object should call release upon delete until refCount reaches 0
	///Only when refCount is 0 should the smart pointer delete the object
	_dependencyList.clear();
}

void TrackedObject::AddRef() {
	++_refCount;

	cancelDeletion();

	std::list<TrackedObject *>::iterator it;
	for(it = _dependencyList.begin(); it != _dependencyList.end(); it++) {
		if(*it)
			(*it)->AddRef();
	}
	
}

void TrackedObject::Release() {

	--_refCount;

	if(_refCount <= 0){
		scheduleDeletion();
	}

	std::list<TrackedObject *>::iterator it;
	for(it = _dependencyList.begin(); it != _dependencyList.end(); ) {
		if(*it){
			(*it)->Release();
			if((*it)->getRefCount() <= 0){
				it = _dependencyList.erase(it);
				continue;
			}
		}
		it++;
	}
}

void TrackedObject::addDependency(TrackedObject* obj){
	///Some dependecies may be loaded later, so add null ones as well
	_dependencyList.push_back(obj);
}

void TrackedObject::removeDependency(TrackedObject* obj){
	std::list<TrackedObject *>::iterator it;
	for(it = _dependencyList.begin(); it != _dependencyList.end(); ) {
		if(*it == obj){ 
			it = _dependencyList.erase(it);
		}else{
			it++;
		}
	}
}