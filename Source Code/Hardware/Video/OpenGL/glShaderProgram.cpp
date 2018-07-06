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
		ERROR_FN("[GLSL Manager] Validating program [ %s ]: %s", getName().c_str(), buffer);
	}else{
		D_PRINT_FN("[GLSL Manager] Validating program [ %s ]: %s", getName().c_str(),buffer);
	}
}

void glShaderProgram::attachShader(Shader *shader){
   _shaders.push_back(shader);
   glAttachShader(_shaderProgramId,shader->getShaderId());
   _compiled = false;
}

/// Creation of a new shader program.
/// Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::load(const string& name){
	///Set the name and compilation state
	_name = name;
	_compiled = false;
	///NULL shader means use shaderProgram(0)
	if(name.compare("NULL") == 0){
		glDeleteProgram(_shaderProgramId);
		_shaderProgramId = 0;
		_loaded = true;
		return _loaded;
	}
	///If this program wasn't previously loaded (unload first to load again)
	if(!_loaded){
		///Init glsw library
		glswInit();
		///Use the specified shader path
	    glswSetPath(string(getResourceLocation()+"GLSL/").c_str(), ".glsl");
		glswAddDirectiveToken("", "#version 120\n/*�Copyright 2009-2012 DIVIDE-Studio�*/");
		///Split the shader name to get the effect file name and the effect properties 
		std::string shaderName = name.substr(0,name.find_first_of("."));
		std::string shaderProperties;
		size_t propPosition = name.find_first_of(".");
		if(propPosition !=string::npos){
			shaderProperties = "."+name.substr(propPosition+1);
		}
		
		std::string vertexShader = shaderName+".Vertex"+shaderProperties;
		std::string fragmentShader = shaderName+".Fragment"+shaderProperties;
		std::string geometryShader = shaderName+".Geometry"+shaderProperties;
		std::string tessellationShader = shaderName+".Tessellation"+shaderProperties;

		///Load the Vertex Shader
		///See if it already exists in a compiled state
		Shader* s = ShaderManager::getInstance().findShader(vertexShader);
		///If it doesn't
		if(!s){
			///Use glsw to read the vertex part of the effect
			const char* vs = glswGetShader(vertexShader.c_str());
			///If reading was succesfull
			if(vs != NULL){
				//Load our shader and save it in the manager in case a new Shader Program needs it
				s = ShaderManager::getInstance().loadShader(vertexShader,vs,VERTEX_SHADER);
			}else{
				ERROR_FN("ShaderProgram: %s",glswGetError());
			}
		}
		///If the vertex shader loaded ok
		if(s){
			///attach it to the program
			attachShader(s);
			///same goes for every other shader type
		}else{
			ERROR_FN("Could not load shader [ %s ]",vertexShader.c_str());
		}
		///Load the Fragment Shader
		s = ShaderManager::getInstance().findShader(fragmentShader);
		if(!s){
			const char* fs = glswGetShader(fragmentShader.c_str());
			if(fs != NULL){
				s = ShaderManager::getInstance().loadShader(fragmentShader,fs,FRAGMENT_SHADER);
			}else{
				ERROR_FN("ShaderProgram: %s",glswGetError());
			}
		}
		if(s){
			attachShader(s);
		}else{
			ERROR_FN("Could not load shader [ %s ]",fragmentShader.c_str());
		}
		///Load the Geometry Shader
		s = ShaderManager::getInstance().findShader(geometryShader);
		if(!s){
			const char* gs = glswGetShader(geometryShader.c_str());
			if(gs != NULL){
				s = ShaderManager::getInstance().loadShader(geometryShader,gs,GEOMETRY_SHADER);
			}else{
				///Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
				D_ERROR_FN("ShaderProgram: %s", glswGetError());
			}
		}
		if(s){
			attachShader(s);
		}else{
			D_ERROR_FN("Could not load shader [ %s ]",geometryShader.c_str());
		}

		///Load the Tessellation Shader
		s = ShaderManager::getInstance().findShader(tessellationShader);
		if(!s){
			const char* ts = glswGetShader(tessellationShader.c_str());
			if(ts != NULL){
				s = ShaderManager::getInstance().loadShader(tessellationShader,ts,TESSELATION_SHADER);
			}else{
				///Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
				D_ERROR_FN("ShaderProgram: %s", glswGetError());
			}
		}
		if(s){
			attachShader(s);
		}else{
			D_ERROR_FN("Could not load shader [ %s ]",tessellationShader.c_str());
		}

		///Link and validate shaders into this program and update state
		commit();
		///Shut down glsw and clean memory
		glswShutdown();
		_loaded = true;
	}
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

I32 glShaderProgram::getAttributeLocation(const std::string& name){
	return cachedLoc(name,false);
}

I32 glShaderProgram::getUniformLocation(const std::string& name){
	return cachedLoc(name,true);
}

///Cache uniform/attribute locations for shaderprograms
///When we call this function, we check our name<->address map to see if we queried the location before
///If we did not, ask the GPU to give us the variables address and save it for later use
I32 glShaderProgram::cachedLoc(const std::string& name,bool uniform){
	const char* locationName = name.c_str();
	if(!_bound) ERROR_FN("glShaderProgram: attempted to change shader param [ %s ] in unbound shader", name.c_str());
	if(_shaderVars.find(name) != _shaderVars.end()){
		return _shaderVars[name];
	}
	I32 location = -1;
	if(uniform){
		GLCheck(location = glGetUniformLocation(_shaderProgramId, locationName)); 
	}else {
		GLCheck(location = glGetAttribLocation(_shaderProgramId, locationName));
	}
	_shaderVars.insert(std::make_pair(name,location));
	return location;
}

void glShaderProgram::link(){
	D_PRINT_FN("Linking shader [ %s ]",getName().c_str());
	GLCheck(glLinkProgram(_shaderProgramId));
	_compiled = true;
}

void glShaderProgram::bind() {
	if(checkBinding(_shaderProgramId)){ //prevent double bind
		GLCheck(glUseProgram(_shaderProgramId));
		ShaderProgram::bind(); //send default uniforms to GPU;
	}
}

void glShaderProgram::unbind() {
	if(_bound){
		glUseProgram(0);
		ShaderProgram::unbind();
	}
}

void glShaderProgram::Attribute(const std::string& ext, D32 value){
	I32 loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib1d(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, F32 value){
	I32 loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib1f(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec2<F32>& value){
	I32 loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib2fv(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec3<F32>& value){
	I32 loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib3fv(loc,value));
	}
}

void glShaderProgram::Attribute(const std::string& ext, const vec4<F32>& value){
	I32 loc = cachedLoc(ext,false);
	if(loc != -1){
		GLCheck(glVertexAttrib4fv(loc,value));
	}
}


void glShaderProgram::Uniform(const string& ext, I32 value){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1i(loc, value));
	}
}

void glShaderProgram::Uniform(const string& ext, F32 value){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform1f(loc, value));
	}
}

void glShaderProgram::Uniform(const string& ext, const vec2<F32>& value){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform2fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const string& ext, const vec3<F32>& value){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform3fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const string& ext, const vec4<F32>& value){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniform4fv(loc, 1, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor){
I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix3fv(loc, 1,rowMajor, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix4fv(loc, 1, rowMajor, value));
	}
}

void glShaderProgram::Uniform(const std::string& ext, const vector<mat4<F32> >& values, bool rowMajor){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		GLCheck(glUniformMatrix4fv(loc,values.size(), rowMajor,values[0]));
	}
}

void glShaderProgram::UniformTexture(const std::string& ext, U16 slot){
	I32 loc = cachedLoc(ext);
	if(loc != -1){
		glActiveTexture(GL_TEXTURE0+slot);
		glUniform1i(loc, slot);
	}
}