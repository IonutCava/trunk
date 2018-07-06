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
                                                    _wasBound(false)
{
    _shaderProgramId = 0;//<Override in concrete implementations with appropriate invalid values
    _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = false;

    _maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);
}

ShaderProgram::~ShaderProgram()
{
    D_PRINT_FN(Locale::get("SHADER_PROGRAM_REMOVE"), getName().c_str());
    for_each(ShaderIdMap::value_type& it, _shaderIdMap){
        ShaderManager::getInstance().removeShader(it.second);
    }
    ShaderManager::getInstance().unregisterShaderProgram(getName());
    _shaderIdMap.clear();
}

U8 ShaderProgram::update(const D32 deltaTime){
    ParamHandler& par = ParamHandler::getInstance();
    this->Uniform("dvd_enableFog",par.getParam<bool>("rendering.enableFog"));
    this->Uniform("dvd_lightAmbient", LightManager::getInstance().getAmbientLight());

    if(_dirty){
        this->Uniform("dvd_zPlanes",  Frustum::getInstance().getZPlanes());
        this->Uniform("screenDimension", Application::getInstance().getResolution());
        U8 shadowMapSlot = Config::MAX_TEXTURE_STORAGE;
        //Apply global shader values valid throughout application runtime:
        char depthMapSampler[32];
        for(I32 i = 0; i < Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE; i++){
            sprintf_s(depthMapSampler, "texDepthMapFromLight%d", i);
             //Shadow Maps always bound from the last texture slot upwards
            this->UniformTexture(depthMapSampler,shadowMapSlot++);
        }
        shadowMapSlot =  Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE;
        //Reserve first for directional shadows
        this->UniformTexture("texDepthMapFromLightArray", shadowMapSlot);
         //Reserve second for point shadows
        this->UniformTexture("texDepthMapFromLightCube", shadowMapSlot + 1);

        this->UniformTexture("texNormalMap",    Material::TEXTURE_NORMALMAP);
        this->UniformTexture("texOpacityMap",   Material::TEXTURE_OPACITY);
        this->UniformTexture("texSpecular",     Material::TEXTURE_SPECULAR);

        for(U32 i = Material::TEXTURE_UNIT0, j = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        {
            char uniformSlot[32];
            sprintf_s(uniformSlot, "texDiffuse%d", j++);
            this->UniformTexture(uniformSlot, i);
        }

        this->Uniform("fogColor",  vec3<F32>(par.getParam<F32>("rendering.sceneState.fogColor.r"),
                                             par.getParam<F32>("rendering.sceneState.fogColor.g"),
                                             par.getParam<F32>("rendering.sceneState.fogColor.b")));
        this->Uniform("fogDensity",par.getParam<F32>("rendering.sceneState.fogDensity"));
        this->Uniform("fogStart",  par.getParam<F32>("rendering.sceneState.fogStart"));
        this->Uniform("fogEnd",    par.getParam<F32>("rendering.sceneState.fogEnd"));
        this->Uniform("fogMode",   par.getParam<FogMode>("rendering.sceneState.fogMode"));
        this->Uniform("fogDetailLevel", par.getParam<U8>("rendering.fogDetailLevel", 2));
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
    return HardwareResource::generateHWResource(name);
}

void ShaderProgram::bind(){
    _bound = true;
    _wasBound = true;
    if(_shaderProgramId == 0) 
        return;

    //Apply global shader values valid throughout current render call:
    this->Uniform("dvd_time", static_cast<F32>(GETMSTIME()));
    this->Attribute("dvd_cameraPosition",Frustum::getInstance().getEyePos());
}

void ShaderProgram::uploadNodeMatrices(){
    GFXDevice& GFX = GFX_DEVICE;
    /*Get and upload matrix data*/
    if(this->getUniformLocation("dvd_NormalMatrix") != -1 ){
        GFX.getMatrix(GFXDevice::NORMAL_MATRIX,_cachedNormalMatrix);
        this->Uniform("dvd_NormalMatrix",_cachedNormalMatrix);
    }
    if(this->getUniformLocation("dvd_WorldMatrix") != -1){
		GFX.getMatrix(GFXDevice::WORLD_MATRIX,_cachedMatrix);
        this->Uniform("dvd_WorldMatrix",_cachedMatrix);
    }
    if(this->getUniformLocation("dvd_WorldViewMatrix") != -1){
        GFX.getMatrix(GFXDevice::WV_MATRIX,_cachedMatrix);
        this->Uniform("dvd_WorldViewMatrix",_cachedMatrix);
    }
	if(this->getUniformLocation("dvd_WorldViewMatrixInverse") != -1){
        GFX.getMatrix(GFXDevice::WV_INV_MATRIX,_cachedMatrix);
        this->Uniform("dvd_WorldViewMatrixInverse",_cachedMatrix);
    }
	if(this->getUniformLocation("dvd_WorldViewProjectionMatrix") != -1){
		GFX.getMatrix(GFXDevice::WVP_MATRIX, _cachedMatrix);
        this->Uniform("dvd_WorldViewProjectionMatrix",_cachedMatrix);
    }
    /*Get and upload clip plane data*/
    if(GFX_DEVICE.clippingPlanesDirty()){
        GFX_DEVICE.updateClipPlanes();
        U32 planeCount = GFX_DEVICE.getClippingPlanes().size();
        this->Uniform("dvd_clip_plane_count", (I32)planeCount);
        if(planeCount == 0) return;

        _clipPlanes.resize(planeCount);
        _clipPlanesStates.resize(planeCount, false);
        for(U32 i = 0; i < planeCount; i++){
            const Plane<F32>& currentPlane = GFX_DEVICE.getClippingPlanes()[i];
            _clipPlanes[i] = currentPlane.getEquation();
            _clipPlanesStates[i] = currentPlane.active() ? 1 : 0;
        }

        this->Uniform("dvd_clip_plane", _clipPlanes);
        this->Uniform("dvd_clip_plane_active",_clipPlanesStates);
    }
}

void ShaderProgram::unbind(bool resetActiveProgram){
    _bound = false;
}

vectorImpl<Shader* > ShaderProgram::getShaders(const ShaderType& type) const{
    vectorImpl<Shader* > returnShaders;
    for_each(ShaderIdMap::value_type it, _shaderIdMap){
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
    if(type == FRAGMENT_SHADER)
        _fragUniforms.push_back(uniform);
    else
        _vertUniforms.push_back(uniform);
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

void ShaderProgram::recompile(const bool vertex,
                              const bool fragment,
                              const bool geometry,
                              const bool tessellation)
{
    _compiled = false;
    _wasBound = _bound;

    if(_wasBound) 
        unbind();

    //update refresh tags
    _refreshVert = vertex;
    _refreshFrag = fragment;
    _refreshGeom = geometry;
    _refreshTess = tessellation;
    //recreate all the needed shaders
    generateHWResource(getName());
    //clear refresh tags
    _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = false;

    if(_wasBound)
        bind();
}