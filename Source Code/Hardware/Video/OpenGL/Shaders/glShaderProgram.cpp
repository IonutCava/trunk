#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"

#include "core.h"
#include "Headers/glShaderProgram.h"
#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"
#include <boost/algorithm/string.hpp>

glShaderProgram::glShaderProgram() : ShaderProgram(), _shaderProgramIdInternal(std::numeric_limits<U32>::max()),_validationQueued(false) {}

glShaderProgram::~glShaderProgram() {
	if(_shaderProgramIdInternal > 0){
		for_each(Shader* s, _shaders){
			GLCheck(glDetachShader(_shaderProgramIdInternal, s->getShaderId()));
		}
  		GLCheck(glDeleteProgram(_shaderProgramIdInternal));
	}
}

void glShaderProgram::validate() {
	_validationQueued = true;
}

void glShaderProgram::validateInternal(){
	const GLushort BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	GLint length = 0;
    
	memset(buffer, 0, BUFFER_SIZE);

	GLCheck(glValidateProgram(_shaderProgramIdInternal));
	GLCheck(glGetProgramInfoLog(_shaderProgramIdInternal, BUFFER_SIZE, &length, buffer));
	GLint status = 0;
	GLCheck(glGetProgramiv(_shaderProgramIdInternal, GL_VALIDATE_STATUS, &status));
#pragma message("ToDo: Problem with AMD(ATI) cards. GLSL validation errors about multiple samplers to same uniform, but they still work. Fix that. -Ionut")
	if (status == GL_FALSE){
		ERROR_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(), buffer);
	}else{
		D_PRINT_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(),buffer);
	}
	_validationQueued = false;
}

void glShaderProgram::attachShader(Shader *shader){
   _shaders.push_back(shader);
   GLCheck(glAttachShader(_shaderProgramIdInternal,shader->getShaderId()));
   _compiled = false;
}

/// Creation of a new shader program.
/// Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::generateHWResource(const std::string& name){
	///Set the name and compilation state
	_name = name;
	_compiled = false;

	///NULL shader means use shaderProgram(0)
	if(name.compare("NULL_SHADER") == 0){
		_shaderProgramIdInternal = 0;
		_shaderProgramId = 0;
		_validationQueued = false;
		return true;
	}

	GLint lineCountOffset = 0;

	///Create a new program
	GLCheck(_shaderProgramIdInternal = glCreateProgram());
	_shaderProgramId = _shaderProgramIdInternal;
	///Init glsw library
	glswInit();
	///Use the specified shader path
	glswSetPath(std::string(getResourceLocation()+"GLSL/").c_str(), ".glsl");
	glswAddDirectiveToken("", "#version 130\n/*“Copyright 2009-2013 DIVIDE-Studio”*/");
	lineCountOffset += 2;
	std::string lightCount("#define MAX_LIGHT_COUNT ");
	lightCount += Util::toString(MAX_LIGHTS_PER_SCENE_NODE);
	glswAddDirectiveToken("", lightCount.c_str());
	lineCountOffset++;
	std::string shadowCount("#define MAX_SHADOW_CASTING_LIGHTS ");
	shadowCount += Util::toString(MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
	glswAddDirectiveToken("", shadowCount.c_str());
	lineCountOffset++;
	glswAddDirectiveToken("","#extension GL_EXT_texture_array : enable");
	lineCountOffset++;
	//glswAddDirectiveToken("","#define Z_TEST_SIGMA 0.0001");
	//lineCountOffset++;
	if(GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA || 
	   GFX_DEVICE.getGPUVendor() == GPU_VENDOR_AMD){
		glswAddDirectiveToken("","#extension GL_EXT_gpu_shader4 : enable");
		lineCountOffset++;
	}
	if(!_definesList.empty()){
		///get all of the preprocessor defines
		vectorImpl<std::string> defines;
		boost::split(defines, _definesList, boost::is_any_of(","), boost::token_compress_on);
		for(U8 i = 0; i < defines.size(); i++){
			std::string define("#define ");
			define += defines[i];
			glswAddDirectiveToken("",define.c_str());
			lineCountOffset++;
			if(define.find("USE_VBO_DATA") != std::string::npos){
				glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_POSITION_LOCATION ,"inVertexData");
				glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_NORMAL_LOCATION   ,"inNormalData");
				glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_TEXCOORD_LOCATION ,"inTexCoordData");
				glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_TANGENT_LOCATION  ,"inTangentData");
				glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_BITANGENT_LOCATION,"inBiTangentData");
			}
		}
	}
	///Split the shader name to get the effect file name and the effect properties 
	std::string shaderName = name.substr(0,name.find_first_of(".,"));
	std::string shaderProperties;
	///Vertex Properties work in revers order. All the text after "," goes first.
	///The rest of the shader properties are added later
	size_t propPositionVertex = name.find_first_of(",");
	size_t propPosition = name.find_first_of(".");
	if(propPosition != std::string::npos){
		shaderProperties = "."+ name.substr(propPosition+1,propPositionVertex-propPosition-1);
	}
	std::string vertexProperties;
	if(propPositionVertex != std::string::npos){
		vertexProperties = "."+name.substr(propPositionVertex+1);
	}
	
	vertexProperties += shaderProperties;

	if(vertexProperties.find("WithBones") != std::string::npos){
		glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_BONE_WEIGHT_LOCATION,"inBoneWeightData");
		glBindAttribLocation(_shaderProgramIdInternal,Divide::GL::VERTEX_BONE_INDICE_LOCATION,"inBoneIndiceData");
	}

	std::string vertexShader = shaderName+".Vertex"+vertexProperties;
	std::string fragmentShader = shaderName+".Fragment"+shaderProperties;
	std::string geometryShader = shaderName+".Geometry"+shaderProperties;
	std::string tessellationShader = shaderName+".Tessellation"+shaderProperties;

	///Load the Vertex Shader
	///See if it already exists in a compiled state
	Shader* s = ShaderManager::getInstance().findShader(vertexShader);
	///If it doesn't
	if(!s){
		///Use glsw to read the vertex part of the effect
		const char* vs = glswGetShader(vertexShader.c_str(),lineCountOffset);
		///If reading was succesfull
		if(vs != NULL){
			//Load our shader and save it in the manager in case a new Shader Program needs it
			s = ShaderManager::getInstance().loadShader(vertexShader,vs,VERTEX_SHADER);
		}else{
			ERROR_FN("[GLSL]  %s",glswGetError());
		}
	}
	///If the vertex shader loaded ok
	if(s){
		///attach it to the program
		attachShader(s);
		///same goes for every other shader type
	}else{
		ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),vertexShader.c_str());
	}
	///Load the Fragment Shader
	s = ShaderManager::getInstance().findShader(fragmentShader);
	if(!s){
		const char* fs = glswGetShader(fragmentShader.c_str(),lineCountOffset);
		if(fs != NULL){
			s = ShaderManager::getInstance().loadShader(fragmentShader,fs,FRAGMENT_SHADER);
		}else{
			ERROR_FN("[GLSL] %s",glswGetError());
		}
	}
	if(s){
		attachShader(s);
	}else{
		ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),fragmentShader.c_str());
	}
	///Load the Geometry Shader
	s = ShaderManager::getInstance().findShader(geometryShader);
	if(!s){
		const char* gs = glswGetShader(geometryShader.c_str(),lineCountOffset);
		if(gs != NULL){
			s = ShaderManager::getInstance().loadShader(geometryShader,gs,GEOMETRY_SHADER);
		}else{
			///Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
			D_ERROR_FN("[GLSL] %s", glswGetError());
		}
	}
	if(s){
		attachShader(s);
	}else{
		D_ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),geometryShader.c_str());
	}

	///Load the Tessellation Shader
	s = ShaderManager::getInstance().findShader(tessellationShader);
	if(!s){
		const char* ts = glswGetShader(tessellationShader.c_str(),lineCountOffset);
		if(ts != NULL){
			s = ShaderManager::getInstance().loadShader(tessellationShader,ts,TESSELATION_SHADER);
		}else{
			///Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
			D_ERROR_FN("[GLSL] %s", glswGetError());
		}
	}
	if(s){
		attachShader(s);
	}else{
		D_ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),tessellationShader.c_str());
	}
	///Link and validate shaders into this program and update state
	commit();
	///Shut down glsw and clean memory
	glswShutdown();

	return ShaderProgram::generateHWResource(name);
}

bool glShaderProgram::flushLocCache(){
	_shaderVars.clear();
	return true;
}

GLint glShaderProgram::getAttributeLocation(const std::string& name){
	return cachedLoc(name,false);
}

GLint glShaderProgram::getUniformLocation(const std::string& name){
	return cachedLoc(name,true);
}

///Cache uniform/attribute locations for shaderprograms
///When we call this function, we check our name<->address map to see if we queried the location before
///If we did not, ask the GPU to give us the variables address and save it for later use
GLint glShaderProgram::cachedLoc(const std::string& name,bool uniform){
	if(_shaderProgramIdInternal == 0) return -1; //<NULL_SHADER
	const char* locationName = name.c_str();
	if(!_bound)
		ERROR_FN(Locale::get("ERROR_GLSL_CHANGE_UNBOUND"), name.c_str());
	if(_shaderVars.find(name) != _shaderVars.end()){
		return _shaderVars[name];
	}
	if(_validationQueued) validateInternal();
	GLint location = -1;
	if(uniform){
		GLCheck(location = glGetUniformLocation(_shaderProgramIdInternal, locationName)); 
	}else {
		GLCheck(location = glGetAttribLocation(_shaderProgramIdInternal, locationName));
	}
	_shaderVars.insert(std::make_pair(name,location));
	return location;
}

void glShaderProgram::link(){
	D_PRINT_FN(Locale::get("GLSL_LINK_PROGRAM"),getName().c_str());
	GLCheck(glLinkProgram(_shaderProgramIdInternal));
	_compiled = true;
}

bool glShaderProgram::checkBinding(){
    if(GL_API::getActiveShaderId() != _shaderProgramIdInternal){
        GL_API::setActiveShaderId(_shaderProgramIdInternal);
		return true;
	}
	return false;
}

void glShaderProgram::bind() {
	///If we did not create the hardware resource, do not try and bind it, as it is invalid
	if(_shaderProgramIdInternal == std::numeric_limits<U32>::max()) return; 
	///prevent double bind
	if(checkBinding()){
		GLCheck(glUseProgram(_shaderProgramIdInternal));
		///send default uniforms to GPU;
        if(_shaderProgramIdInternal != 0){
            ShaderProgram::bind();
        }
	}
}

void glShaderProgram::unbind() {
	if(_bound){
		GLCheck(glUseProgram(0));
        GL_API::setActiveShaderId(0);
		ShaderProgram::unbind();
	}
}

void glShaderProgram::Attribute(const std::string& ext, GLdouble value){
	GLint loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib1d(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, GLfloat value){
	GLint loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib1f(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec2<GLfloat>& value){
	GLint loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib2fv(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec3<GLfloat>& value){
	GLint loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib3fv(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec4<GLfloat>& value){
	GLint loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib4fv(loc,value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, GLuint value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1ui(loc, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, GLint value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1i(loc, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, GLfloat value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1f(loc, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLfloat>& value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform2fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLint>& value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform2iv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLushort>& value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform2iv(loc, 1, vec2<I32>(value.x, value.y)));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vec3<GLfloat>& value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform3fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vec4<GLfloat>& value){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform4fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const mat3<GLfloat>& value, bool rowMajor){
GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix3fv(loc, 1,rowMajor, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const mat4<GLfloat>& value, bool rowMajor){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix4fv(loc, 1, rowMajor, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<GLint >& values){
	if(values.empty()) return;
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1iv(loc,values.size(),&values.front()));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<GLfloat >& values){
	if(values.empty()) return;
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1fv(loc,values.size(),&values.front()));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<mat4<GLfloat> >& values, bool rowMajor){
	if(values.empty()) return;
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix4fv(loc,values.size(), rowMajor,values.front()));
	}
}

void glShaderProgram::UniformTexture(const std::string& ext, GLushort slot){
	GLint loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1i(loc, slot));
	}
}