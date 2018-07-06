#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMap.h"
#include "Headers/SingleShadowMap.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Headers/CascadedShadowMaps.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Buffers/FrameBuffer/Headers/FrameBuffer.h"

ShadowMap::ShadowMap(Light* light, ShadowType type) : _resolutionFactor(1),
                                                      _init(false),
                                                      _isBound(false),
                                                      _light(light),
                                                      _shadowMapType(type),
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
     _resolutionFactor = 1.0f;
     _numLayers = 1;
}

ShadowMapInfo::~ShadowMapInfo(){
    SAFE_DELETE(_shadowMap);
}

ShadowMap* ShadowMapInfo::getOrCreateShadowMap(const SceneRenderState& renderState){
    if(_shadowMap) return _shadowMap;

    if(!_light->castsShadows()) return nullptr;
    _resolutionFactor = renderState.shadowMapResolutionFactor();

    switch(_light->getLightType()){
        case LIGHT_TYPE_POINT:{
            _numLayers = 6;
            _shadowMap = New CubeShadowMap(_light);
            }break;
        case LIGHT_TYPE_DIRECTIONAL:{
            _numLayers = renderState.csmSplitCount();
            _shadowMap = New CascadedShadowMaps(_light, _numLayers, renderState.csmSplitLogFactor());
            }break;
        case LIGHT_TYPE_SPOT:{
            _shadowMap = New SingleShadowMap(_light);
            }break;
        default:
            break;
    };

    _shadowMap->init(this);

    return _shadowMap;
}

bool ShadowMap::Bind(U8 offset){
    if(_isBound)
        return false;

    _isBound = true;

    if(_depthMap){
        if(getShadowMapType() == SHADOW_TYPE_CSM){
            _depthMap->Bind(offset, TextureDescriptor::Color0);
            _depthMap->UpdateMipMaps(TextureDescriptor::Color0);
        }else{
            _depthMap->Bind(offset, TextureDescriptor::Depth);
        }
    }

    return true;
}

bool ShadowMap::Unbind(U8 offset){
    if(!_isBound)
        return false;

    if(_depthMap)
        _depthMap->Unbind(offset);

    _isBound = false;
    return true;
}

U16  ShadowMap::resolution(){
    return _depthMap->getWidth();
}

void ShadowMap::postRender(){
}