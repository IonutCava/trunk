#include "glResources.h"
#include "resource.h"
#include "glShader.h"
#include "Utility/Headers/ParamHandler.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"
using namespace std;

void glShader::validateShader(U16 shader, const string &file) {
	const U16 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	I32 length = 0;
    
	glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
	cout << "GLSL Manager: Validating shader: " << buffer << endl;
}

void glShader::validateProgram(U16 program) {
	const U16 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	I32 length = 0;
    
	memset(buffer, 0, BUFFER_SIZE);

	glValidateProgram(program);
	glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
	Con::getInstance().printfn("GLSL Manager: Validating program:  %s ", buffer);
	I32 status;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	if (status == GL_FALSE)
		Con::getInstance().errorfn("GLSL Manager: Error validating shader [ %d ]", program);
}



char* glShader::shaderFileRead(const string &fn) {

	string file = ParamHandler::getInstance().getParam<string>("assetsLocation") + "/shaders/" + fn;
	FILE *fp = NULL;
	fopen_s(&fp,file.c_str(),"r");

	char *content = NULL;

	I32 count=0;

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

I8 glShader::shaderFileWrite(char *fn, char *s) {

	FILE *fp = NULL;
	I8 status = 0;

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

bool glShader::loadFragOnly(const string& name)
{
	init("none",name.c_str());
	return true;
}

bool glShader::loadVertOnly(const string& name)
{
	init(name.c_str(),"none");
	return true;
}

bool glShader::load(const string& name)
{
	if(!_loaded)
	{
		_name = name;
		if(name.length() >= 6)
		{
			size_t pos = name.find(",");
			if(pos != string::npos)
			{
				string shaderFile1 = name.substr(0, pos);
				string shaderFile2 = name.substr(pos+1, name.length());
				string extension1(shaderFile1.substr(shaderFile1.length()- 5, shaderFile1.length()));
				string extension2(shaderFile2.substr(shaderFile2.length()- 5, shaderFile2.length()));
				if(extension1.compare(".frag") == 0 && extension2.compare(".vert") == 0)
					init(shaderFile2,shaderFile1);
				else if(extension1.compare(".frag") == 0 && extension2.compare(".vert") == 0)
					init(shaderFile1,shaderFile2);
				else
					Con::getInstance().errorfn("GLSL: could not load shaders: %s",name.c_str());
			}
			else
			{
				string extension(name.substr(name.length()- 5, name.length()));
				if(extension.compare(".frag") == 0) loadFragOnly(name);
				else if(extension.compare(".vert") == 0) loadVertOnly(name);
				else init(name+".vert",name+".frag");
			}
		}
		else
			init(name+".vert",name+".frag");

		_loaded = true;
	}
	return _loaded;
}

void glShader::init(const string &vsFile, const string &fsFile) {
	bool useVert = false, useFrag = false;
	if(vsFile.compare("none") != 0) useVert = true;
	if(fsFile.compare("none") != 0) useFrag = true;

    if(useVert && useFrag) 
		Con::getInstance().printfn("GLSL Manager: Loading shaders %s and %s.",vsFile.c_str(),fsFile.c_str());
	if(useVert && !useFrag)
		Con::getInstance().printfn( "GLSL Manager: Loading shaders %s.",vsFile.c_str());
	if(!useVert && useFrag)
		Con::getInstance().printfn("GLSL Manager: Loading shaders %s.",fsFile.c_str());

	_shaderId = glCreateProgram();
	if(useVert)
	{
		_shaderVP = glCreateShader(GL_VERTEX_SHADER);
		const char* vsText = shaderFileRead(vsFile);
		if(vsText == NULL)
		{
			Con::getInstance().errorfn("GLSL Manager: Vertex shader [ %s ] not found!",vsFile.c_str());
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
			Con::getInstance().errorfn("GLSL Manager: Fragment shader [ %s ] not found!",fsFile.c_str());
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

U16 glShader::getId() {
	return _shaderId;
}

void glShader::bind() {
	if(!_shaderId || GFXDevice::getInstance().wireframeRendering()) return;
	glUseProgram(_shaderId);
}

void glShader::unbind() {
	glUseProgram(0);
}

void glShader::UniformTexture(const string& ext, U16 slot)
{
	glActiveTexture(GL_TEXTURE0+slot);
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), slot);
}

void glShader::Uniform(const string& ext, I32 value)
{
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const string& ext, bool state)
{
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), state ? 1 : 0);
}

void glShader::Uniform(const string& ext, F32 value)
{
	glUniform1f(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const string& ext, const vec2& value)
{
	glUniform2fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const string& ext, const vec3& value)
{
	glUniform3fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const string& ext, const vec4& value)
{
	glUniform4fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const std::string& ext, const mat3& value)
{
	glUniformMatrix3fv(glGetUniformLocation(_shaderId, ext.c_str()), 1,false, value);
}

void glShader::Uniform(const std::string& ext, const mat4& value)
{
	glUniformMatrix4fv(glGetUniformLocation(_shaderId, ext.c_str()), 1,false, value);
}

//For old shaders (v1.0) -Ionut
void glShader::Uniform(I32 location, const vec4& value)
{
	glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, location, value);
}