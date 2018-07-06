#include "Headers/ShaderManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Hardware/Video/Headers/GFXDevice.h"

ShaderManager::ShaderManager() : _nullShader(NULL), _init(false)
{
}

ShaderManager::~ShaderManager()
{
}

void ShaderManager::Destroy() {
    GFX_DEVICE.deInitShaders();
    RemoveResource(_nullShader);
}

bool ShaderManager::init(){
    if(_init){
        ERROR_FN(Locale::get("WARNING_SHADER_MANAGER_DOUBLE_INIT"));
    }
    _init = GFX_DEVICE.initShaders();
    return _init;
}

void ShaderManager::unregisterShaderProgram(const std::string& name){
    ShaderProgramMap::iterator it = _shaderPrograms.find(name);
    if(it != _shaderPrograms.end()){
        _shaderPrograms.erase(it);
    }else{
        ERROR_FN(Locale::get("ERROR_SHADER_REMOVE_NOT_FOUND"),name.c_str());
    }
}

void ShaderManager::registerShaderProgram(const std::string& name, ShaderProgram* const shaderProgram){
    if(_shaderPrograms.find(name) != _shaderPrograms.end()){
        SAFE_UPDATE(_shaderPrograms[name], shaderProgram);
    }else{
        _shaderPrograms.insert(std::make_pair(name,shaderProgram));
    }
}

U8 ShaderManager::update(const D32 deltaTime){
    for_each(ShaderProgramMap::value_type& it, _shaderPrograms){
        std::string name = it.first;
        if(it.second->update(deltaTime) == 0)
            return 0;//+ Error
    }
    return 1;
}

U8 ShaderManager::idle(){
    if(_recompileQueue.empty()) return 0;
    _recompileQueue.top()->recompile(true,true,true,true);
    _recompileQueue.pop();
    return 1;
}

void ShaderManager::refresh(){
    for_each(ShaderProgramMap::value_type& it, _shaderPrograms){
        it.second->refresh();
    }
}

char* ShaderManager::shaderFileRead(const std::string &atomName, const std::string& location){
    AtomMap::iterator it = _atoms.find(atomName);
    if(it != _atoms.end()){
        return (char*)(it->second);
    }

    if(location.empty()){
        return NULL;
    }

    std::string file = location+"/"+atomName;
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
        _atoms.insert(std::make_pair(atomName,content));
    }

    return content;
}

I8  ShaderManager::shaderFileWrite(char *atomName, const char *s){
    FILE *fp = NULL;
    I8 status = 0;

    if (atomName != NULL) {
        fopen_s(&fp,atomName,"w");

        if (fp != NULL) {
            if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
                status = 1;
            fclose(fp);
        }
    }
    return(status);
}

void ShaderManager::removeShader(Shader* s){
    std::string name = s->getName();
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if(it != _shaderNameMap.end()){
        if(s->SubRef()){
            SAFE_DELETE(it->second);
            _shaderNameMap.erase(name);
        }
    }
}

Shader* ShaderManager::findShader(const std::string& name,const bool recompile){
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if(it != _shaderNameMap.end()){
        if(!recompile){
            it->second->AddRef();//<We don't need a ref count increase if we just recompile the shader
            D_PRINT_FN(Locale::get("SHADER_MANAGER_GET_SHADER_INC"),name.c_str(), it->second->getRefCount());
        }
        return it->second;
    }
    return NULL;
}

Shader* ShaderManager::loadShader(const std::string& name, const std::string& source,const ShaderType& type,const bool recompile){
    Shader* shader = findShader(name,recompile);

    if(!recompile){
        if(shader != NULL) return shader;

        shader = GFX_DEVICE.newShader(name, type);
    }

    if(!shader->load(source)){
        SAFE_DELETE(shader);
    }else{
        if(recompile) _shaderNameMap[name] = shader;
        else _shaderNameMap.insert(std::make_pair(name,shader));
    }

    return shader;
}

bool ShaderManager::recompileShaderProgram(const std::string& name) {
    bool state = false;
    for_each(ShaderProgramMap::value_type& it, _shaderPrograms){
        const std::string& shaderName = it.second->getName();
        if(shaderName.find(name) != std::string::npos || shaderName.compare(name) == 0){
            _recompileQueue.push(it.second);
            state = true;
        }
    }
    if(!state) ERROR_FN(Locale::get("ERROR_SHADER_RECOMPILE_NOT_FOUND"),name.c_str());
    return state;
}

bool ShaderManager::unbind(){
    if(!_nullShader){
        _nullShader = CreateResource<ShaderProgram>(ResourceDescriptor("NULL_SHADER"));
        ///the null shader should never be NULL!!!!
        assert(_nullShader != NULL); //LoL -Ionut
    }
    //Should use program 0 and set previous shader ID to 0 as well
    _nullShader->bind();
    return true;
}