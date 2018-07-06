#include "glShader.h"
#include "glResources.h"
#include <boost/regex.hpp>
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"

glShader::glShader(const std::string& name, SHADER_TYPE type) : Shader(name, type){
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
		Console::getInstance().errorfn("GLSL: Tesselation not yet implemented"); 
		break;
	}
	default:
		Console::getInstance().errorfn("GLSL: Unknown shader type received: %d",type);
		break;
  }
}

glShader::~glShader(){
	glDeleteShader(_shader);
}

bool glShader::load(const std::string& source){
	if(source.empty()){
		Console::getInstance().errorfn("GLSL Manager: Shader [ %s ] not found!",getName().c_str());
		return false;
	}
	std::string parsedSource = preprocessIncludes(source,getName(),0);
	ShaderManager::getInstance().shaderFileWrite((char*)(std::string("shaderCache/"+getName()).c_str()),(char*)parsedSource.c_str());
	const char* c_str = parsedSource.c_str();
	glShaderSource(_shader, 1, &c_str, 0);
	glCompileShader(_shader);
	_compiled = true;
	validate();
	return true;
}

void glShader::validate() {
	const U16 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	I32 length = 0;
    I32 status = 0;
	glGetShaderInfoLog(_shader, BUFFER_SIZE, &length, buffer);
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE){
		Console::getInstance().errorfn("[GLSL Manager] Validating shader [ %s ]: %s", _name.c_str(),buffer);
	}else{
		Console::getInstance().d_printfn("[GLSL Manager] Validating shader [ %s ]: %s", _name.c_str(),buffer);
	}
}

std::string glShader::preprocessIncludes( const std::string& source, const std::string& filename, int level /*= 0 */ ){

	if(level > 32){
		Console::getInstance().errorfn("glShader: Header inclusion depth limit reached, might be caused by cyclic header inclusion");
	}

	using namespace std;

	static const boost::regex re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
	stringstream input;
	stringstream output;
	input << source;

	size_t line_number = 1;
	boost::smatch matches;

	string line;
	while(std::getline(input,line))	{

		if (boost::regex_search(line, matches, re))	{

			std::string include_file = matches[1];
			std::string include_string;
			std::string loc;
			if(include_file.find("frag") != string::npos){
				loc =  "fragmentAtoms";
			}else if(include_file.find("vert") != string::npos){
				loc =  "vertexAtoms";
			}else if(include_file.find("geom") != string::npos){
				loc =  "geometryAtoms";
			}else if(include_file.find("tess") != string::npos){
				loc =  "tessellationAtoms";
			}
			ParamHandler& par = ParamHandler::getInstance();
			include_string = ShaderManager::getInstance().shaderFileRead(include_file,par.getParam<string>("assetsLocation") + "/" + par.getParam<string>("shaderLocation")+"/GLSL/"+loc);
			if(include_string.empty()){
				stringstream str;
				str <<  getName() <<"(" << line_number << ") : fatal error: cannot open include file " << include_file;
				Console::getInstance().errorfn("glShader: %s",str.str());
			}
			output << preprocessIncludes(include_string, include_file, level + 1) << endl;
		}
		else
		{
			//output << "#line "<< line_number << " \"" << filename << "\""  << endl;
			output <<  line << endl;
		}
		++line_number;
	}
	return output.str();
}