#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMap.h"
#include "Headers/SingleShadowMap.h"
#include "Core/Headers/ParamHandler.h"
#include "Headers/ParallelSplitShadowMaps.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

ShadowMap::ShadowMap(Light* light) : _resolutionFactor(1),
                                     _init(false),
                                     _isBound(false),
                                     _light(light),
                                     _par(ParamHandler::getInstance())
{
}

ShadowMap::~ShadowMap()
{
    SAFE_DELETE(_depthMap);
}

ShadowMapInfo::ShadowMapInfo(Light* light) : _light(light),
                                             _shadowMap(NULL)
{
     _resolution = 1024;
     _numSplits = 3;
}

ShadowMapInfo::~ShadowMapInfo(){
    SAFE_DELETE(_shadowMap);
}

ShadowMap* ShadowMapInfo::getOrCreateShadowMap(const SceneRenderState& renderState){
    if(_shadowMap) return _shadowMap;
    if(!_light->castsShadows()) return NULL;
    switch(_light->getLightType()){
    case LIGHT_TYPE_POINT:
        _shadowMap = New CubeShadowMap(_light);
        break;
    case LIGHT_TYPE_DIRECTIONAL:
        _shadowMap = New PSShadowMaps(_light);
        break;
    case LIGHT_TYPE_SPOT:
        _shadowMap = New SingleShadowMap(_light);
        break;
    default:
        break;
    };
    _shadowMap->resolution(_resolution, renderState);
    return _shadowMap;
}

bool ShadowMap::Bind(U8 offset){
    if(_isBound)
        return false;

    _isBound = true;

    if(_depthMap)
        _depthMap->Bind(offset);

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