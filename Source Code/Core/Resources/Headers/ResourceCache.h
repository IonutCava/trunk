/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "ResourceLoader.h"
#include "ResourceDescriptor.h"
#include "Managers/Headers/BaseCache.h" 

///Resource Cache responsibilities:
/// - keep track of already loaded resources
/// - load new resources using apropriate resource loader and store them in cache
/// - remove resources by name / pointer or remove all
DEFINE_SINGLETON_EXT1( ResourceCache, BaseCache )

public:
	///Each resource entity should have a 'resource name'Loader implementation.
	template<class T>
	inline T* loadResource(const ResourceDescriptor& descriptor) {
		///Check cache first to avoit loading the same resource twice
		T* ptr = dynamic_cast<T*>(loadResource(descriptor.getName()));
		///If the cache did not contain our resource ...
		if(!ptr){
			/// ...aquire the resource's loader
			ImplResourceLoader<T> assetLoader(descriptor);
			/// and get our resource as the loader creates it
			ptr = assetLoader();
			/// validate it's integrity and add it to the cache
			add(descriptor.getName(),ptr);
		}
		return ptr;
	}

	void add(const std::string& name, Resource* const resource);

protected:
	~ResourceCache() {
		PRINT_FN("Destroying resource cache ...");
		PRINT_FN("Deleting resource cache ...");
	}
	///this method handles cache lookups and reference counting
	Resource* loadResource(const std::string& name);
	///multithreaded resource creation
	Lock _creationMutex;

END_SINGLETON

template<class T>
inline void RemoveResource(T*& resource, bool force = false){
	if(ResourceCache::getInstance().remove(resource,force)){
		ResourceCache::getInstance().eraseEntry(resource->getName());
		SAFE_DELETE(resource);
	}
}

template<class T>
inline T* CreateResource(const ResourceDescriptor& descriptor){
	return ResourceCache::getInstance().loadResource<T>(descriptor);
}

inline Resource* const FindResource(const std::string& name){
	return ResourceCache::getInstance().find(name);
}

#endif

