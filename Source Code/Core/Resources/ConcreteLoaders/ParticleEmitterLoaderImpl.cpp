#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

namespace Divide {

ParticleEmitter* ImplResourceLoader<ParticleEmitter>::operator()(){
	DIVIDE_ASSERT( _descriptor.hasPropertyDescriptor(), "ImplResourceLoader<ParticleEmitter> error: No property descriptor specified!" );
    ParticleEmitter* ptr = New ParticleEmitter();

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::SAFE_DELETE( ptr );
    } else {
        ptr->getSceneNodeRenderState().useDefaultMaterial( false );
        ptr->setMaterial( nullptr );
        if ( !ptr->initData() ) {
            MemoryManager::SAFE_DELETE( ptr );
        } else {
            ptr->setDescriptor( *_descriptor.getPropertyDescriptor<ParticleEmitterDescriptor>() );
        }
    }
    return ptr;
}

DEFAULT_LOADER_IMPL(ParticleEmitter)

};