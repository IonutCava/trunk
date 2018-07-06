#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMap.h"
#include "Headers/SingleShadowMap.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Headers/CascadedShadowMaps.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {

ShadowMap::ShadowMap(Light* light, Camera* shadowCamera, ShadowType type) : _init(false),
                                                                            _light(light),
                                                                            _shadowCamera(shadowCamera),
                                                                            _shadowMapType(type),
                                                                            _resolution(0),
                                                                            _par(ParamHandler::getInstance())
{
    _bias.bias();
}

ShadowMap::~ShadowMap()
{
    SAFE_DELETE(_depthMap);
}

ShadowMapInfo::ShadowMapInfo(Light* light) : _light(light),
                                             _shadowMap(nullptr)
{
     _resolution = 512;
     if (GFX_DEVICE.shadowDetailLevel() == DETAIL_HIGH)  _resolution = 1024;
     if (GFX_DEVICE.shadowDetailLevel() == DETAIL_ULTRA) _resolution = 2048;
     _numLayers = 1;
}

ShadowMapInfo::~ShadowMapInfo(){
    SAFE_DELETE(_shadowMap);
}

ShadowMap* ShadowMapInfo::getOrCreateShadowMap(const SceneRenderState& renderState, Camera* shadowCamera){
    if(_shadowMap) return _shadowMap;

    if(!_light->castsShadows()) return nullptr;

    switch(_light->getLightType()){
        case LIGHT_TYPE_POINT:{
            _numLayers = 6;
            _shadowMap = New CubeShadowMap(_light, shadowCamera);
            }break;
        case LIGHT_TYPE_DIRECTIONAL:{
            DirectionalLight* dirLight = dynamic_cast<DirectionalLight*>(_light);
            _numLayers = dirLight->csmSplitCount();
            _shadowMap = New CascadedShadowMaps(_light, shadowCamera, _numLayers);
            }break;
        case LIGHT_TYPE_SPOT:{
            _shadowMap = New SingleShadowMap(_light, shadowCamera);
            }break;
        default:
            break;
    };

    _shadowMap->init(this);

    
    return _shadowMap;
}

void ShadowMapInfo::resolution(U16 resolution) {
    _resolution = resolution;
    if (_shadowMap)
        _shadowMap->resolution(_resolution, _light->shadowMapResolutionFactor());
}

bool ShadowMap::Bind(U8 offset){
    if (!_depthMap)
        return false;

    return BindInternal(offset);
}

bool ShadowMap::BindInternal(U8 offset){
    _depthMap->Bind(offset, TextureDescriptor::Depth);
    return true;
}

U16  ShadowMap::resolution(){
    return _depthMap->getWidth();
}

void ShadowMap::postRender(){
}

};