/*
   Copyright (c) 2016 DIVIDE-Studio
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
DEFINE_SINGLETON(ResourceCache)
  public:
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
            ptr = std::static_pointer_cast<T>(ImplResourceLoader<T>(descriptor)());
            if (ptr) {
                /// validate it's integrity and add it to the cache
                add(ptr);
            }
        }

        return ptr;
    }

    Resource_ptr find(const stringImpl& name);
    void add(Resource_wptr resource);

  protected:
    ResourceCache();
    ~ResourceCache();
    /// Empty the entire cache of resources
    void destroy();
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

END_SINGLETON

// This will copy the pointer but will still call unload on it if that is the case
template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, bool>::type
RemoveResource(std::shared_ptr<T>& resource) {
    if (resource && 
        ResourceCache::instance().remove(*resource, resource.use_count()) ){
        resource.reset();
        return true;
    }

    return false;
}

template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
CreateResource(const ResourceDescriptor& descriptor) {
    return ResourceCache::instance().loadResource<T>(descriptor);
}

template <typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
FindResourceImpl(const stringImpl& name) {
    return std::static_pointer_cast<T>(ResourceCache::instance().find(name));
}

};  // namespace Divide

#endif
