#include "Headers/LightManager.h"
#include "Headers/SceneManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Hardware/Video/Buffers/FrameBuffer/Headers/FrameBuffer.h"
#include "Hardware/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

ProfileTimer* s_shadowPassTimer = nullptr;

namespace {
    struct PerNodeLightData { 
        U32  _lightIndex;
        U32  _lightType;
        U32  _lightCastsShadows;
        U32  _lightCount; // padding basically

        PerNodeLightData() : _lightCount(0),
                             _lightIndex(0),
                             _lightType(0),
                             _lightCastsShadows(0)
        {
        }

        void fromLight(U32 count, Light* crtLight){
            _lightCount = count;
            _lightCastsShadows = crtLight->castsShadows() ? 1 : 0;
            _lightType  = crtLight->getLightType();
            _lightIndex = crtLight->getSlot();
        }
    };
    vectorImpl<PerNodeLightData> perNodeLights;
}

LightManager::LightManager() : FrameListener(),
                               _shadowMapsEnabled(true),
                               _previewShadowMaps(false),
                               _currentShadowPass(0)
{
    s_shadowPassTimer = ADD_TIMER("ShadowPassTimer");
    
    _lightShaderBuffer[SHADER_BUFFER_VISUAL]    = GFX_DEVICE.newSB();
    _lightShaderBuffer[SHADER_BUFFER_PHYSICAL]  = GFX_DEVICE.newSB();
    _lightShaderBuffer[SHADER_BUFFER_SHADOW]    = GFX_DEVICE.newSB();
    _lightShaderBuffer[SHADER_BUFFER_PER_NODE]  = GFX_DEVICE.newSB();
    
    memset(normShadowLocation,  -1, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));
    memset(cubeShadowLocation,  -1, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));
    memset(arrayShadowLocation, -1, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));

    _opaqueGrid = New LightGrid();
    _transparentGrid = New LightGrid();
}

LightManager::~LightManager()
{
    clear();
    REMOVE_TIMER(s_shadowPassTimer);
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_VISUAL] );
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_PHYSICAL]);
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_SHADOW]);
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_PER_NODE]);
    SAFE_DELETE(_opaqueGrid);
    SAFE_DELETE(_transparentGrid);
}

void LightManager::init(){
    REGISTER_FRAME_LISTENER(&(this->getInstance()), 2);
    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&LightManager::previewShadowMaps, this, nullptr), 1);
    _lightShaderBuffer[SHADER_BUFFER_VISUAL]->Create(true, false, Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightVisualProperties));
    _lightShaderBuffer[SHADER_BUFFER_VISUAL]->Bind(Divide::SHADER_BUFFER_LIGHT_COLOR);

    _lightShaderBuffer[SHADER_BUFFER_PHYSICAL]->Create(true, false, Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightPhysicalProperties));
    _lightShaderBuffer[SHADER_BUFFER_PHYSICAL]->Bind(Divide::SHADER_BUFFER_LIGHT_NORMAL);

    _lightShaderBuffer[SHADER_BUFFER_SHADOW]->Create(true, false, Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightShadowProperties));
    _lightShaderBuffer[SHADER_BUFFER_SHADOW]->Bind(Divide::SHADER_BUFFER_LIGHT_SHADOW);

    _lightShaderBuffer[SHADER_BUFFER_PER_NODE]->Create(true, true, Config::Lighting::MAX_LIGHTS_PER_SCENE_NODE, sizeof(PerNodeLightData));
    _lightShaderBuffer[SHADER_BUFFER_PER_NODE]->Bind(Divide::SHADER_BUFFER_LIGHT_PER_NODE);

    perNodeLights.resize(Config::Lighting::MAX_LIGHTS_PER_SCENE_NODE);

    _cachedResolution.set(GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution());

    I32 maxTextureStorage = GFX_DEVICE.getMaxTextureUnits();
    I32 maxSlotsPerLight = 3;
    maxTextureStorage -= Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * maxSlotsPerLight;

    for (U8 i = 0; i < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; ++i){
        normShadowLocation[i]  = maxTextureStorage + 0 + (i * maxSlotsPerLight);
        cubeShadowLocation[i]  = maxTextureStorage + 1 + (i * maxSlotsPerLight);
        arrayShadowLocation[i] = maxTextureStorage + 2 + (i * maxSlotsPerLight);
    }
}

bool LightManager::clear(){
    //Lights are removed by the sceneGraph
    FOR_EACH(Light::LightMap::value_type it, _lights){
        //in case we had some light hanging
        RemoveResource(it.second);
    }

    _lights.clear();

    return _lights.empty();
}

bool LightManager::addLight(Light* const light){
    light->addShadowMapInfo(New ShadowMapInfo(light));

    if(_lights.find(light->getGUID()) != _lights.end()){
        ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"), light->getGUID());
        return false;
    }

    light->setSlot((U8)_lights.size());
    _lights.insert(std::make_pair(light->getGUID(), light));
    GET_ACTIVE_SCENE()->renderState().getCameraMgr().addNewCamera(light->getName(), light->shadowCamera());
    return true;
}

// try to remove any leftover lights
bool LightManager::removeLight(U32 lightId){
    /// we can't remove a light if the light list is empty. That light has to exist somewhere!
    assert(!_lights.empty());

    Light::LightMap::const_iterator it = _lights.find(lightId);

    if(it == _lights.end()){
        ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_REMOVE_LIGHT"),lightId);
        return false;
    }

    _lights.erase(it); //remove it from the map
    return true;
}

void LightManager::idle(){
    _shadowMapsEnabled = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");

    s_shadowPassTimer->pause(!_shadowMapsEnabled);
}

void LightManager::updateResolution(I32 newWidth, I32 newHeight){
    for(Light* light : _currLightsPerNode)
        light->updateResolution(newWidth, newHeight);

    _cachedResolution.set(newWidth, newHeight);
}

///Check light properties for every light (this is bound to the camera change listener group
///Update only if needed. Get projection and view matrices if they changed
///Also, search for the dominant light if any
void LightManager::onCameraChange(){
    GFX_DEVICE.getMatrix(VIEW_MATRIX, _viewMatrixCache);

    for(Light* light : _currLightsPerNode){
        assert(light != nullptr);
        light->onCameraChange();
    }
}

bool LightManager::buildLightGrid(SceneRenderState& sceneRenderState){
    vectorImpl<LightGrid::LightInternal > omniLights;
    FOR_EACH(Light::LightMap::value_type& it, _lights){
        const Light& light = *it.second;
        if (light.getLightType() == LIGHT_TYPE_POINT){
            omniLights.push_back(LightGrid::make_light(light.getPosition(), light.getDiffuseColor(), light.getRange()));
        }
    }
    if (!omniLights.empty()){
        Camera& cam = sceneRenderState.getCamera();
        _transparentGrid->build(vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
                                _cachedResolution,
                                omniLights,
                                cam.getViewMatrix(),
                                cam.getProjectionMatrix(),
                                cam.getZPlanes().x, 
                                vectorImpl<vec2<F32> >());

        {
            vectorImpl<vec2<F32> > depthRanges;
            GFX_DEVICE.DownSampleDepthBuffer(depthRanges);
            // We take a copy of this, and prune the grid using depth ranges found from pre-z pass (for opaque geometry).
            // Note that the pruning does not occur if the pre-z pass was not performed (depthRanges is empty in this case).
            _opaqueGrid = _transparentGrid;
            _opaqueGrid->prune(depthRanges);
            _transparentGrid->pruneFarOnly(cam.getZPlanes().x, depthRanges);
        }
    }

    return true;
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire application!
bool LightManager::framePreRenderEnded(const FrameEvent& evt){
    if(!_shadowMapsEnabled)
        return true;

    START_TIMER(s_shadowPassTimer);

    //Tell the engine that we are drawing to depth maps
    //set the current render stage to SHADOW_STAGE
    RenderStage previousRS = GFX_DEVICE.setRenderStage(SHADOW_STAGE);
    //generate shadowmaps for each light
    FOR_EACH(Light::LightMap::value_type& light, _lights){
        setCurrentLight(light.second);
        light.second->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
    }

    //Revert back to the previous stage
    GFX_DEVICE.setRenderStage(previousRS);

    STOP_TIMER(s_shadowPassTimer);

    return true;
}

void LightManager::togglePreviewShadowMaps() { 
    _previewShadowMaps = !_previewShadowMaps; 
    //Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))
        return;

    FOR_EACH(Light::LightMap::value_type& it, _lights){
        assert(it.second->getShadowMapInfo() && it.second->getShadowMapInfo()->getShadowMap());
        it.second->getShadowMapInfo()->getShadowMap()->togglePreviewShadowMaps(_previewShadowMaps);
    }
}

void LightManager::previewShadowMaps(Light* light) {
    //Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !_previewShadowMaps || !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))
        return;

    if (!light) light = _lights.begin()->second;

    if(!light->castsShadows())
        return;

    light->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
}

//If we have computed shadowmaps, bind them before rendering any geometry;
//Always bind shadowmaps to slots Config::MAX_TEXTURE_STORAGE, Config::MAX_TEXTURE_STORAGE+1, Config::MAX_TEXTURE_STORAGE+2 ...
void LightManager::bindDepthMaps(U32 lightIndex, bool overrideDominant){
    //Skip applying shadows if we are rendering to depth map, or we have shadows disabled
    if(!_shadowMapsEnabled)
        return;

    Light* lightLocal = getLightForCurrentNode(lightIndex);
    assert(lightLocal);

    if(!lightLocal->castsShadows())
        return;

    ShadowMap* sm = lightLocal->getShadowMapInfo()->getShadowMap();
    if(sm)
        sm->Bind(getShadowBindSlot(lightLocal->getLightType(), lightIndex));
}

bool LightManager::shadowMappingEnabled() const {
    if(!_shadowMapsEnabled)
        return false;

    FOR_EACH(Light::LightMap::value_type light, _lights){
        if(!light.second->castsShadows())
            continue;

        ShadowMapInfo* smi = light.second->getShadowMapInfo();
        //no shadow info;
        if(!smi)
            continue;

        ShadowMap* sm = smi->getShadowMap();
        //no shadow map;
        if(!sm)
            continue;

        if(sm->getDepthMap())
            return true;
    }

    return false;
}

U8 LightManager::findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter){
    const vec3<F32> lumDot( 0.2125f, 0.7154f, 0.0721f );
    F32 luminace = 0.0f;
    F32 dist = 0.0f;
    F32 weight = 1.0f; // later
    U16 i = 0;

    // Reset light buffer
    _tempLightsPerNode.resize(_lights.size());
    _currLightsPerNode.resize(0);
    // loop over every light in the scene
    // ToDo: add a grid based light search system? -Ionut
    FOR_EACH(Light::LightMap::value_type& lightIt, _lights){
        Light* light = lightIt.second;
        if(!light->getEnabled())
            continue;

        LightType lType = light->getLightType();
        if(lType != LIGHT_TYPE_DIRECTIONAL )  {
            // get the luminosity.
            luminace  = light->getDiffuseColor().dot(lumDot);
            luminace *= light->getRange();

            F32 radiusSq = squared(light->getRange() + node->getBoundingSphereConst().getRadius());
            // get the distance to the light... score it 1 to 0 near to far.
            vec3<F32> distToLight(node->getBoundingBoxConst().getCenter() - light->getPosition());
            F32 distSq = radiusSq - distToLight.lengthSquared();

            if ( distSq > 0.0f ) {
                dist = square_root_tpl(distSq) / 1000.0f;
                CLAMP<F32>(dist, 0.0f, 1.0f );
            }
            light->setScore( luminace * weight * dist );
        }else{// directional
            light->setScore( weight );
        }
        // use type filter
        if ((typeFilter != LightType_PLACEHOLDER && lType == typeFilter) || //< whether we have the correct light type
            typeFilter == LightType_PLACEHOLDER){ //< or we did not specify a type filter
            _tempLightsPerNode[i++] = light;
        }
    }

    // sort the lights by score
    std::sort(_tempLightsPerNode.begin(), _tempLightsPerNode.end(), [](Light* const a, Light* const b) -> bool { return (a)->getScore() >(b)->getScore(); });

    // create the light buffer for the specified node
    size_t maxLights = _tempLightsPerNode.size();
    CLAMP<size_t>(maxLights, 0, Config::Lighting::MAX_LIGHTS_PER_SCENE_NODE);
    for(i = 0; i < maxLights; i++){
        PerNodeLightData& data = perNodeLights[i];
        Light* crtLight = _tempLightsPerNode[i];
        data.fromLight((U32)maxLights, crtLight);

        updatePhysicalLightProperties(crtLight);
        updateVisualLightProperties(crtLight);

        _currLightsPerNode.push_back(crtLight);
        if (crtLight->castsShadows()){
            updateShadowLightProperties(crtLight);
        }
    }

    _lightShaderBuffer[SHADER_BUFFER_PER_NODE]->UpdateData(0, perNodeLights.size() * sizeof(PerNodeLightData), &perNodeLights.front(), true);

    return (U8)maxLights;
}

Light* LightManager::getLight(U32 slot) {
    FOR_EACH(Light::LightMap::value_type it, _lights){
        if (it.second->getSlot() == slot)
            return it.second;
    }
    
    assert(false);
    return nullptr;
}

void LightManager::updatePhysicalLightProperties(Light* const light){
    assert(light != nullptr);
    if (!light->_dirty[Light::PROPERTY_TYPE_PHYSICAL])
        return;

    LightPhysicalProperties temp = light->getPhysicalProperties();

    if (light->getLightType() == LIGHT_TYPE_DIRECTIONAL){
        temp._position.set(vec3<F32>(_viewMatrixCache * temp._position), temp._position.w);
    }
    else if (light->getLightType() == LIGHT_TYPE_SPOT){
        temp._direction.set(vec3<F32>(_viewMatrixCache * temp._direction), temp._direction.w);
    }
    _lightShaderBuffer[SHADER_BUFFER_PHYSICAL]->UpdateData(light->getSlot() * sizeof(LightPhysicalProperties), sizeof(LightPhysicalProperties), (GLvoid*)&temp);
    light->_dirty[Light::PROPERTY_TYPE_PHYSICAL] = false;
}

void LightManager::updateVisualLightProperties(Light* const light){
    assert(light != nullptr);
    if (!light->_dirty[Light::PROPERTY_TYPE_VISUAL])
        return;
    _lightShaderBuffer[SHADER_BUFFER_VISUAL]->UpdateData(light->getSlot() * sizeof(LightVisualProperties), sizeof(LightVisualProperties), (void*)&light->getVisualProperties());
    light->_dirty[Light::PROPERTY_TYPE_VISUAL] = false;

}

void LightManager::updateShadowLightProperties(Light* const light){
    assert(light != nullptr);
    if (!light->_dirty[Light::PROPERTY_TYPE_SHADOW])
        return;
    _lightShaderBuffer[SHADER_BUFFER_SHADOW]->UpdateData(light->getSlot() * sizeof(LightShadowProperties), sizeof(LightShadowProperties), (void*)&light->getShadowProperties());
    light->_dirty[Light::PROPERTY_TYPE_SHADOW] = false;
}