/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "ResourceLoader.h"
#include "Platform/Threading/Headers/ThreadPool.h"

#include "Utility/Headers/Localization.h"
#include "Core/Headers/PlatformContextComponent.h"

namespace Divide {

class ResourceLoadLock {
public:
    explicit ResourceLoadLock(size_t hash)
        : _loadingHash(hash)
    {
        WAIT_FOR_CONDITION(notLoading(_loadingHash));

        UniqueLockShared w_lock(_hashLock);
        _loadingHashes.push_back(_loadingHash);
    }

    ~ResourceLoadLock()
    {
        UniqueLockShared u_lock(_hashLock);
        _loadingHashes.erase(std::find(std::cbegin(_loadingHashes), std::cend(_loadingHashes), _loadingHash));
    }

private:
    static bool notLoading(size_t hash) {
        SharedLock r_lock(_hashLock);
        return std::find(std::cbegin(_loadingHashes), std::cend(_loadingHashes), hash) == std::cend(_loadingHashes);
    }

private:
    size_t _loadingHash;
    static SharedMutex _hashLock;
    static vector<size_t> _loadingHashes;
};
/// Resource Cache responsibilities:
/// - keep track of already loaded resources
/// - load new resources using appropriate resource loader and store them in
/// cache
/// - remove resources by name / pointer or remove all
class ResourceCache : public PlatformContextComponent {
public:
    ResourceCache(PlatformContext& context);
    ~ResourceCache();

 
    /// Each resource entity should have a 'resource name'Loader implementation.
    template <typename T>
    typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
    loadResource(const ResourceDescriptor& descriptor, bool& wasInCache)
    {
        // The loading process may change the resource descriptor so always use the user-specified descriptor hash for lookup!
        size_t loadingHash = descriptor.getHash();

        // If two thread are trying to load the same resource at the same time, by the time one of them adds the resource to the cache, it's too late
        // So check if the hash is currently in the "processing" list, and if it is, just busy-spin until done
        // Once done, lock the hash for ourselves
        ResourceLoadLock res_lock(loadingHash);

        /// Check cache first to avoid loading the same resource twice
        std::shared_ptr<T> ptr = std::static_pointer_cast<T>(loadResource(loadingHash, descriptor.resourceName()));
        /// If the cache did not contain our resource ...
        wasInCache = ptr != nullptr;
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

    void printContents() const;

protected:
    /// this method handles cache lookups and reference counting
    CachedResource_ptr loadResource(size_t descriptorHash, const stringImpl& resourceName);

protected:
    friend struct DeleteResource;
    /// unload a single resource and pend deletion
    void remove(CachedResource* resource);

protected:
    /// multithreaded resource creation
    typedef hashMap<size_t, CachedResource_wptr> ResourceMap;

    ResourceMap _resDB; 
    mutable SharedMutex _creationMutex;
};

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache& cache, const ResourceDescriptor& descriptor, bool& wasInCache) {
    return cache.loadResource<T>(descriptor, wasInCache);
}

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache& cache, const ResourceDescriptor& descriptor) {
    bool wasInCache = false;
    return CreateResource<T>(cache, descriptor, wasInCache);
}


};  // namespace Divide

#endif
