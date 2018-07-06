#include "glResources.h"
#include "resource.h"
#include "glShader.h"
#include "Utility/Headers/ParamHandler.h"
#include "Rendering/common.h"

void glShader::validateShader(U32 shader, const string &file) {
	const U32 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	int length = 0;
    
	glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
	cout << "GLSL Manager: Validating shader: " << buffer << endl;
}

void glShader::validateProgram(U32 program) {
	const U32 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	int length = 0;
    
	memset(buffer, 0, BUFFER_SIZE);

	glValidateProgram(program);
	glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
	cout << "GLSL Manager: Validating program: " << buffer << endl;
	int status;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	if (status == GL_FALSE)
		cout << "GLSL Manager: Error validating shader " << program << endl;
}



char* glShader::shaderFileRead(const std::string &fn) {

	std::string file = ParamHandler::getInstance().getParam<string>("assetsLocation") + "/shaders/" + fn;
	FILE *fp = NULL;
	fopen_s(&fp,file.c_str(),"r");

	char *content = NULL;

	int count=0;

	if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = New char[count+1];
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
	}
	return content;
}

int glShader::shaderFileWrite(char *fn, char *s) {

	FILE *fp = NULL;
	int status = 0;

	if (fn != NULL) {
		fopen_s(&fp,fn,"w");

		if (fp != NULL) {
			
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}

glShader::glShader(const char *vsFile, const char *fsFile)  : _loaded(false)  {
    init(vsFile, fsFile);
}

bool glShader::loadFragOnly(const std::string& name)
{
	init("none",name.c_str());
	return true;
}

bool glShader::loadVertOnly(const std::string& name)
{
	init(name.c_str(),"none");
	return true;
}

bool glShader::load(const std::string& name)
{
	if(!_loaded)
	{
		_name = name;
		if(name.length() >= 6)
		{
			string extension(name.substr(name.length()- 5, name.length()));
			if(extension.compare(".frag") == 0) loadFragOnly(name);
			else if(extension.compare(".vert") == 0) loadVertOnly(name);
			else init(name+".vert",name+".frag");
		}
		else
			init(name+".vert",name+".frag");

		_loaded = true;
	}
	return _loaded;
}

void glShader::init(const std::string &vsFile, const std::string &fsFile) {
	bool useVert = false, useFrag = false;
	if(vsFile.compare("none") != 0) useVert = true;
	if(fsFile.compare("none") != 0) useFrag = true;

    if(useVert && useFrag) 
		cout << "GLSL Manager: Loading shaders " << vsFile << " and " << fsFile << endl;
	if(useVert && !useFrag)
		cout << "GLSL Manager: Loading shaders " << vsFile  << endl;
	if(!useVert && useFrag)
		cout << "GLSL Manager: Loading shaders " << fsFile  << endl;

	_shaderId = glCreateProgram();
	if(useVert)
	{
		_shaderVP = glCreateShader(GL_VERTEX_SHADER);
		const char* vsText = shaderFileRead(vsFile);
		if(vsText == NULL)
		{
			cout << "GLSL Manager: Vertex shader [" << vsFile << "] not found!" << endl;
			return;
		}
		glShaderSource(_shaderVP, 1, &vsText, 0);
		glCompileShader(_shaderVP);
		validateShader(_shaderVP, vsFile);
		glAttachShader(_shaderId, _shaderVP);
		delete vsText;
		vsText = NULL;
	}
	if(useFrag)
	{
		_shaderFP = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fsText = shaderFileRead(fsFile);
		if(fsText == NULL)
		{
			cout << "GLSL Manager: Fragment shader [" << fsFile << "] not found!" << endl;
			return;
		}
		glShaderSource(_shaderFP, 1, &fsText, 0);
		glCompileShader(_shaderFP);
		validateShader(_shaderFP, fsFile);
		glAttachShader(_shaderId, _shaderFP);
		delete fsText;
		fsText = NULL;
	}

	glLinkProgram(_shaderId);
	validateProgram(_shaderId);
}

glShader::~glShader() {
	glDetachShader(_shaderId, _shaderFP);
	glDetachShader(_shaderId, _shaderVP);
    
	glDeleteShader(_shaderFP);
	glDeleteShader(_shaderVP);
	glDeleteProgram(_shaderId);
}

U32 glShader::getId() {
	return _shaderId;
}

void glShader::bind() {
	if(!_shaderId || Engine::getInstance().isWireframeRendering()) return;
	glUseProgram(_shaderId);
}

void glShader::unbind() {
	glUseProgram(0);
}

void glShader::UniformTexture(const std::string& ext, int slot)
{
	glActiveTexture(GL_TEXTURE0+slot);
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), slot);
}

void glShader::Uniform(const std::string& ext, int value)
{
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const std::string& ext, bool state)
{
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), state ? 1 : 0);
}

void glShader::Uniform(const std::string& ext, F32 value)
{
	glUniform1f(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const std::string& ext, const vec2& value)
{
	glUniform2fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const std::string& ext, const vec3& value)
{
	glUniform3fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const std::string& ext, const vec4& value)
{
	glUniform4fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}