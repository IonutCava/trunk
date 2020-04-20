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

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContextComponent.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

class ResourceLoadLock : private NonMovable {
public:
    explicit ResourceLoadLock(size_t hash, PlatformContext& context, const bool threaded);
    ~ResourceLoadLock();

    static void notifyTaskPool(PlatformContext& context);

private:
    const size_t _loadingHash;

    static Mutex s_hashLock;
    static eastl::unordered_set<size_t> s_loadingHashes;
};
/// Resource Cache responsibilities:
/// - keep track of already loaded resources
/// - load new resources using appropriate resource loader and store them in cache
/// - remove resources by name / pointer or remove all
class ResourceCache : public PlatformContextComponent {
public:
    ResourceCache(PlatformContext& context);
    ~ResourceCache();

    /// Each resource entity should have a 'resource name'Loader implementation.
    template <typename T, bool UseAtomicCounter>
    typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
    loadResource(const ResourceDescriptor& descriptor, bool& wasInCache, std::atomic_uint& taskCounter)
    {
        Time::ProfileTimer loadTimer = {};
        loadTimer.start();

        std::shared_ptr<T> ptr;
        {
            // The loading process may change the resource descriptor so always use the user-specified descriptor hash for lookup!
            const size_t loadingHash = descriptor.getHash();

            // If two thread are trying to load the same resource at the same time, by the time one of them adds the resource to the cache, it's too late
            // So check if the hash is currently in the "processing" list, and if it is, just busy-spin until done
            // Once done, lock the hash for ourselves
            ResourceLoadLock res_lock(loadingHash, _context, !Runtime::isMainThread());

            if_constexpr(UseAtomicCounter) {
                taskCounter.fetch_add(1u);
            }
            /// Check cache first to avoid loading the same resource twice
            ptr = std::static_pointer_cast<T>(find(loadingHash));
            /// If the cache did not contain our resource ...
            wasInCache = ptr != nullptr;
            if (!wasInCache) {
                Console::printfn(Locale::get(_ID("RESOURCE_CACHE_GET_RES")), descriptor.resourceName().c_str(), loadingHash);

                /// ...aquire the resource's loader and get our resource as the loader creates it
                ptr = std::static_pointer_cast<T>(ImplResourceLoader<T>(this, _context, descriptor, loadingHash)());
                assert(ptr != nullptr);
                add(ptr);
            }

            if_constexpr(UseAtomicCounter) {
                ptr->addStateCallback(ResourceState::RES_LOADED, [&taskCounter](auto) noexcept {
                    taskCounter.fetch_sub(1u);
                });
            }
        }
        if (descriptor.waitForReady()) {
            if (Runtime::isMainThread()) {
                WAIT_FOR_CONDITION(ptr->getState() == ResourceState::RES_LOADED);
            } else {
                WAIT_FOR_CONDITION_CALLBACK(ptr->getState() == ResourceState::RES_LOADED, ResourceLoadLock::notifyTaskPool, _context);
            }
        }

        loadTimer.stop();

        // Print load times
        const F32 timeInMS = Time::MicrosecondsToMilliseconds<F32>(loadTimer.get());
        if (timeInMS >= Time::SecondsToMilliseconds(1)) {
            if (wasInCache) {
                Console::printfn(Locale::get(_ID("RESOURCE_CACHE_RETRIEVE_TIME_S")), descriptor.resourceName().c_str(), Time::MillisecondsToSeconds(timeInMS));
            } else {
                Console::printfn(Locale::get(_ID("RESOURCE_CACHE_LOAD_TIME_S")), descriptor.resourceName().c_str(), Time::MillisecondsToSeconds(timeInMS));
            }
        } else {
            if (wasInCache) {
                Console::printfn(Locale::get(_ID("RESOURCE_CACHE_RETRIEVE_TIME_MS")), descriptor.resourceName().c_str(), timeInMS);
            } else {
                Console::printfn(Locale::get(_ID("RESOURCE_CACHE_LOAD_TIME_MS")), descriptor.resourceName().c_str(), timeInMS);
            }
        }

        return ptr;
    }

    CachedResource_ptr find(const size_t descriptorHash);
    void add(CachedResource_wptr resource);
    /// Empty the entire cache of resources
    void clear();

    void printContents() const;

protected:
    /// this method handles cache lookups and reference counting
    CachedResource_ptr loadResource(size_t descriptorHash, const Str128& resourceName);

protected:
    friend struct DeleteResource;
    /// unload a single resource and pend deletion
    void remove(CachedResource* resource);

protected:
    /// multithreaded resource creation
    using ResourceMap = ska::bytell_hash_map<size_t, CachedResource_wptr>;

    ResourceMap _resDB; 
    mutable SharedMutex _creationMutex;
};

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache* cache, const ResourceDescriptor& descriptor, bool& wasInCache) {
    std::atomic_uint taskCounter = 0u;
    return cache->loadResource<T, false>(descriptor, wasInCache, taskCounter);
}

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache* cache, const ResourceDescriptor& descriptor, bool& wasInCache, std::atomic_uint& taskCounter) {
    return cache->loadResource<T, true>(descriptor, wasInCache, taskCounter);
}

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache* cache, const ResourceDescriptor& descriptor) {
    bool wasInCache = false;
    return CreateResource<T>(cache, descriptor, wasInCache);
}

template <typename T>
typename std::enable_if<std::is_base_of<CachedResource, T>::value, std::shared_ptr<T>>::type
CreateResource(ResourceCache* cache, const ResourceDescriptor& descriptor, std::atomic_uint& taskCounter) {
    bool wasInCache = false;
    return CreateResource<T>(cache, descriptor, wasInCache, taskCounter);
}


};  // namespace Divide

#endif
