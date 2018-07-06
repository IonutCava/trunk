#include "Headers/glShader.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include <boost/regex.hpp>
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"

glShader::glShader(const std::string& name, ShaderType type) : Shader(name, type){
  switch (type) {
    case VERTEX_SHADER : { 
		_shader = glCreateShader(GL_VERTEX_SHADER);
		break;
	}
    case FRAGMENT_SHADER : {
		_shader = glCreateShader(GL_FRAGMENT_SHADER);
		break;
	}

    case GEOMETRY_SHADER : {
		_shader = glCreateShader(GL_GEOMETRY_SHADER);
		break;
	}
	case TESSELATION_SHADER : {
		_shader = NULL; 
		ERROR_FN(Locale::get("WARN_GLSL_NO_TESSELATION")); 
		break;
	}
	default:
		ERROR_FN(Locale::get("ERROR_GLSL_UNKNOWN_ShaderType"),type);
		break;
  }
}

glShader::~glShader(){
	GLCheck(glDeleteShader(_shader));
}

bool glShader::load(const std::string& source){
	if(source.empty()){
		ERROR_FN(Locale::get("ERROR_GLSL_SHADER_NOT_FOUND"),getName().c_str());
		return false;
	}
	std::string parsedSource = preprocessIncludes(source,getName(),0);
	ShaderManager::getInstance().shaderFileWrite((char*)(std::string("shaderCache/"+getName()).c_str()),(char*)parsedSource.c_str());
	const char* c_str = parsedSource.c_str();
	GLCheck(glShaderSource(_shader, 1, &c_str, 0));
	GLCheck(glCompileShader(_shader));
	_compiled = true;
	validate();
	return true;
}

void glShader::validate() {
	const GLushort BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	GLint length = 0;
    GLint status = 0;
	GLCheck(glGetShaderInfoLog(_shader, BUFFER_SIZE, &length, buffer));
	GLCheck(glGetShaderiv(_shader, GL_COMPILE_STATUS, &status));
	if(status == GL_FALSE){
		ERROR_FN(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),buffer);
	}else{
		D_PRINT_FN(Locale::get("GLSL_VALIDATING_SHADER"), _name.c_str(),buffer);
	}
}

std::string glShader::preprocessIncludes( const std::string& source, const std::string& filename, int level /*= 0 */ ){

	if(level > 32){
		ERROR_FN(Locale::get("ERROR_GLSL_INCLUD_LIMIT"));
	}

	static const boost::regex re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
	std::stringstream input;
	std::stringstream output;
	input << source;

	size_t line_number = 1;
	boost::smatch matches;

	std::string line;
	while(std::getline(input,line))	{

		if (boost::regex_search(line, matches, re))	{

			std::string include_file = matches[1];
			std::string include_string;
			std::string loc;
			if(include_file.find("frag") != std::string::npos){
				loc =  "fragmentAtoms";
			}else if(include_file.find("vert") != std::string::npos){
				loc =  "vertexAtoms";
			}else if(include_file.find("geom") != std::string::npos){
				loc =  "geometryAtoms";
			}else if(include_file.find("tess") != std::string::npos){
				loc =  "tessellationAtoms";
			}
			ParamHandler& par = ParamHandler::getInstance();
			include_string = ShaderManager::getInstance().shaderFileRead(include_file,par.getParam<std::string>("assetsLocation") + "/" + par.getParam<std::string>("shaderLocation")+"/GLSL/"+loc);
			if(include_string.empty()){
				ERROR_FN(Locale::get("ERROR_GLSL_NO_INCLUDE_FILE"),getName().c_str(), line_number, include_file.c_str());
			}
			output << preprocessIncludes(include_string, include_file, level + 1) << std::endl;
		}else{
			output <<  line << std::endl;
		}
		++line_number;
	}
	return output.str();
}