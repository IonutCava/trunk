#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

namespace Divide {

template<>
ParticleEmitter* ImplResourceLoader<ParticleEmitter>::operator()() {
    ParticleEmitter* ptr = MemoryManager_NEW ParticleEmitter();

    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }
    return ptr;
}

};
