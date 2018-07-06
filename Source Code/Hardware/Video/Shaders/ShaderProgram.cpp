#include "Headers/ShaderProgram.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/SceneManager.h"
#include "core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

vec3<F32> ShaderProgram::_cachedCamEye;
vec2<F32> ShaderProgram::_cachedZPlanes;
vec2<F32> ShaderProgram::_cachedSceneZPlanes;

ShaderProgram::ShaderProgram(const bool optimise) : HardwareResource("temp_shader_program"),
                                                    _activeCamera(nullptr),
                                                    _optimise(optimise),
                                                    _linked(false),
                                                    _bound(false),
                                                    _dirty(true),
                                                    _wasBound(false),
                                                    _elapsedTime(0ULL),
                                                    _outputCount(0),
                                                    _elapsedTimeMS(0.0f)
{
    _shaderProgramId = 0;//<Override in concrete implementations with appropriate invalid values
    
    memset(_refreshStage, false, ShaderType_PLACEHOLDER * sizeof(bool));

    _maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);

    _extendedMatricesDirty = true;
    _clipPlanesDirty = true;
    _sceneDataDirty = true;
    _extendedMatrixEntry[WORLD_MATRIX]  = -1;
    _extendedMatrixEntry[WV_MATRIX]     = -1;
    _extendedMatrixEntry[WV_INV_MATRIX] = -1;
    _extendedMatrixEntry[WVP_MATRIX]    = -1;
    _extendedMatrixEntry[NORMAL_MATRIX] = -1;
    _invProjMatrixEntry  = -1;
    _timeLoc             = -1;
    _cameraLocationLoc   = -1;
    _clipPlanesLoc       = -1;
    _clipPlaneCountLoc   = -1;
    _enableFogLoc        = -1;
    _lightAmbientLoc     = -1;
    _zPlanesLoc          = -1;
    _sceneZPlanesLoc     = -1;
    _screenDimensionLoc  = -1;
    _invScreenDimension  = -1;
    _fogColorLoc       = -1;
    _fogDensityLoc     = -1;
    _prevLOD           = 250;
    _prevTextureCount  = 250;
    _lodVertLight.resize(2);
    _lodFragLight.resize(2);
    I32 i = 0, j = 0;
    for (; i < Material::TEXTURE_UNIT0; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation[%d]", Material::TEXTURE_UNIT0 + i);

    for (i = Material::TEXTURE_UNIT0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation[%d]", j++);
}

ShaderProgram::~ShaderProgram()
{
    D_PRINT_FN(Locale::get("SHADER_PROGRAM_REMOVE"), getName().c_str());
    FOR_EACH(ShaderIdMap::value_type& it, _shaderIdMap){
        ShaderManager::getInstance().removeShader(it.second);
    }
    ShaderManager::getInstance().unregisterShaderProgram(getName());
    _shaderIdMap.clear();
}

void ShaderProgram::updateCamera(const Camera& activeCamera) {
    _cachedCamEye = activeCamera.getEye();
    _cachedZPlanes = activeCamera.getZPlanes();
}

U8 ShaderProgram::update(const U64 deltaTime){
    _elapsedTime += deltaTime;
    _elapsedTimeMS = static_cast<F32>(getUsToMs(_elapsedTime));
    if (!isHWInitComplete())
        return 0;

    _activeCamera = Application::getInstance().getKernel()->getCameraMgr().getActiveCamera();
    
    ParamHandler& par = ParamHandler::getInstance();
    LightManager& lightMgr = LightManager::getInstance();
    bool enableFog = par.getParam<bool>("rendering.enableFog");
#ifdef _DEBUG
    this->Uniform("dvd_showShadowSplits", par.getParam<bool>("rendering.debug.showSplits"));
#endif
    this->Uniform(_timeLoc, _elapsedTimeMS);
    this->Uniform(_enableFogLoc, enableFog);
    this->Uniform(_lightAmbientLoc, lightMgr.getAmbientLight());
    if(_sceneDataDirty){
        if(enableFog){
            this->Uniform(_fogColorLoc, vec3<F32>(par.getParam<F32>("rendering.sceneState.fogColor.r"),
                                                  par.getParam<F32>("rendering.sceneState.fogColor.g"),
                                                  par.getParam<F32>("rendering.sceneState.fogColor.b")));
            this->Uniform(_fogDensityLoc, par.getParam<F32>("rendering.sceneState.fogDensity"));
        }
        Scene* activeScene = GET_ACTIVE_SCENE();
        this->Uniform("windDirection", vec2<F32>(activeScene->state().getWindDirX(), activeScene->state().getWindDirZ()));
        this->Uniform("windSpeed", activeScene->state().getWindSpeed());
        _sceneDataDirty = false;
    }
    if(_dirty){
        const vec2<U16>& screenRes = GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution();
        this->Uniform(_screenDimensionLoc, screenRes);
        this->Uniform(_invScreenDimension, vec2<F32>(1.0f / screenRes.width, 1.0f / screenRes.height));
        //Apply global shader values valid throughout application runtime:
        char depthMapSampler1[32], depthMapSampler2[32], depthMapSampler3[32];
        for (I32 i = 0; i < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; i++){
            sprintf_s(depthMapSampler1, "texDepthMapFromLight[%d]",      i);
            sprintf_s(depthMapSampler2, "texDepthMapFromLightArray[%d]", i);
            sprintf_s(depthMapSampler3, "texDepthMapFromLightCube[%d]",  i);
             //Shadow Maps always bound from the last texture slot upwards
            this->UniformTexture(depthMapSampler1, lightMgr.getShadowBindSlot(LightManager::SHADOW_SLOT_TYPE_NORMAL, i));
            this->UniformTexture(depthMapSampler2, lightMgr.getShadowBindSlot(LightManager::SHADOW_SLOT_TYPE_ARRAY,  i));
            this->UniformTexture(depthMapSampler3, lightMgr.getShadowBindSlot(LightManager::SHADOW_SLOT_TYPE_CUBE,   i));
        }

        char textureSampler[32];
        for (I32 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
            sprintf_s(textureSampler, "texDiffuse[%d]", i);
            //Shadow Maps always bound from the last texture slot upwards
            this->UniformTexture(textureSampler, Material::TEXTURE_UNIT0 + i);
        }
        this->Uniform("dvd_lightBleedBias", 0.0000002f);
        this->Uniform("dvd_minShadowVariance", 0.0002f);
        this->Uniform("dvd_shadowMaxDist", 250.0f);
        this->Uniform("dvd_shadowFadeDist",150.0f);
        _dirty = false;
    }

    return 1;
}

bool ShaderProgram::generateHWResource(const std::string& name){
    _name = name;

    if (!HardwareResource::generateHWResource(name))
        return false;

    HardwareResource::threadedLoad(name);

    DIVIDE_ASSERT(isHWInitComplete(), "ShaderProgram error: hardware initialization failed!");

    _extendedMatrixEntry[WORLD_MATRIX]  = this->cachedLoc("dvd_WorldMatrix[0]");
    _extendedMatrixEntry[WV_MATRIX]     = this->cachedLoc("dvd_WorldViewMatrix");
    _extendedMatrixEntry[WV_INV_MATRIX] = this->cachedLoc("dvd_WorldViewMatrixInverse");
    _extendedMatrixEntry[WVP_MATRIX]    = this->cachedLoc("dvd_WorldViewProjectionMatrix");
    _extendedMatrixEntry[NORMAL_MATRIX] = this->cachedLoc("dvd_NormalMatrix[0]");
    _invProjMatrixEntry  = this->cachedLoc("dvd_ProjectionMatrixInverse");
    _timeLoc             = this->cachedLoc("dvd_time");
    _cameraLocationLoc   = this->cachedLoc("dvd_cameraPosition");
    _clipPlanesLoc       = this->cachedLoc("dvd_clip_plane");
    _clipPlaneCountLoc   = this->cachedLoc("dvd_clip_plane_count");
    _enableFogLoc        = this->cachedLoc("dvd_enableFog");
    _lightAmbientLoc     = this->cachedLoc("dvd_lightAmbient");
    _zPlanesLoc          = this->cachedLoc("dvd_zPlanes");
    _sceneZPlanesLoc     = this->cachedLoc("dvd_sceneZPlanes");
    _screenDimensionLoc  = this->cachedLoc("dvd_screenDimension");
    _invScreenDimension  = this->cachedLoc("dvd_invScreenDimension");
    _fogColorLoc         = this->cachedLoc("fogColor");
    _fogDensityLoc       = this->cachedLoc("fogDensity");

    _lodVertLight[0] = GetSubroutineIndex(VERTEX_SHADER, "computeLightInfoLOD0");
    _lodVertLight[1] = GetSubroutineIndex(VERTEX_SHADER, "computeLightInfoLOD1");

    _lodFragLight[0] = GetSubroutineIndex(FRAGMENT_SHADER, "computeLightInfoLOD0Frag");
    _lodFragLight[1] = GetSubroutineIndex(FRAGMENT_SHADER, "computeLightInfoLOD1Frag");
    _dirty = true;

    return true;
}

bool ShaderProgram::bind(){
    _bound = _wasBound = true;
    return _shaderProgramId != 0;
}

void ShaderProgram::uploadNodeMatrices(){
    DIVIDE_ASSERT(_bound, "ShaderProgram error: tried to upload transform data to an unbound shader program!");

    GFXDevice& GFX = GFX_DEVICE;

    this->Uniform(_cameraLocationLoc, _cachedCamEye);
    this->Uniform(_zPlanesLoc, _cachedZPlanes);
    this->Uniform(_sceneZPlanesLoc, _cachedSceneZPlanes);

    I32 currentLocation = -1;
    /*Get and upload matrix data*/
    if (_extendedMatricesDirty == true){

        currentLocation = _extendedMatrixEntry[NORMAL_MATRIX];
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix3(NORMAL_MATRIX));
        }
        
        currentLocation = _extendedMatrixEntry[WORLD_MATRIX];
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix4(WORLD_MATRIX));
        }

        currentLocation = _extendedMatrixEntry[WV_MATRIX];
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix4(WV_MATRIX));
        }

        currentLocation = _extendedMatrixEntry[WV_INV_MATRIX];
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix4(WV_INV_MATRIX));
        }
 
        currentLocation = _extendedMatrixEntry[WVP_MATRIX];
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix4(WVP_MATRIX));
        }

        currentLocation = _invProjMatrixEntry;
        if (currentLocation != -1){
            this->Uniform(currentLocation, GFX.getMatrix(PROJECTION_INV_MATRIX));
        }
        _extendedMatricesDirty = false;
    }

    /*Get and upload clip plane data*/
    //GFX_DEVICE.updateClipPlanes();

    if (_clipPlanesDirty == true){
        _clipPlanesDirty = false;
        _clipPlanes.resize(0);

        for (const Plane<F32>& currentPlane : GFX.getClippingPlanes())
            _clipPlanes.push_back(currentPlane.getEquation());

        this->Uniform(_clipPlaneCountLoc, (I32)_clipPlanes.size());

        if (_clipPlanes.empty()) return;

        this->Uniform(_clipPlanesLoc, _clipPlanes);
    }
}

void ShaderProgram::ApplyMaterial(Material* const material){
    if (!material) return;

    Texture* texture = nullptr;
    for (U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if ((texture = material->getTexture(i)) != nullptr){
            texture->Bind(i);
            if (i >= Material::TEXTURE_UNIT0)
                Uniform(_textureOperationUniformSlots[i], (I32)material->getTextureOperation(i));
        }
    }
    
    if (material->isTranslucent()){
        Uniform("opacity",      material->getOpacityValue());
        Uniform("useAlphaTest", material->useAlphaTest() || GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE));
    }

    Uniform("material",     material->getMaterialMatrix());
    U8 currentTextureCount = material->getTextureCount();
    if(_prevTextureCount != currentTextureCount){
        Uniform("textureCount", material->getTextureCount());
        _prevTextureCount = currentTextureCount;
    }
}

void ShaderProgram::SetLOD(U8 currentLOD){
    SetSubroutine(VERTEX_SHADER, currentLOD == 0 ? _lodVertLight[0] : _lodVertLight[1]);
    SetSubroutine(FRAGMENT_SHADER, currentLOD == 0 ? _lodFragLight[0] : _lodFragLight[1]);

    if (currentLOD != _prevLOD){
        Uniform("lodLevel", (I32)currentLOD);
        _prevLOD = currentLOD;
    }
}

void ShaderProgram::unbind(bool resetActiveProgram){
    _bound = false;
}

vectorImpl<Shader* > ShaderProgram::getShaders(const ShaderType& type) const{
    static vectorImpl<Shader* > returnShaders;

    returnShaders.clear();
    FOR_EACH(ShaderIdMap::value_type it, _shaderIdMap){
        if(it.second->getType() == type){
            returnShaders.push_back(it.second);
        }
    }
    return returnShaders;
}

void ShaderProgram::removeShaderDefine(const std::string& define){
    vectorImpl<std::string >::iterator it = find(_definesList.begin(), _definesList.end(), define);
    if (it != _definesList.end()) _definesList.erase(it);
    else D_ERROR_FN(Locale::get("ERROR_INVALID_SHADER_DEFINE_DELETE"),define.c_str(),getName().c_str());
}

void ShaderProgram::addShaderUniform(const std::string& uniform, const ShaderType& type) {
    _customUniforms[type].push_back(uniform);
}

void ShaderProgram::removeUniform(const std::string& uniform, const ShaderType& type) {
    vectorImpl<std::string >::const_iterator it = find(_customUniforms[type].begin(), _customUniforms[type].end(), uniform);
    if (it != _customUniforms[type].end()) _customUniforms[type].erase(it);
    else D_ERROR_FN(Locale::get("ERROR_INVALID_SHADER_UNIFORM_DELETE"), uniform.c_str(), (U32)type , getName().c_str());
}

void ShaderProgram::recompile(const bool vertex, const bool fragment, const bool geometry, const bool tessellation, const bool compute){
    _linked = false;
    _wasBound = _bound;

    if(_wasBound) unbind();

    //update refresh tags
    _refreshStage[VERTEX_SHADER] = vertex;
    _refreshStage[FRAGMENT_SHADER] = fragment;
    _refreshStage[GEOMETRY_SHADER] = geometry;
    _refreshStage[TESSELATION_CTRL_SHADER] = tessellation;
    _refreshStage[TESSELATION_EVAL_SHADER] = tessellation;
    _refreshStage[COMPUTE_SHADER] = compute;
    //recreate all the needed shaders
    generateHWResource(getName());

    if(_wasBound)  bind();
}