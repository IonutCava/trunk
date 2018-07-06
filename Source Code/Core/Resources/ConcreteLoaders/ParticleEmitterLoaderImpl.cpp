#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<ParticleEmitter>::operator()() {
    std::shared_ptr<ParticleEmitter> ptr(MemoryManager_NEW ParticleEmitter(_context.gfx(), _cache, _descriptor.getName()),
                                         DeleteResource(_cache));

    if (!load(ptr)) {
        ptr.reset();
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }
    return ptr;
}

};
