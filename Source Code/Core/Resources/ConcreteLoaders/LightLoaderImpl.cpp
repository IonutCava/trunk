#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/PointLight.h"
#include "Rendering/Lighting/Headers/SpotLight.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"

Light* ImplResourceLoader<Light>::operator()(){
    Light* ptr = NULL;
    //descriptor ID is not the same as light ID. This is the light's slot!!
    switch(_descriptor.getEnumValue()){
        case -1:
        case LIGHT_TYPE_POINT:
        default:
            ptr = New PointLight(_descriptor.getId());
        break;
        case LIGHT_TYPE_DIRECTIONAL:
            ptr = New DirectionalLight(_descriptor.getId());
            break;
        case LIGHT_TYPE_SPOT:
            ptr = New SpotLight(_descriptor.getId());
            break;
    };
    assert(ptr != NULL);

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
        ptr->getSceneNodeRenderState().useDefaultMaterial(false);
        ptr->setMaterial(NULL);
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Light)