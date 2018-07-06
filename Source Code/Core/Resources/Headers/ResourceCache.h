/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "ResourceLoader.h"
#include "Platform/Threading/Headers/Thread.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

/// Resource Cache responsibilities:
/// - keep track of already loaded resources
/// - load new resources using apropriate resource loader and store them in
/// cache
/// - remove resources by name / pointer or remove all
class ResourceCache {
public:
    ResourceCache(PlatformContext& context);
    ~ResourceCache();

    /// Each resource entity should have a 'resource name'Loader implementation.
    template <typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
        loadResource(const ResourceDescriptor& descriptor) {
        /// Check cache first to avoit loading the same resource twice
        std::shared_ptr<T> ptr = std::static_pointer_cast<T>(loadResource(descriptor.getName()));
        /// If the cache did not contain our resource ...
        if (!ptr) {
            /// ...aquire the resource's loader
            /// and get our resource as the loader creates it
            ptr = std::static_pointer_cast<T>(ImplResourceLoader<T>(*this, _context, descriptor)());
            if (ptr) {
                /// validate it's integrity and add it to the cache
                add(ptr);
            }
        } else {
            if (descriptor.onLoadCallback()) {
                descriptor.onLoadCallback()(ptr);
            }
        }

        return ptr;
    }

    Resource_ptr find(const stringImpl& name);
    void add(Resource_wptr resource);
    /// Empty the entire cache of resources
    void clear();

protected:
    /// this method handles cache lookups and reference counting
    Resource_ptr loadResource(const stringImpl& name);

protected:
    friend struct DeleteResource;
    /// unload a single resource and pend deletion
    void remove(Resource* resource);

protected:
    /// multithreaded resource creation
    SharedLock _creationMutex;
    typedef hashMapImpl<U64, Resource_wptr> ResourceMap;
    ResourceMap _resDB;

private:
    PlatformContext& _context;

};

// This will copy the pointer but will still call unload on it if that is the case
template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, bool>::type
RemoveResource(ResourceCache& cache, std::shared_ptr<T>& resource) {
    if (resource && 
        cache.remove(*resource, resource.use_count()) ){
        resource.reset();
        return true;
    }

    return false;
}

template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache& cache, const ResourceDescriptor& descriptor) {
    return cache.loadResource<T>(descriptor);
}

template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
FindResourceImpl(ResourceCache& cache, const stringImpl& name) {
    return std::static_pointer_cast<T>(cache.find(name));
}

};  // namespace Divide

#endif
