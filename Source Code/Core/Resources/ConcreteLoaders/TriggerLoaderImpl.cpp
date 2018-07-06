#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

Trigger* ImplResourceLoader<Trigger>::operator()(){
    Trigger* ptr = New Trigger();

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
        ptr->getSceneNodeRenderState().useDefaultMaterial(false);
        ptr->setMaterial(nullptr);
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Trigger)

};