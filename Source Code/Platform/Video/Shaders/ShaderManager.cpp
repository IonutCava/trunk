#include "Headers/ShaderManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/Shader.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

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
    RemoveResource(_imShader);
    RemoveResource(_nullShader);
    GFX_DEVICE.deInitShaders();
    _atoms.clear();
}

/// Initialize the shader manager so that it may process program loading requests
bool ShaderManager::init() {    
    // Avoid double init requests
    if (_init) {
        Console::errorfn(Locale::get("WARNING_SHADER_MANAGER_DOUBLE_INIT"));
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
void ShaderManager::registerShaderProgram(const stringImpl& name, ShaderProgram* const shaderProgram) {
    ShaderProgramMap::iterator it = _shaderPrograms.find(name);
    // Either update an existing shader
    if (it != std::end(_shaderPrograms)) {
        MemoryManager::SAFE_UPDATE( it->second, shaderProgram );
    } else {
        // Or register a new one
        hashAlg::emplace(_shaderPrograms, name, shaderProgram);
    }
}

/// Unloading/Deleting a program will unregister it from the manager
void ShaderManager::unregisterShaderProgram(const stringImpl& name) {
    // The shader program must be registered in order to unregister it
    ShaderProgramMap::iterator it = _shaderPrograms.find(name);
    if (it != std::end(_shaderPrograms)) {
        _shaderPrograms.erase(it);
    } else {
        // Show an error if this isn't the case
        Console::errorfn(Locale::get("ERROR_SHADER_REMOVE_NOT_FOUND"),name.c_str());
    }
}

/// Called once per frame
bool ShaderManager::update(const U64 deltaTime) {
    // Pass the update call to all registered programs
    for ( ShaderProgramMap::value_type& it : _shaderPrograms ) {
        if ( !it.second->update( deltaTime ) ) {
            // If an update call fails, stop updating
            return false;
        }
    }
    return true;
}

/// Calling this will force a recompilation of all shader stages for the program that matches the name specified
bool ShaderManager::recompileShaderProgram(const stringImpl& name) {
    bool state = false;
    // Find the shader program
    for ( ShaderProgramMap::value_type& it : _shaderPrograms ) {
        const stringImpl& shaderName = it.second->getName();
        // Check if the name matches any of the program's name components
        if ( shaderName.find( name ) != stringImpl::npos || shaderName.compare( name ) == 0 ) {
            // We process every partial match. So add it to the recompilation queue
            _recompileQueue.push( it.second );
            // Mark as found
            state = true;
        }
    }
    // If no shaders were found, show an error
    if ( !state ) {
        Console::errorfn( Locale::get( "ERROR_SHADER_RECOMPILE_NOT_FOUND" ), name.c_str() );
    }

    return state;
}

/// Called after a swap buffer request
void ShaderManager::idle(){
    // If we don't have any shaders queued for recompilation, return early
    if ( !_recompileQueue.empty() ) {
        // Else, recompile the top program from the queue
        _recompileQueue.top()->recompile( true, true, true, true, true );
        _recompileQueue.pop();
    }
    return;
}

/// Pass uniform data update call to every registered program
void ShaderManager::refreshShaderData() {
    for (ShaderProgramMap::value_type& it : _shaderPrograms) {
        it.second->refreshShaderData();
    }
}

/// Pass scene data update call to every registered program
void ShaderManager::refreshSceneData() {
    for (ShaderProgramMap::value_type& it : _shaderPrograms) {
        it.second->refreshSceneData();
    }
}

/// Open the file found at 'location' matching 'atomName' and return it's source code
const stringImpl& ShaderManager::shaderFileRead(const stringImpl &atomName, const stringImpl& location) {
    // See if the atom was previously loaded and still in cache
    AtomMap::iterator it = _atoms.find(atomName);
    // If that's the case, return the code from cache
    if (it != std::end(_atoms)) {
        return it->second;
    }
    // If we forgot to specify an atom location, we have nothing to return
    assert(!location.empty());

    // Open the atom file
    stringImpl file = location+"/"+atomName;
    FILE *fp = nullptr;
    fopen_s(&fp,file.c_str(),"r");
    assert(fp != nullptr);

    // Read the contents
    fseek(fp, 0, SEEK_END);
    I32 count = ftell(fp);
    rewind(fp);
    assert(count > 0);

    char* content = MemoryManager_NEW char[count + 1];
    count = (I32)(fread(content,sizeof(char),count,fp));
    content[count] = '\0';
    fclose(fp);

    // Add the code to the atom cache for future reference
    hashAlg::pair<AtomMap::iterator, bool> result = hashAlg::emplace(_atoms, atomName, stringImpl(content));
    assert(result.second);
    MemoryManager::DELETE_ARRAY( content );
    
    // Return the source code
    return result.first->second;
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
    stringImpl name(s->getName());
    // Try to find it
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it. 
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader
            MemoryManager::DELETE( it->second );
            _shaderNameMap.erase(name);
        }
    }
}

/// Return a new shader reference
Shader* ShaderManager::getShader(const stringImpl& name, const bool recompile) {
    // Try to find the shader
    ShaderMap::iterator it = _shaderNameMap.find(name);
    if (it != std::end(_shaderNameMap)) {
        if (!recompile) {
            // We don't need a ref count increase if we just recompile the shader
            it->second->AddRef();
            Console::d_printfn(Locale::get("SHADER_MANAGER_GET_SHADER_INC"),name.c_str(), it->second->GetRef());
        }
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
Shader* ShaderManager::loadShader(const stringImpl& name, const stringImpl& source, const ShaderType& type,const bool recompile) {
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
        MemoryManager::DELETE( shader );
    } else {
        // If we loaded the source code successfully, either update it (if we recompiled) or register it
        if(recompile) {
            _shaderNameMap[name] = shader;
        } else {
            hashAlg::emplace(_shaderNameMap, name, shader);
        }
    }

    return shader;
}

/// Bind the NULL shader which should have the same effect as using no shaders at all
bool ShaderManager::unbind() {
    _nullShader->bind();
    return true;
}

};