#include "Shader.h"

Shader::Shader(const std::string& name, SHADER_TYPE type){
  _name = name;
  _type = type;
  _compiled = false;
  _refCount = 1;
}

Shader::~Shader(){
	Console::getInstance().d_printfn("Deleting Shader  [ %s ]",getName().c_str());
}

char* Shader::shaderFileRead(const std::string &fn) {

	std::string file = fn;
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

I8 Shader::shaderFileWrite(char *fn, char *s) {

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
