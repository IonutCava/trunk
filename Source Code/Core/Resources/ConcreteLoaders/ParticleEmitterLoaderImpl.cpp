#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

namespace Divide {

template<>
ParticleEmitter* ImplResourceLoader<ParticleEmitter>::operator()() {
    ParticleEmitter* ptr = MemoryManager_NEW ParticleEmitter(_descriptor.getName());

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }
    return ptr;
}

};
