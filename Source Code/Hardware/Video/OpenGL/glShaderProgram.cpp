#include "glResources.h"
#include "glsw/glsw.h"

#include "resource.h"
#include "glShaderProgram.h"
#include "Hardware/Video/Shader.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"
#include <boost/algorithm/string.hpp>

using namespace std;
glShaderProgram::glShaderProgram() : ShaderProgram(),
								     _loaded(false){
		_shaderProgramId = glCreateProgram();
}

glShaderProgram::~glShaderProgram() {
	for_each(Shader* s, _shaders){
		glDetachShader(_shaderProgramId, s->getShaderId());
	}
  	glDeleteProgram(_shaderProgramId);
}

void glShaderProgram::validate() {
	const U16 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	I32 length = 0;
    
	memset(buffer, 0, BUFFER_SIZE);

	glValidateProgram(_shaderProgramId);
	glGetProgramInfoLog(_shaderProgramId, BUFFER_SIZE, &length, buffer);
	I32 status = 0;
	glGetProgramiv(_shaderProgramId, GL_VALIDATE_STATUS, &status);
	// ToDo: Problem with AMD(ATI) cards. GLSL validation errors about multiple samplers to same uniform, but they still work. Fix that. -Ionut
	if (status == GL_FALSE){
		Console::getInstance().errorfn("[GLSL Manager] Validating program [ %s ]: %s", getName().c_str(), buffer);
	}else{
		Console::getInstance().d_printfn("[GLSL Manager] Validating program [ %s ]: %s", getName().c_str(),buffer);
	}
}

void glShaderProgram::attachShader(Shader *shader){
   _shaders.push_back(shader);
   glAttachShader(_shaderProgramId,shader->getShaderId());
   _compiled = false;
}

bool glShaderProgram::load(const string& name){
	_name = name;
	_compiled = false;
	if(name.compare("NULL") == 0){
		glDeleteProgram(_shaderProgramId);
		_shaderProgramId = 0;
		_loaded = true;
		return _loaded;
	}
	if(!_loaded){
		glswInit();
	    glswSetPath(getResourceLocation().c_str(), ".glsl");
		std::vector<std::string> strs;
		boost::split(strs, name, boost::is_any_of(","));
		if(strs.size() == 1){
			Shader* s = ShaderManager::getInstance().loadShader(name+".frag", getResourceLocation());
			if(s){
				attachShader(s);
			}
			s = ShaderManager::getInstance().loadShader(name+".vert", getResourceLocation());
			if(s){
				attachShader(s);
			}
			s = ShaderManager::getInstance().loadShader(name+".geom", getResourceLocation());
			if(s){
				attachShader(s);
			}
		}else{
			for_each(std::string shader, strs){
				Shader* s = ShaderManager::getInstance().loadShader(shader, getResourceLocation());
				if(s){
					attachShader(s);
				}
			}
		}
		
		commit();
		_loaded = true;
	}
	glswShutdown();
	if(_loaded){
		return ShaderProgram::load(name);
	}
	return false;
		
	 _loaded;
}

bool glShaderProgram::flushLocCache(){
	_shaderVars.clear();
	return true;
}

I32 glShaderProgram::cachedLoc(const std::string& name,bool uniform){
	if(_shaderVars.find(name) != _shaderVars.end()){
		return _shaderVars[name];
	}
	I32 location = 0;
	uniform ? location = glGetUniformLocation(_shaderProgramId, name.c_str()) : location = glGetAttribLocation(_shaderProgramId, name.c_str());

	_shaderVars.insert(std::make_pair(name,location));
	return location;
}

void glShaderProgram::link(){
	Console::getInstance().d_printfn("Linking shader [ %s ]",getName().c_str());
	glLinkProgram(_shaderProgramId);
	_compiled = true;
}

void glShaderProgram::bind() {
	if(checkBinding(_shaderProgramId)){ //prevent double bind
		glUseProgram(_shaderProgramId);
	}
	ShaderProgram::bind(); //send default uniforms to GPU;
}

void glShaderProgram::unbind() {
	glUseProgram(0);
	_prevShaderProgramId = 0;
}

void glShaderProgram::Attribute(const std::string& ext, D32 value){
	glVertexAttrib1d(cachedLoc(ext,false),value);
}

void glShaderProgram::Attribute(const std::string& ext, F32 value){
	glVertexAttrib1f(cachedLoc(ext,false),value);
}

void glShaderProgram::Attribute(const std::string& ext, const vec2& value){
	glVertexAttrib2fv(cachedLoc(ext,false),value);
}

void glShaderProgram::Attribute(const std::string& ext, const vec3& value){
	glVertexAttrib3fv(cachedLoc(ext,false),value);
}

void glShaderProgram::Attribute(const std::string& ext, const vec4& value){
	glVertexAttrib4fv(cachedLoc(ext,false),value);
}


void glShaderProgram::Uniform(const string& ext, I32 value){
	glUniform1i(cachedLoc(ext), value);
}

void glShaderProgram::Uniform(const string& ext, F32 value){
	glUniform1f(cachedLoc(ext), value);
}

void glShaderProgram::Uniform(const string& ext, const vec2& value){
	glUniform2fv(cachedLoc(ext), 1, value);
}

void glShaderProgram::Uniform(const string& ext, const vec3& value){
	glUniform3fv(cachedLoc(ext), 1, value);
}

void glShaderProgram::Uniform(const string& ext, const vec4& value){
	glUniform4fv(cachedLoc(ext), 1, value);
}

void glShaderProgram::Uniform(const std::string& ext, const mat3& value){

	glUniformMatrix3fv(cachedLoc(ext), 1,false, value);
}

void glShaderProgram::Uniform(const std::string& ext, const mat4& value){
	glUniformMatrix4fv(cachedLoc(ext), 1,false, value);
}

void glShaderProgram::Uniform(const std::string& ext, const vector<mat4>& values){
	glUniformMatrix4fv(cachedLoc(ext),values.size(),true, values[0]);
}

void glShaderProgram::UniformTexture(const string& ext, U16 slot){
	glActiveTexture(GL_TEXTURE0+slot);
	glUniform1i(cachedLoc(ext), slot);
}