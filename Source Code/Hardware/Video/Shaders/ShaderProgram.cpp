#include "Headers/ShaderProgram.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"

ShaderProgram::ShaderProgram(const bool optimise) : HardwareResource(),
													_invalidShaderProgramId(std::numeric_limits<U32>::max()),
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
		_shaderProgramId = _invalidShaderProgramId;
		_refreshVert = _refreshFrag = _refreshGeom = _refreshTess = false;
        //Enable all matrix uploads by default (note: projection and view matrices are ALWAYS uploaded)
        _matrixMask.b.b0 = 1;
        _matrixMask.b.b1 = 1;
        _matrixMask.b.b2 = 1;
        _matrixMask.b.b3 = 1;
		_maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);
    }

ShaderProgram::~ShaderProgram(){
	D_PRINT_FN(Locale::get("SHADER_PROGRAM_REMOVE"), getName().c_str());
	for_each(ShaderIdMap::value_type& it, _shaderIdMap){
		ShaderManager::getInstance().removeShader(it.second);
	}
	ShaderManager::getInstance().unregisterShaderProgram(getName());
	_shaderIdMap.clear();
}

U8 ShaderProgram::tick(const U32 deltaTime){
	ParamHandler& par = ParamHandler::getInstance();
    this->Uniform("enableFog",par.getParam<bool>("rendering.enableFog"));
	this->Uniform("dvd_lightAmbient", LightManager::getInstance().getAmbientLight());

	if(_dirty){
		//Apply global shader values valid throughout application runtime:
		std::string shadowMaps("texDepthMapFromLight");
		std::stringstream ss;
		for(I32 i = 0; i < MAX_SHADOW_CASTING_LIGHTS_PER_NODE; i++){
			ss.str(std::string());
			ss << shadowMaps << i;
			this->UniformTexture(ss.str(),10+i); //Shadow Maps always bound from 8 and above
		}

		this->Uniform("zPlanes",  Frustum::getInstance().getZPlanes());
		this->Uniform("screenDimension", Application::getInstance().getResolution());
		this->UniformTexture("texDepthMapFromLightArray",_maxCombinedTextureUnits - 2); //Reserve first for directional shadows
		this->UniformTexture("texDepthMapFromLightCube", _maxCombinedTextureUnits - 1); //Reserve second for point shadows
		this->UniformTexture("texSpecular",Material::SPECULAR_TEXTURE_UNIT);
		this->UniformTexture("opacityMap",Material::OPACITY_TEXTURE_UNIT);
		this->UniformTexture("texBump",Material::BUMP_TEXTURE_UNIT);
		this->UniformTexture("texDiffuse1",Material::SECOND_TEXTURE_UNIT);
		this->UniformTexture("texDiffuse0", Material::FIRST_TEXTURE_UNIT);

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

void ShaderProgram::setMatrixMask(const bool uploadNormals,
                                  const bool uploadModel,
                                  const bool uploadModelView,
                                  const bool uploadModelViewProjection){

    _matrixMask.b.b0 = uploadNormals ? 1 : 0;
    _matrixMask.b.b1 = uploadModel ? 1 : 0;
    _matrixMask.b.b2 = uploadModelView ? 1 : 0;
    _matrixMask.b.b3 = uploadModelViewProjection ? 1 : 0;
}

void ShaderProgram::threadedLoad(const std::string& name){
	if(ShaderProgram::generateHWResource(name)) HardwareResource::threadedLoad(name);
	tick(0);
}

bool ShaderProgram::generateHWResource(const std::string& name){
	_name = name;
	return HardwareResource::generateHWResource(name);
}

void ShaderProgram::bind(){
	_bound = true;
	_wasBound = true;
	if(_shaderProgramId == 0) return;

	//Apply global shader values valid throughout current render call:
    this->Uniform("time", static_cast<F32>(GETMSTIME()));
    this->Attribute("cameraPosition",Frustum::getInstance().getEyePos());
}

void ShaderProgram::uploadModelMatrices(){
	GFXDevice& GFX = GFX_DEVICE;
	/*Get matrix data*/
    if(_matrixMask.b.b0){
        mat3<F32> norMat;
        GFX.getMatrix(NORMAL_MATRIX,norMat);
        this->Uniform("dvd_NormalMatrix",norMat);
    }
    if(_matrixMask.b.b1){
        mat4<F32> modMat;
        GFX.getMatrix(MODEL_MATRIX,modMat);
        this->Uniform("dvd_ModelMatrix",modMat);
    }
    if(_matrixMask.b.b2){
        mat4<F32> mvMat;
        GFX.getMatrix(MV_MATRIX,mvMat);
        this->Uniform("dvd_ModelViewMatrix",mvMat);
    }
    if(_matrixMask.b.b3){
        mat4<F32> mvpMat;
        GFX.getMatrix(MVP_MATRIX,mvpMat);
        this->Uniform("dvd_ModelViewProjectionMatrix",mvpMat);
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

void ShaderProgram::recompile(const bool vertex,
                              const bool fragment,
                              const bool geometry,
                              const bool tessellation)
{
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
    if(_wasBound) bind();
}