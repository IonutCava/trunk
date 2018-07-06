#include "glResources.h"
#include "resource.h"
#include "glShader.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Hardware/Video/GFXDevice.h"

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
	Console::getInstance().printfn("GLSL Manager: Validating program:  %s ", buffer);
	I32 status;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
// ToDo: Problem with AMD(ATI) cards. GLSL validation errors about multiple samplers to same uniform, but they still work. Fix that. -Ionut
	if (status == GL_FALSE)
		Console::getInstance().errorfn("GLSL Manager: Error validating shader [ %d ]", program);
}



char* glShader::shaderFileRead(const string &fn) {

	string file = getResourceLocation()+ fn;
	FILE *fp = NULL;
	fopen_s(&fp,file.c_str(),"r");

	char *content = NULL;


	if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      I32 count = ftell(fp);
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

glShader::glShader(const char *vsFile, const char *fsFile)  : Shader(), _loaded(false), _bound(false)  {
    init(vsFile, fsFile);
}

bool glShader::loadFragOnly(const string& name){
	init("none",name.c_str());
	return true;
}

bool glShader::loadVertOnly(const string& name){
	init(name.c_str(),"none");
	return true;
}

bool glShader::load(const string& name){

	if(!_loaded){
		if(name.length() >= 6){
			size_t pos = name.find(",");
			if(pos != string::npos){
				string shaderFile1 = name.substr(0, pos);
				string shaderFile2 = name.substr(pos+1, name.length());
				string extension1(shaderFile1.substr(shaderFile1.length()- 5, shaderFile1.length()));
				string extension2(shaderFile2.substr(shaderFile2.length()- 5, shaderFile2.length()));

				if(extension1.compare(".frag") == 0 && extension2.compare(".vert") == 0){
					init(shaderFile2,shaderFile1);
					_fragName = shaderFile1;
					_vertName = shaderFile2;
				}else if(extension1.compare(".vert") == 0 && extension2.compare(".frag") == 0){
					init(shaderFile1,shaderFile2);
					_fragName = shaderFile2;
					_vertName = shaderFile1;
				}else{
					Console::getInstance().errorfn("GLSL: could not load shaders: %s",name.c_str());
				}
			}else{
				string extension(name.substr(name.length()- 5, name.length()));
				if(extension.compare(".frag") == 0) {
					_fragName = name;
					_vertName = "none";
					loadFragOnly(_fragName);
					
				}
				else if(extension.compare(".vert") == 0){
					_fragName = "none";
					_vertName = name;
					loadVertOnly(_vertName);
					
				}
				else{
					_fragName = name+".frag";
					_vertName = name+".vert";
					init(_vertName,_fragName);
					
				}
			
			}
		}else{
			init(name+".vert",name+".frag");
		}

		_loaded = true;
	}
	if(_loaded){
		Shader::load(name);
	}
	return _loaded;
}

void glShader::init(const string &vsFile, const string &fsFile) {
	bool useVert = false, useFrag = false;
	if(vsFile.compare("none") != 0) useVert = true;
	if(fsFile.compare("none") != 0) useFrag = true;

    if(useVert && useFrag) 
		Console::getInstance().printfn("GLSL Manager: Loading shaders %s and %s.",vsFile.c_str(),fsFile.c_str());
	if(useVert && !useFrag)
		Console::getInstance().printfn( "GLSL Manager: Loading shaders %s.",vsFile.c_str());
	if(!useVert && useFrag)
		Console::getInstance().printfn("GLSL Manager: Loading shaders %s.",fsFile.c_str());
	_shaderId = glCreateProgram();
	if(useVert){
		_shaderVP = glCreateShader(GL_VERTEX_SHADER);
		const char* vsText = shaderFileRead(vsFile);
		if(vsText == NULL){
			Console::getInstance().errorfn("GLSL Manager: Vertex shader [ %s ] not found!",vsFile.c_str());
			return;
		}
		glShaderSource(_shaderVP, 1, &vsText, 0);
		glCompileShader(_shaderVP);
		validateShader(_shaderVP, vsFile);
		glAttachShader(_shaderId, _shaderVP);
		delete vsText;
		vsText = NULL;
	}
	if(useFrag)	{
		_shaderFP = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fsText = shaderFileRead(fsFile);
		if(fsText == NULL)
		{
			Console::getInstance().errorfn("GLSL Manager: Fragment shader [ %s ] not found!",fsFile.c_str());
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
	if(_bound) return; //prevent double bind
	if(!_shaderId || GFXDevice::getInstance().wireframeRendering()) return;
	glUseProgram(_shaderId);
	_bound = true;
	Shader::bind(); //send default uniforms to GPU;
}

void glShader::unbind() {
	glUseProgram(0);
	_bound = false;
}

void glShader::Attribute(const std::string& ext, D32 value){
	glVertexAttrib1d(glGetAttribLocation(_shaderId, ext.c_str()),value);
}

void glShader::Attribute(const std::string& ext, F32 value){
	glVertexAttrib1f(glGetAttribLocation(_shaderId, ext.c_str()),value);
}

void glShader::Attribute(const std::string& ext, const vec2& value){
	glVertexAttrib2fv(glGetAttribLocation(_shaderId, ext.c_str()),value);
}

void glShader::Attribute(const std::string& ext, const vec3& value){
	glVertexAttrib3fv(glGetAttribLocation(_shaderId, ext.c_str()),value);
}

void glShader::Attribute(const std::string& ext, const vec4& value){
	glVertexAttrib4fv(glGetAttribLocation(_shaderId, ext.c_str()),value);
}


void glShader::Uniform(const string& ext, I32 value){
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const string& ext, F32 value){
	glUniform1f(glGetUniformLocation(_shaderId, ext.c_str()), value);
}

void glShader::Uniform(const string& ext, const vec2& value){
	glUniform2fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const string& ext, const vec3& value){
	glUniform3fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const string& ext, const vec4& value){
	glUniform4fv(glGetUniformLocation(_shaderId, ext.c_str()), 1, value);
}

void glShader::Uniform(const std::string& ext, const mat3& value){
	glUniformMatrix3fv(glGetUniformLocation(_shaderId, ext.c_str()), 1,false, value);
}

void glShader::Uniform(const std::string& ext, const mat4& value){
	glUniformMatrix4fv(glGetUniformLocation(_shaderId, ext.c_str()), 1,false, value);
}

void glShader::Uniform(const std::string& ext, const vector<mat4>& values){
	glUniformMatrix4fv(glGetUniformLocation(_shaderId, ext.c_str()),values.size(),true, values[0]);
}

void glShader::UniformTexture(const string& ext, U16 slot){
	glActiveTexture(GL_TEXTURE0+slot);
	glUniform1i(glGetUniformLocation(_shaderId, ext.c_str()), slot);
}


//For old shaders (v1.0) -Ionut
void glShader::Uniform(I32 location, const vec4& value){
	glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, location, value);
}