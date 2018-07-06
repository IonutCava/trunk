#include "Headers/ShaderProgram.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"

ShaderProgram::ShaderProgram(const bool optimise) : HardwareResource(),
                                                    _optimise(optimise),
                                                    _useTessellation(false),
                                                    _useGeometry(false),
                                                    _useFragment(true),
                                                    _useVertex(true),
                                                    _compiled(false),
                                                    _bound(false),
                                                    _dirty(true),
                                                    _wasBound(false),
                                                    _elapsedTime(0ULL)
{
    _shaderProgramId = 0;//<Override in concrete implementations with appropriate invalid values
    _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = false;

    _maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);

    _extendedMatricesDirty = true;
 
    _extendedMatrixEntry[WORLD_MATRIX]  = -1;
    _extendedMatrixEntry[WV_MATRIX]     = -1;
    _extendedMatrixEntry[WV_INV_MATRIX] = -1;
    _extendedMatrixEntry[WVP_MATRIX]    = -1;
    _extendedMatrixEntry[NORMAL_MATRIX] = -1;
    _timeLoc             = -1;
    _cameraLocationLoc   = -1;
    _clipPlanesLoc       = -1;
    _clipPlaneCountLoc   = -1;
    _clipPlanesActiveLoc = -1;
    _enableFogLoc        = -1;
    _lightAmbientLoc     = -1;
    _zPlanesLoc          = -1;
    _screenDimensionLoc  = -1;
    _texDepthMapFromLightArrayLoc = -1;
    _texDepthMapFromLightCubeLoc  = -1;
    _texNormalMapLoc   = -1;
    _texOpacityMapLoc  = -1;
    _texSpecularLoc    = -1;
    _fogColorLoc       = -1;
    _fogDensityLoc     = -1;
    _fogStartLoc       = -1;
    _fogEndLoc         = -1;
    _fogModeLoc        = -1;
    _fogDetailLevelLoc = -1;
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

U8 ShaderProgram::update(const U64 deltaTime){
    _elapsedTime += deltaTime;
    ParamHandler& par = ParamHandler::getInstance();
    bool enableFog = par.getParam<bool>("rendering.enableFog");
    this->Uniform(_enableFogLoc, enableFog);
    this->Uniform(_lightAmbientLoc, LightManager::getInstance().getAmbientLight());

    if(_dirty){
        this->Uniform(_zPlanesLoc, Frustum::getInstance().getZPlanes());
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

        for(U32 i = Material::TEXTURE_UNIT0, j = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
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
            this->Uniform(_fogStartLoc,   par.getParam<F32>("rendering.sceneState.fogStart"));
            this->Uniform(_fogEndLoc,     par.getParam<F32>("rendering.sceneState.fogEnd"));
            this->Uniform(_fogModeLoc,    par.getParam<FogMode>("rendering.sceneState.fogMode"));
            this->Uniform(_fogDetailLevelLoc, par.getParam<U8>("rendering.fogDetailLevel", 2));
        }
        _dirty = false;
    }

    return 1;
}

void ShaderProgram::threadedLoad(const std::string& name){
    if(ShaderProgram::generateHWResource(name)) 
        HardwareResource::threadedLoad(name);

    update(0.0);
}

bool ShaderProgram::generateHWResource(const std::string& name){
    _name = name;
    if (!HardwareResource::generateHWResource(name))
        return false;
    
    _extendedMatrixEntry[WORLD_MATRIX]  = this->cachedLoc("dvd_WorldMatrix");
    _extendedMatrixEntry[WV_MATRIX]     = this->cachedLoc("dvd_WorldViewMatrix");
    _extendedMatrixEntry[WV_INV_MATRIX] = this->cachedLoc("dvd_WorldViewMatrixInverse");
    _extendedMatrixEntry[WVP_MATRIX]    = this->cachedLoc("dvd_WorldViewProjectionMatrix");
    _extendedMatrixEntry[NORMAL_MATRIX] = this->cachedLoc("dvd_NormalMatrix");
    _timeLoc             = this->cachedLoc("dvd_time");
    _cameraLocationLoc   = this->cachedLoc("dvd_cameraPosition", false);
    _clipPlanesLoc       = this->cachedLoc("dvd_clip_plane");
    _clipPlaneCountLoc   = this->cachedLoc("dvd_clip_plane_count");
    _clipPlanesActiveLoc = this->cachedLoc("dvd_clip_plane_active");
    _enableFogLoc        = this->cachedLoc("dvd_enableFog");
    _lightAmbientLoc     = this->cachedLoc("dvd_lightAmbient");
    _zPlanesLoc          = this->cachedLoc("dvd_zPlanes");
    _screenDimensionLoc  = this->cachedLoc("screenDimension");
    _texDepthMapFromLightArrayLoc = this->cachedLoc("texDepthMapFromLightArray");
    _texDepthMapFromLightCubeLoc  = this->cachedLoc("texDepthMapFromLightCube");
    _texNormalMapLoc   = this->cachedLoc("texNormalMap");
    _texOpacityMapLoc  = this->cachedLoc("texOpacityMap");
    _texSpecularLoc    = this->cachedLoc("texSpecular");
    _fogColorLoc       = this->cachedLoc("fogColor");
    _fogDensityLoc     = this->cachedLoc("fogDensity");
    _fogStartLoc       = this->cachedLoc("fogStart");
    _fogEndLoc         = this->cachedLoc("fogEnd");
    _fogModeLoc        = this->cachedLoc("fogMode");
    _fogDetailLevelLoc = this->cachedLoc("fogDetailLevel");

    return true;
}

void ShaderProgram::bind(){
    _bound = true;
    _wasBound = true;
    if(_shaderProgramId == 0) 
        return;

    //Apply global shader values valid throughout current render call:
    this->Uniform(_timeLoc, static_cast<F32>(getUsToMs(_elapsedTime)));
    this->Attribute(_cameraLocationLoc, Frustum::getInstance().getEyePos());
}

void ShaderProgram::uploadNodeMatrices(){
    GFXDevice& GFX = GFX_DEVICE;
    I32 currentLocation = -1;
    /*Get and upload matrix data*/
    if (_extendedMatricesDirty == true){

        currentLocation = _extendedMatrixEntry[NORMAL_MATRIX];
        if (currentLocation != -1){
            GFX.getMatrix(NORMAL_MATRIX, _cachedNormalMatrix);
            this->Uniform(currentLocation, _cachedNormalMatrix);
        }
        
        currentLocation = _extendedMatrixEntry[WORLD_MATRIX];
        if (currentLocation != -1){
            GFX.getMatrix(WORLD_MATRIX, _cachedMatrix);
            this->Uniform(currentLocation, _cachedMatrix);
        }

        currentLocation = _extendedMatrixEntry[WV_INV_MATRIX];
        if (currentLocation != -1){
            GFX.getMatrix(WV_MATRIX, _cachedMatrix);
            this->Uniform(currentLocation, _cachedMatrix);
        }

        currentLocation = _extendedMatrixEntry[WV_INV_MATRIX];
        if (currentLocation != -1){
            GFX.getMatrix(WV_INV_MATRIX, _cachedMatrix);
            this->Uniform(currentLocation, _cachedMatrix);
        }
 
        currentLocation = _extendedMatrixEntry[WVP_MATRIX];
        if (currentLocation != -1){
            GFX.getMatrix(WVP_MATRIX, _cachedMatrix);
            this->Uniform(currentLocation, _cachedMatrix);
        }

        _extendedMatricesDirty = false;
    }
    
    /*Get and upload clip plane data*/
    if (GFX.clippingPlanesDirty()){
        GFX.updateClipPlanes();
        size_t planeCount = GFX.getClippingPlanes().size();
        this->Uniform(_clipPlaneCountLoc, (I32)planeCount);
        _clipPlanes.resize(0);
        _clipPlanes.reserve(planeCount);

        _clipPlanesStates.resize(0);
        _clipPlanesStates.reserve(planeCount);

        if (planeCount == 0) return;
        for (const Plane<F32>& currentPlane : GFX.getClippingPlanes()){
            _clipPlanes.push_back(currentPlane.getEquation());
            _clipPlanesStates.push_back(currentPlane.active() ? 1 : 0);
        }

        this->Uniform(_clipPlanesLoc, _clipPlanes);
        this->Uniform(_clipPlanesActiveLoc, _clipPlanesStates);
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

void ShaderProgram::recompile(const bool vertex, const bool fragment, const bool geometry, const bool tessellation){
    _compiled = false;
    _wasBound = _bound;

    if(_wasBound) unbind();

    //update refresh tags
    _refreshVert = vertex;
    _refreshFrag = fragment;
    _refreshGeom = geometry;
    _refreshTess = tessellation;
    //recreate all the needed shaders
    generateHWResource(getName());
    //clear refresh tags
    _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = false;

    if(_wasBound)  bind();
}