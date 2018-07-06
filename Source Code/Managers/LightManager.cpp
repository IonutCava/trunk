#include "Headers/LightManager.h"
#include "Headers/SceneManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Hardware/Video/Buffers/Framebuffer/Headers/Framebuffer.h"
#include "Hardware/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

ProfileTimer* s_shadowPassTimer = nullptr;

LightManager::LightManager() : FrameListener(),
                               _init(false),
                               _shadowMapsEnabled(true),
                               _previewShadowMaps(false),
                               _currentShadowPass(0)
{
    s_shadowPassTimer = ADD_TIMER("ShadowPassTimer");
    
    _lightShaderBuffer[SHADER_BUFFER_NORMAL]   = GFX_DEVICE.newSB();
    _lightShaderBuffer[SHADER_BUFFER_SHADOW]   = GFX_DEVICE.newSB();
    
    for(U8 i = 0; i < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; ++i){
        normShadowLocation[i] = 255;
        cubeShadowLocation[i] = 255;
        arrayShadowLocation[i] = 255;
    }

    _opaqueGrid = New LightGrid();
    _transparentGrid = New LightGrid();
}

LightManager::~LightManager()
{
    clear();
    REMOVE_TIMER(s_shadowPassTimer);
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_NORMAL] );
    SAFE_DELETE(_lightShaderBuffer[SHADER_BUFFER_SHADOW]);
    SAFE_DELETE(_opaqueGrid);
    SAFE_DELETE(_transparentGrid);
}

void LightManager::init(){
    STUBBED("Replace light map bind slots with bindless textures! Max texture units is currently hard coded! -Ionut!");

    REGISTER_FRAME_LISTENER(&(this->getInstance()), 2);

    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&LightManager::previewShadowMaps, this, nullptr), 1);
    _lightShaderBuffer[SHADER_BUFFER_NORMAL]->Create(Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightProperties));
    _lightShaderBuffer[SHADER_BUFFER_NORMAL]->Bind(Divide::SHADER_BUFFER_LIGHT_NORMAL);

    _lightShaderBuffer[SHADER_BUFFER_SHADOW]->Create(Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightShadowProperties));
    _lightShaderBuffer[SHADER_BUFFER_SHADOW]->Bind(Divide::SHADER_BUFFER_LIGHT_SHADOW);

    _cachedResolution.set(GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution());

    U32 maxTextureStorage = GFX_DEVICE.getMaxTextureSlots();
    U32 maxSlotsPerLight = 3;
    maxTextureStorage -= Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * maxSlotsPerLight;

    for (U8 i = 0; i < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; ++i){
        normShadowLocation[i]  = maxTextureStorage + 0 + (i * maxSlotsPerLight);
        cubeShadowLocation[i]  = maxTextureStorage + 1 + (i * maxSlotsPerLight);
        arrayShadowLocation[i] = maxTextureStorage + 2 + (i * maxSlotsPerLight);
    }
    _init = true;
}

bool LightManager::clear(){
    if(!_init)
        return true;

    UNREGISTER_FRAME_LISTENER(&(this->getInstance()));

    //Lights are removed by the sceneGraph
    FOR_EACH(Light::LightMap::value_type it, _lights){
        //in case we had some light hanging
        RemoveResource(it.second);
    }

    _lights.clear();

    _init = false;

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
    FOR_EACH(Light::LightMap::value_type& it, _lights)
        it.second->updateResolution(newWidth, newHeight);

    _cachedResolution.set(newWidth, newHeight);
}

///Check light properties for every light (this is bound to the camera change listener group
///Update only if needed. Get projection and view matrices if they changed
///Also, search for the dominant light if any
void LightManager::onCameraChange(){
     FOR_EACH(Light::LightMap::value_type& it, _lights){
        assert(it.second != nullptr);
        it.second->onCameraChange();
    }
}

bool LightManager::buildLightGrid(const mat4<F32>& viewMatrix, const mat4<F32>& projectionMatrix, const vec2<F32>& zPlanes){
    vectorImpl<LightGrid::LightInternal > omniLights;
    omniLights.reserve(_lights.size());
    FOR_EACH(Light::LightMap::value_type& it, _lights){
        const Light& light = *it.second;
        if (light.getLightType() == LIGHT_TYPE_POINT){
            omniLights.push_back(LightGrid::make_light(light.getPosition(), light.getDiffuseColor(), light.getRange()));
        }
    }
    if (!omniLights.empty()){
        _transparentGrid->build(vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
                                _cachedResolution,
                                omniLights,
                                viewMatrix,
                                projectionMatrix,
                                zPlanes.x, 
                                vectorImpl<vec2<F32> >());

        {
            vectorImpl<vec2<F32> > depthRanges;
            GFX_DEVICE.DownSampleDepthBuffer(depthRanges);
            // We take a copy of this, and prune the grid using depth ranges found from pre-z pass (for opaque geometry).
            // Note that the pruning does not occur if the pre-z pass was not performed (depthRanges is empty in this case).
            _opaqueGrid = _transparentGrid;
            _opaqueGrid->prune(depthRanges);
            _transparentGrid->pruneFarOnly(zPlanes.x, depthRanges);
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
    /*FOR_EACH(Light::LightMap::value_type& light, _lights){
        setCurrentLight(light.second);
        light.second->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
    }*/

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
        if(it.second->getShadowMapInfo()->getShadowMap()){
            it.second->getShadowMapInfo()->getShadowMap()->togglePreviewShadowMaps(_previewShadowMaps);
        }
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
void LightManager::bindDepthMaps(){
    //Skip applying shadows if we are rendering to depth map, or we have shadows disabled
    if(!_shadowMapsEnabled)
        return;

    for(U8 i = 0; i < std::min((U32)_lights.size(), Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE); ++i){
        Light* lightLocal = getLight(i);
        assert(lightLocal);

        if(!lightLocal->castsShadows())
            continue;

        ShadowMap* sm = lightLocal->getShadowMapInfo()->getShadowMap();
        if(sm){
#           if defined(_DEBUG)
                U8 slot = getShadowBindSlot(lightLocal->getLightType(), i);
                assert(slot < GFX_DEVICE.getMaxTextureSlots());
                sm->Bind(slot);
#           else
                sm->Bind(getShadowBindSlot(lightLocal->getLightType(), i));
#           endif
        }
    }
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

Light* LightManager::getLight(U32 slot) {
    Light::LightMap::const_iterator it = std::find_if(_lights.begin(), _lights.end(), [&slot](const Light::LightMap::value_type& vt) 
                                                                                       { return vt.second->getSlot() == slot; });
    assert(it != _lights.end());
    return it->second;
}

void LightManager::updateAndUploadLightData(const mat4<F32>& viewMatrix){
    _lightProperties.resize(0);
    _lightProperties.reserve(_lights.size());

    _lightShadowProperties.resize(0);
    _lightShadowProperties.reserve(_lights.size());

    FOR_EACH(Light::LightMap::value_type& lightIt, _lights){
        Light* light = lightIt.second;
        if (light->_dirty[Light::PROPERTY_TYPE_PHYSICAL]){
            LightProperties temp = light->getProperties();
            if (light->getLightType() == LIGHT_TYPE_DIRECTIONAL){
                temp._position.set(vec3<F32>(viewMatrix * temp._position), temp._position.w);
            }else if (light->getLightType() == LIGHT_TYPE_SPOT){
                temp._direction.set(vec3<F32>(viewMatrix * temp._direction), temp._direction.w);
            }
            _lightProperties.push_back(temp);
        }else{
            _lightProperties.push_back(light->getProperties());
        }
        if(light->castsShadows()){
            _lightShadowProperties.push_back(light->getShadowProperties());
            light->_dirty[Light::PROPERTY_TYPE_SHADOW] = false;
        }

        light->_dirty[Light::PROPERTY_TYPE_VISUAL] = false;
        light->_dirty[Light::PROPERTY_TYPE_PHYSICAL] = false;        
    }

    if(!_lightProperties.empty())
        _lightShaderBuffer[SHADER_BUFFER_NORMAL]->UpdateData(0, _lightProperties.size() * sizeof(LightProperties), (void*)&_lightProperties.front(), true);
    if(!_lightShadowProperties.empty())
        _lightShaderBuffer[SHADER_BUFFER_SHADOW]->UpdateData(0, _lightShadowProperties.size() * sizeof(LightShadowProperties), (void*)&_lightShadowProperties.front(), true);
}