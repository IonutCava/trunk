#include "Headers/ShaderProgram.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
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
    _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = _refreshComp = false;

    _maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);

    _extendedMatricesDirty = true;
    _clipPlanesDirty = true;

    _extendedMatrixEntry[WORLD_MATRIX]  = -1;
    _extendedMatrixEntry[WV_MATRIX]     = -1;
    _extendedMatrixEntry[WV_INV_MATRIX] = -1;
    _extendedMatrixEntry[WVP_MATRIX]    = -1;
    _extendedMatrixEntry[NORMAL_MATRIX] = -1;
    _timeLoc             = -1;
    _cameraLocationLoc   = -1;
    _clipPlanesLoc       = -1;
    _clipPlaneCountLoc   = -1;
    _enableFogLoc        = -1;
    _lightAmbientLoc     = -1;
    _zPlanesLoc          = -1;
    _sceneZPlanesLoc     = -1;
    _screenDimensionLoc  = -1;
    _texDepthMapFromLightArrayLoc = -1;
    _texDepthMapFromLightCubeLoc  = -1;
    _texNormalMapLoc   = -1;
    _texOpacityMapLoc  = -1;
    _texSpecularLoc    = -1;
    _fogColorLoc       = -1;
    _fogDensityLoc     = -1;
    _prevLOD           = 250;
    _lod0VertLight.resize(1);
    _lod1VertLight.resize(1);
    I32 i = 0, j = 0;
    for (; i < Material::TEXTURE_UNIT0; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", Material::TEXTURE_UNIT0 + i);

    for (i = Material::TEXTURE_UNIT0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", j++);
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
    bool enableFog = par.getParam<bool>("rendering.enableFog");
#ifdef _DEBUG
    this->Uniform("dvd_showShadowSplits", par.getParam<bool>("rendering.debug.showSplits"));
#endif
    this->Uniform(_timeLoc, _elapsedTimeMS);
    this->Uniform(_enableFogLoc, enableFog);
    this->Uniform(_lightAmbientLoc, LightManager::getInstance().getAmbientLight());
    if(_dirty){
        this->Uniform(_screenDimensionLoc, Application::getInstance().getResolution());

        U8 shadowMapSlot = Config::MAX_TEXTURE_STORAGE;
        //Apply global shader values valid throughout application runtime:
        char depthMapSampler[32];
        for(I32 i = 0; i < Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; i++){
            sprintf_s(depthMapSampler, "texDepthMapFromLight[%d]", i);
             //Shadow Maps always bound from the last texture slot upwards
            this->UniformTexture(depthMapSampler,shadowMapSlot++);
        }
        shadowMapSlot =  Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE;
        //Reserve first for directional shadows
        this->UniformTexture(_texDepthMapFromLightArrayLoc, shadowMapSlot);
         //Reserve second for point shadows
        this->UniformTexture(_texDepthMapFromLightCubeLoc, shadowMapSlot + 1);

        this->UniformTexture(_texNormalMapLoc,  Material::TEXTURE_NORMALMAP);
        this->UniformTexture(_texOpacityMapLoc, Material::TEXTURE_OPACITY);
        this->UniformTexture(_texSpecularLoc,   Material::TEXTURE_SPECULAR);

        for(I32 i = Material::TEXTURE_UNIT0, j = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        {
            char uniformSlot[32];
            sprintf_s(uniformSlot, "texDiffuse%d", j++);
            this->UniformTexture(uniformSlot, i);
        }

        if(enableFog){
            this->Uniform(_fogColorLoc, vec3<F32>(par.getParam<F32>("rendering.sceneState.fogColor.r"),
                                                  par.getParam<F32>("rendering.sceneState.fogColor.g"),
                                                  par.getParam<F32>("rendering.sceneState.fogColor.b")));
            this->Uniform(_fogDensityLoc, par.getParam<F32>("rendering.sceneState.fogDensity"));
        }
        _dirty = false;
    }

    return 1;
}

bool ShaderProgram::generateHWResource(const std::string& name){
    _name = name;

    if (!HardwareResource::generateHWResource(name))
        return false;

    HardwareResource::threadedLoad(name);

    assert(isHWInitComplete());

    _extendedMatrixEntry[WORLD_MATRIX]  = this->cachedLoc("dvd_WorldMatrix");
    _extendedMatrixEntry[WV_MATRIX]     = this->cachedLoc("dvd_WorldViewMatrix");
    _extendedMatrixEntry[WV_INV_MATRIX] = this->cachedLoc("dvd_WorldViewMatrixInverse");
    _extendedMatrixEntry[WVP_MATRIX]    = this->cachedLoc("dvd_WorldViewProjectionMatrix");
    _extendedMatrixEntry[NORMAL_MATRIX] = this->cachedLoc("dvd_NormalMatrix");
    _timeLoc             = this->cachedLoc("dvd_time");
    _cameraLocationLoc   = this->cachedLoc("dvd_cameraPosition", false);
    _clipPlanesLoc       = this->cachedLoc("dvd_clip_plane");
    _clipPlaneCountLoc   = this->cachedLoc("dvd_clip_plane_count");
    _enableFogLoc        = this->cachedLoc("dvd_enableFog");
    _lightAmbientLoc     = this->cachedLoc("dvd_lightAmbient");
    _zPlanesLoc          = this->cachedLoc("dvd_zPlanes");
    _sceneZPlanesLoc     = this->cachedLoc("dvd_sceneZPlanes");
    _screenDimensionLoc  = this->cachedLoc("screenDimension");
    _texDepthMapFromLightArrayLoc = this->cachedLoc("texDepthMapFromLightArray");
    _texDepthMapFromLightCubeLoc  = this->cachedLoc("texDepthMapFromLightCube");
    _texNormalMapLoc   = this->cachedLoc("texNormalMap");
    _texOpacityMapLoc  = this->cachedLoc("texOpacityMap");
    _texSpecularLoc    = this->cachedLoc("texSpecular");
    _fogColorLoc       = this->cachedLoc("fogColor");
    _fogDensityLoc     = this->cachedLoc("fogDensity");

    _lod0VertLight[0] = GetSubroutineIndex(VERTEX_SHADER, "computeLightInfoLOD0");
    _lod1VertLight[0] = GetSubroutineIndex(VERTEX_SHADER, "computeLightInfoLOD1");

    _dirty = true;

    return true;
}

bool ShaderProgram::bind(){
    _bound = _wasBound = true;
    return _shaderProgramId != 0;
}

void ShaderProgram::uploadNodeMatrices(){
    assert(_bound);
    GFXDevice& GFX = GFX_DEVICE;

    this->Attribute(_cameraLocationLoc, _cachedCamEye);
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

        currentLocation = _extendedMatrixEntry[WV_INV_MATRIX];
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

        _extendedMatricesDirty = false;
    }
    
    /*Get and upload clip plane data*/
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
    for (U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if (material->getTexture(i)){
            if (i >= Material::TEXTURE_UNIT0)
                Uniform(_textureOperationUniformSlots[i], (I32)material->getTextureOperation(i));
        }
    }
    
    if (material->isTranslucent()){
        Uniform("opacity",      material->getOpacityValue());
        Uniform("useAlphaTest", material->useAlphaTest() || GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE));
    }

    Uniform("material",     material->getMaterialMatrix());
    Uniform("textureCount", material->getTextureCount());
}

void ShaderProgram::SetLOD(U8 currentLOD){
    SetSubroutines(VERTEX_SHADER, currentLOD == 0 ? _lod0VertLight : _lod1VertLight);

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
    if(type == FRAGMENT_SHADER) _fragUniforms.push_back(uniform);
    else                        _vertUniforms.push_back(uniform);
}

void ShaderProgram::removeUniform(const std::string& uniform, const ShaderType& type) {
    vectorImpl<std::string >::iterator it;
    if(type == FRAGMENT_SHADER){
        it = find(_fragUniforms.begin(), _fragUniforms.end(), uniform);
        if (it != _fragUniforms.end()) _fragUniforms.erase(it);
        else D_ERROR_FN(Locale::get("ERROR_INVALID_SHADER_UNIFORM_DELETE"),uniform.c_str(),"fragment", getName().c_str());
    }else{
        it = find(_vertUniforms.begin(), _vertUniforms.end(), uniform);
        if (it != _vertUniforms.end()) _vertUniforms.erase(it);
        else D_ERROR_FN(Locale::get("ERROR_INVALID_SHADER_DEFINE_DELETE"),uniform.c_str(), "vertex", getName().c_str());
    }
}

void ShaderProgram::recompile(const bool vertex, const bool fragment, const bool geometry, const bool tessellation, const bool compute){
    _linked = false;
    _wasBound = _bound;

    if(_wasBound) unbind();

    //update refresh tags
    _refreshVert = vertex;
    _refreshFrag = fragment;
    _refreshGeom = geometry;
    _refreshTess = tessellation;
    _refreshComp = compute;
    //recreate all the needed shaders
    generateHWResource(getName());

    if(_wasBound)  bind();
}