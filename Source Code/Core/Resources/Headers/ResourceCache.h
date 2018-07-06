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
#include "Platform/Threading/Headers/ThreadPool.h"

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
    typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type loadResource(const ResourceDescriptor& descriptor) {
        // The loading process may change the resource descriptor so always use the user-specified descriptor hash for loockup!
        size_t loadingHash = descriptor.getHash();
        /// Check cache first to avoit loading the same resource twice
        std::shared_ptr<T> ptr = std::static_pointer_cast<T>(loadResource(loadingHash, descriptor.getName()));
        /// If the cache did not contain our resource ...
        if (!ptr) {
            /// ...aquire the resource's loader
            /// and get our resource as the loader creates it
            ptr = std::static_pointer_cast<T>(ImplResourceLoader<T>(*this, _context, descriptor, loadingHash)());
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

    CachedResource_ptr find(size_t descriptorHash);
    void add(CachedResource_wptr resource);
    /// Empty the entire cache of resources
    void clear();

protected:
    /// this method handles cache lookups and reference counting
    CachedResource_ptr loadResource(size_t descriptorHash, const stringImpl& resourceName);

protected:
    friend struct DeleteResource;
    /// unload a single resource and pend deletion
    void remove(CachedResource* resource);

protected:
    /// multithreaded resource creation
    SharedLock _creationMutex;
    typedef hashMapImpl<size_t, CachedResource_wptr> ResourceMap;
    ResourceMap _resDB;

private:
    PlatformContext& _context;

};

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache& cache, const ResourceDescriptor& descriptor) {
    return cache.loadResource<T>(descriptor);
}

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
FindResourceImpl(ResourceCache& cache, size_t hash) {
    return std::static_pointer_cast<T>(cache.find(hash));
}

};  // namespace Divide

#endif
