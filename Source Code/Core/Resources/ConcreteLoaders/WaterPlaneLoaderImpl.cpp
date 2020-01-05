#include "stdafx.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/StringHelper.h"
#include "Environment/Water/Headers/Water.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<WaterPlane>::operator()() {

    eastl::shared_ptr<WaterPlane> ptr(MemoryManager_NEW WaterPlane(_cache,
                                                                   _loadingDescriptorHash,
                                                                   _descriptor.resourceName()),
                                    DeleteResource(_cache));

    ptr->setState(ResourceState::RES_LOADING);
    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
