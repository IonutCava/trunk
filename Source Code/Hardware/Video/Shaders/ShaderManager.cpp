#include "Headers/ShaderManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Geometry/Material/Headers/Material.h"

ShaderManager::ShaderManager() :  _init(false),
                                  _imShader(nullptr),
                                  _nullShader(nullptr)
{
}

ShaderManager::~ShaderManager()
{
}

/// Remove the NULL and IM shaders and destroy the API specific shader loading system
void ShaderManager::destroy() {
    GFX_DEVICE.deInitShaders();
    RemoveResource(_imShader);
    RemoveResource(_nullShader);
    _atoms.clear();
}

/// Initialize the shader manager so that it may process program loading requests
bool ShaderManager::init() {    
    // Avoid double init requests
    if (_init) {
        ERROR_FN(Locale::get("WARNING_SHADER_MANAGER_DOUBLE_INIT"));
        return false;
    }
    // Initialize the rendering-API specific shader loading system
    _init = GFX_DEVICE.initShaders();
    // Create an immediate mode rendering shader that simulates the fixed function pipeline
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    immediateModeShader.setThreadedLoading(false);
    _imShader = CreateResource<ShaderProgram>(immediateModeShader);
    assert(_imShader != nullptr);
    // Create a null shader (basically telling the API to not use any shaders when bound)
    _nullShader = CreateResource<ShaderProgram>(ResourceDescriptor("NULL_SHADER"));
    // The null shader should never be nullptr!!!!
    assert(_nullShader != nullptr); //LoL -Ionut

    return _init;
}

/// Whenever a new program is created, it's registered with the manager
void ShaderManager::registerShaderProgram(const std::string& name, ShaderProgram* const shaderProgram) {
    // Either update an existing shader
    if (_shaderPrograms.find(name) != _shaderPrograms.end()) {
        SAFE_UPDATE(_shaderPrograms[name], shaderProgram);
    } else {
        // Or register a new one
        _shaderPrograms.insert(std::make_pair(name,shaderProgram));
    }
}

/// Unloading/Deleting a program will unregister it from the manager
void ShaderManager::unregisterShaderProgram(const std::string& name) {
    // The shader program must be registered in order to unregister it
    ShaderProgramMap::iterator it = _shaderPrograms.find(name);
    if (it != _shaderPrograms.end()) {
        _shaderPrograms.erase(it);
    } else {
        // Show an error if this isn't the case
        ERROR_FN(Locale::get("ERROR_SHADER_REMOVE_NOT_FOUND"),name.c_str());
    }
}

/// Called once per frame
U8 ShaderManager::update(const U64 deltaTime) {
    // Pass the update call to all registered programs
    FOR_EACH(ShaderProgramMap::value_type& it, _shaderPrograms) {
        if (it.second->update(deltaTime) == 0) {
            // If an update call fails, stop updating
            return 0;
        }
    }

    return 1;
}

/// Calling this will force a recompilation of all shader stages for the program that matches the name specified
bool ShaderManager::recompileShaderProgram(const std::string& name) {
    bool state = false;
    // Find the shader program
    FOR_EACH(ShaderProgramMap::value_type& it, _shaderPrograms) {
        const std::string& shaderName = it.second->getName();
        // Check if the name matches any of the program's name components
        if (shaderName.find(name) != std::string::npos || shaderName.compare(name) == 0) {
            // We process every partial match. So add it to the recompilation queue
            _recompileQueue.push(it.second);
            // Mark as found
            state = true;
        }
    }
    // If no shaders were found, show an error
    if (!state) {
        ERROR_FN(Locale::get("ERROR_SHADER_RECOMPILE_NOT_FOUND"),name.c_str());
    }

    return state;
}

/// Called after a swap buffer request
U8 ShaderManager::idle(){
    // If we don't have any shaders queued for recompilation, return early
    if (_recompileQueue.empty()) {
        return 0;
    }
    // Else, recompile the top program from the queue
    _recompileQueue.top()->recompile(true,true,true,true,true);
    _recompileQueue.pop();

    return 1;
}

/// Pass uniform data update call to every registered program
void ShaderManager::refreshShaderData() {
    FOR_EACH(ShaderProgramMap::value_type& it, _shaderPrograms) {
        it.second->refreshShaderData();
    }
}

/// Pass scene data update call to every registered program
void ShaderManager::refreshSceneData() {
   FOR_EACH(ShaderProgramMap::value_type& it, _shaderPrograms) {
        it.second->refreshSceneData();
    }
}

/// Open the file found at 'location' matching 'atomName' and return it's source code
const char* ShaderManager::shaderFileRead(const std::string &atomName, const std::string& location) {
    // See if the atom was previously loaded and still in cache
    AtomMap::iterator it = _atoms.find(atomName);
    // If that's the case, return the code from cache
    if (it != _atoms.end()) {
        return (char*)(it->second);
    }
    // If we forgot to specify an atom location, we have nothing to return
    if (location.empty()) {
        return nullptr;
    }
    // Open the atom file
    std::string file = location+"/"+atomName;
    FILE *fp = nullptr;
    fopen_s(&fp,file.c_str(),"r");
    // Read the contents
    const char* retContent = "";
    if (fp != nullptr) {
        fseek(fp, 0, SEEK_END);
        I32 count = ftell(fp);
        rewind(fp);
        if (count > 0) {
            char* content = New char[count+1];
            count = (I32)(fread(content,sizeof(char),count,fp));
            content[count] = '\0';
            retContent = strdup(content);
            // Add the code to the atom cache for future reference
            _atoms.insert(std::make_pair(atomName, retContent));
            SAFE_DELETE_ARRAY(content);
        }
        fclose(fp);
    }
    // Return the source code
    return retContent;
}

/// Dump the source code 's' of atom file 'atomName' to file
I8 ShaderManager::shaderFileWrite(char *atomName, const char *s) {
    I8 status = 0;

    if (atomName != nullptr) {
        // Create the target file (or open it for writing)
        FILE *fp = nullptr;
        fopen_s(&fp, atomName,"w");
        if (fp != nullptr) {
            // Dump the source code
            if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s)) {
                status = 1;
            }
            // Close the file
            fclose(fp);
        }
    }
    return status;
}

/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void ShaderManager::removeShader(Shader* s) {
    // Keep a copy of it's name
    std::string name(s->getName());
    // Try to find it
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if (it != _shaderNameMap.end()) {
        // Subtract one reference from it. 
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader
            SAFE_DELETE(it->second);
            _shaderNameMap.erase(name);
        }
    }
}

/// Return a new shader reference
Shader* ShaderManager::getShader(const std::string& name, const bool recompile) {
    // Try to find the shader
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if (it != _shaderNameMap.end()) {
        if (!recompile) {
            // We don't need a ref count increase if we just recompile the shader
            it->second->AddRef();
            D_PRINT_FN(Locale::get("SHADER_MANAGER_GET_SHADER_INC"),name.c_str(), it->second->getRefCount());
        }
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
Shader* ShaderManager::loadShader(const std::string& name, const std::string& source, const ShaderType& type,const bool recompile) {
    // See if we have the shader already loaded
    Shader* shader = getShader(name, recompile);
    if (!recompile) {
        // If we do, and don't need a recompile, just return it
        if (shader != nullptr) {
            return shader;
        }
        // If we can't find it, we create a new one
        shader = GFX_DEVICE.newShader(name, type);
    }
    // At this stage, we have a valid Shader object, so load the source code
    if (!shader->load(source)) {
        // If loading the source code failed, delete it
        SAFE_DELETE(shader);
    } else {
        // If we loaded the source code successfully, either update it (if we recompiled) or register it
        if(recompile) {
            _shaderNameMap[name] = shader;
        } else {
            _shaderNameMap.insert(std::make_pair(name,shader));
        }
    }

    return shader;
}

/// Bind the NULL shader which should have the same effect as using no shaders at all
bool ShaderManager::unbind() {
    _nullShader->bind();
    return true;
}