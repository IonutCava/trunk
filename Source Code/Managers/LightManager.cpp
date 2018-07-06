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

LightManager::LightManager() : FrameListener(),
                               _shadowMapsEnabled(true),
                               _previewShadowMaps(false),
                               _dominantLight(nullptr),
                               _currentShadowPass(0)
{
    s_shadowPassTimer = ADD_TIMER("ShadowPassTimer");
    _lightShaderBuffer  = GFX_DEVICE.newSB();
    _shadowShaderBuffer = GFX_DEVICE.newSB();

    memset(normShadowLocation, -1,  Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));
    memset(cubeShadowLocation, -1,  Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));
    memset(arrayShadowLocation, -1, Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * sizeof(I32));
}

LightManager::~LightManager()
{
    clear();
    REMOVE_TIMER(s_shadowPassTimer);
    SAFE_DELETE(_lightShaderBuffer);
    SAFE_DELETE(_shadowShaderBuffer);
}

void LightManager::init(){
    REGISTER_FRAME_LISTENER(&(this->getInstance()), 2);
    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&LightManager::previewShadowMaps, this, nullptr), 1);
    _lightShaderBuffer->Create(true, false);
    _shadowShaderBuffer->Create(true, false);
    _lightShaderBuffer->ReserveBuffer(Config::MAX_LIGHTS_PER_SCENE, sizeof(LightProperties));
    _shadowShaderBuffer->ReserveBuffer(Config::MAX_LIGHTS_PER_SCENE, sizeof(LightShadowProperties));
    _lightShaderBuffer->bind(Divide::SHADER_BUFFER_LIGHT_NORMAL);
    _shadowShaderBuffer->bind(Divide::SHADER_BUFFER_LIGHT_SHADOW);

    I32 maxTextureStorage = GFX_DEVICE.getMaxTextureUnits();
    I32 maxSlotsPerLight = 3;
    maxTextureStorage -= Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * maxSlotsPerLight;

    for (U8 i = 0; i < Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; ++i){
        normShadowLocation[i]  = maxTextureStorage + 0 + (i * maxSlotsPerLight);
        cubeShadowLocation[i]  = maxTextureStorage + 1 + (i * maxSlotsPerLight);
        arrayShadowLocation[i] = maxTextureStorage + 2 + (i * maxSlotsPerLight);
    }
}

bool LightManager::clear(){
    //Lights are removed by the sceneGraph
    FOR_EACH(LightMap::value_type it, _lights){
        //in case we had some light hanging
        RemoveResource(it.second);
    }

    _lights.clear();

    return _lights.empty();
}

bool LightManager::addLight(Light* const light){
    if(light->getId() == 0)
        light->setId(generateNewID());

    light->addShadowMapInfo(New ShadowMapInfo(light));

    if(_lights.find(light->getId()) != _lights.end()){
        ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"),light->getId());
        return false;
    }

    light->setSlot((U8)_lights.size());
    _lights.insert(std::make_pair(light->getId(),light));
    GET_ACTIVE_SCENE()->renderState().getCameraMgr().addNewCamera(light->getName(), light->shadowCamera());
    return true;
}

// try to remove any leftover lights
bool LightManager::removeLight(U32 lightId){
    /// we can't remove a light if the light list is empty. That light has to exist somewhere!
    assert(!_lights.empty());

    LightMap::iterator it = _lights.find(lightId);

    if(it == _lights.end()){
        ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_REMOVE_LIGHT"),lightId);
        return false;
    }

    _lights.erase(it); //remove it from the map
    return true;
}

U32 LightManager::generateNewID(){
    U32 tempId = (U32)_lights.size();

    while(!checkId(tempId))
        tempId++;

    return tempId;
}

bool LightManager::checkId(U32 value){
    if (_lights.empty())
        return true;

    FOR_EACH(LightMap::value_type& it, _lights)
        if(it.second->getId() == value)
            return false;

    return true;
}

void LightManager::idle(){
    _shadowMapsEnabled = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");

    s_shadowPassTimer->pause(!_shadowMapsEnabled);
}

void LightManager::updateResolution(I32 newWidth, I32 newHeight){
    for(Light* light : _currLightsPerNode)
        light->updateResolution(newWidth, newHeight);
}

///Check light properties for every light (this is bound to the camera change listener group
///Update only if needed. Get projection and view matrices if they changed
///Also, search for the dominant light if any
void LightManager::update(const bool force){
    GFX_DEVICE.getMatrix(VIEW_MATRIX, _viewMatrixCache);

    for(Light* light : _currLightsPerNode){
        assert(light != nullptr);
        light->updateState(force);
        if(!_dominantLight){ //if we do not have a dominant light registered, search for one
            if(light->getLightMode() == LIGHT_MODE_DOMINANT){
                //setting a light as dominant, will automatically inform the lightmanager, but just in case, make sure
                _dominantLight = light;
            }
        }
    }
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
    // if we have a dominant light, generate only it's shadows
    if(_dominantLight){
        setCurrentLight(_dominantLight);
         // When the entire scene is ready for rendering, generate the shadowmaps
        _dominantLight->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
    }else{
        //generate shadowmaps for each light
        FOR_EACH(LightMap::value_type& light, _lights){
            setCurrentLight(light.second);
            light.second->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
        }
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

    FOR_EACH(LightMap::value_type& it, _lights)
        it.second->getShadowMapInfo()->getShadowMap()->togglePreviewShadowMaps(_previewShadowMaps);
}

void LightManager::previewShadowMaps(Light* light) {
    //Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !_previewShadowMaps || !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))
        return;

    Light* localLight = light;

    if(_dominantLight){
        localLight = _dominantLight;
    }else{
        if(localLight == nullptr)
            localLight = _lights[0];
    }

    if(!localLight->castsShadows())
        return;

    localLight->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
}

//If we have computed shadowmaps, bind them before rendering any geometry;
//Always bind shadowmaps to slots Config::MAX_TEXTURE_STORAGE, Config::MAX_TEXTURE_STORAGE+1, Config::MAX_TEXTURE_STORAGE+2 ...
void LightManager::bindDepthMaps(U8 lightIndex, bool overrideDominant){
    //Skip applying shadows if we are rendering to depth map, or we have shadows disabled
    if(!_shadowMapsEnabled)
        return;

    Light* lightLocal = getLightForCurrentNode(lightIndex);
    ///If we have a dominant light, then both shadow casting lights are the same = the dominant one
    ///Shadow map binding has a failsafe check for this, so it's ok to call bind twice
    if(_dominantLight && !overrideDominant)
        lightLocal = _dominantLight;

    if(!lightLocal->castsShadows())
        return;

    ShadowMap* sm = lightLocal->getShadowMapInfo()->getShadowMap();
    if(sm)
        sm->Bind(getShadowBindSlot(lightLocal->getLightType(), lightIndex));
}

bool LightManager::shadowMappingEnabled() const {
    if(!_shadowMapsEnabled)
        return false;

    FOR_EACH(LightMap::value_type light, _lights){
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

struct scoreCmpFnc{
    bool operator()(Light* const a, Light* const b) const {
        return (a)->getScore() > (b)->getScore();
    }
};

U8 LightManager::findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter){
    const vec3<F32> lumDot( 0.2125f, 0.7154f, 0.0721f );
    F32 luminace = 0.0f;
    F32 dist = 0.0f;
    F32 weight = 1.0f; // later
    U8 i = 0;

    // Reset light buffer
    _tempLightsPerNode.resize(_lights.size());
    _currLightsPerNode.resize(0);
    _currLightTypes.resize(0);
    _currLightIndices.resize(0);
    _currShadowLights.resize(0);
    // loop over every light in the scene
    // ToDo: add a grid based light search system? -Ionut
    FOR_EACH(LightMap::value_type& lightIt, _lights){
        Light* light = lightIt.second;
        if(!light->getEnabled())
            continue;

        LightType lType = light->getLightType();
        if(lType != LIGHT_TYPE_DIRECTIONAL )  {
            // get the luminosity.
            luminace  = light->getVProperty(LIGHT_PROPERTY_DIFFUSE).dot(lumDot);
            luminace *= light->getFProperty(LIGHT_PROPERTY_BRIGHTNESS);

            F32 radiusSq = squared(light->getFProperty(LIGHT_PROPERTY_BRIGHTNESS) + node->getBoundingSphereConst().getRadius());
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
        if((typeFilter != LIGHT_TYPE_PLACEHOLDER && lType == typeFilter) || //< whether we have the correct light type
            typeFilter == LIGHT_TYPE_PLACEHOLDER){ //< or we did not specify a type filter
            _tempLightsPerNode[i++] = light;
        }
    }

    // sort the lights by score
    std::sort(_tempLightsPerNode.begin(),
              _tempLightsPerNode.end(),
              scoreCmpFnc());

    // create the light buffer for the specified node
    size_t maxLights = _tempLightsPerNode.size();
    CLAMP<size_t>(maxLights, 0, Config::MAX_LIGHTS_PER_SCENE_NODE);
    for(i = 0; i < maxLights; i++){
        _currLightsPerNode.push_back(_tempLightsPerNode[i]);
        _currLightTypes.push_back(_tempLightsPerNode[i]->getLightType());
        _currLightIndices.push_back(_tempLightsPerNode[i]->getSlot());
        _currShadowLights.push_back(_tempLightsPerNode[i]->castsShadows() ? 1 : 0);
    }

    return (U8)maxLights;
}

///Update OpenGL light state
void LightManager::setLight(Light* const light, bool shadowPass){
    assert(light != nullptr);
    if (shadowPass){
        _shadowShaderBuffer->ChangeSubData(light->getSlot() * sizeof(LightShadowProperties), sizeof(LightShadowProperties), (GLvoid*)&light->getShadowProperties());
    }else{
        LightProperties temp = light->getProperties();

        if (light->getLightType() == LIGHT_TYPE_DIRECTIONAL){
            temp._position.set(vec3<F32>(_viewMatrixCache * temp._position), temp._position.w);
        }
        else if (light->getLightType() == LIGHT_TYPE_SPOT){
            temp._direction.set(vec3<F32>(_viewMatrixCache * temp._direction), temp._direction.w);
        }
        _lightShaderBuffer->ChangeSubData(light->getSlot() * sizeof(LightProperties), sizeof(LightProperties), (GLvoid*)&temp);
    }
}