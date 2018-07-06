#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

ParticleEmitter* ImplResourceLoader<ParticleEmitter>::operator()(){
    ParticleEmitter* ptr = New ParticleEmitter();

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
        ptr->getSceneNodeRenderState().useDefaultMaterial(false);
        ptr->setMaterial(nullptr);
        if(!ptr->initData()){
            SAFE_DELETE(ptr);
        }
    }
    return ptr;
}

DEFAULT_LOADER_IMPL(ParticleEmitter)