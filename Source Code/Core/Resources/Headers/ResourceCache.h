/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "ResourceLoader.h"
#include "Hardware/Platform/Headers/Thread.h"
#include "Hardware/Platform/Headers/SharedMutex.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

///Resource Cache responsibilities:
/// - keep track of already loaded resources
/// - load new resources using apropriate resource loader and store them in cache
/// - remove resources by name / pointer or remove all
DEFINE_SINGLETON( ResourceCache )

public:
    ///Each resource entity should have a 'resource name'Loader implementation.
    template<typename T>
    inline T* loadResource(const ResourceDescriptor& descriptor) {
        ///Check cache first to avoit loading the same resource twice
        T* ptr = dynamic_cast<T*>(loadResource(descriptor.getName()));
        ///If the cache did not contain our resource ...
        if(!ptr){
            /// ...aquire the resource's loader
            ImplResourceLoader<T> assetLoader(descriptor);
            /// and get our resource as the loader creates it
            ptr = assetLoader();
            if(ptr){
                ptr->setState(RES_LOADED);
                /// validate it's integrity and add it to the cache
                add(descriptor.getName(),ptr);
            }
        }
        return ptr;
    }

    Resource* const find(const stringImpl& name);
    void add(const stringImpl& name, Resource* const resource);
    bool remove(Resource* res);
    bool load(Resource* const res, const stringImpl& name);
    bool loadHW(Resource* const res, const stringImpl& name);

protected:
    ResourceCache();
    ~ResourceCache();
    ///Empty the entire cache of resources
    void Destroy();
    ///this method handles cache lookups and reference counting
    Resource* loadResource(const stringImpl& name);
    ///unload a single resource and pend deletion
    bool removeInternal(Resource* const resource);
    ///multithreaded resource creation
    SharedLock _creationMutex;

    typedef hashMapImpl<stringImpl, Resource*> ResourceMap;
    ResourceMap _resDB;
    //boost::threadpool::pool* _loadingPool;

END_SINGLETON

template<typename T>
inline bool RemoveResource(T*& resource){
    DIVIDE_ASSERT(ResourceCache::hasInstance(), 
                  Locale::get( "RESOURCE_CACHE_NOT_AVAILABLE" ));
    DIVIDE_ASSERT(dynamic_cast<Resource*>( resource ) != nullptr,
                  Locale::get( "RESOURCE_CACHE_REMOVE_NOT_RESOURCE" ));

    if (ResourceCache::getInstance().remove(resource)) {
       resource = nullptr;
       return true;
	}

	return false;
}

template<typename T>
inline T* CreateResource(const ResourceDescriptor& descriptor){
    return ResourceCache::getInstance().loadResource<T>(descriptor);
}

template<typename T>
inline T* const FindResourceImpl(const stringImpl& name){
    return static_cast<T*>(ResourceCache::getInstance().find(name));
}

}; //namespace Divide

#endif
