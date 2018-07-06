#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

const char* Shader::CACHE_LOCATION_TEXT = "shaderCache/Text/";
const char* Shader::CACHE_LOCATION_BIN = "shaderCache/Binary/";

Shader::ShaderMap Shader::_shaderNameMap;

Shader::Shader(GFXDevice& context,
               const stringImpl& name,
               const ShaderType& type,
               const bool optimise)
    : GraphicsResource(context),
      _skipIncludes(false),
      _shader(std::numeric_limits<U32>::max()),
      _name(name),
      _type(type)
{
    _compiled = false;
}

Shader::~Shader()
{
    Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), getName().c_str());
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void Shader::removeShader(Shader* s) {
    // Keep a copy of it's name
    stringImpl name(s->getName());
    // Try to find it
    ULL nameHash = _ID_RT(name);
    ShaderMap::iterator it = _shaderNameMap.find(nameHash);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it.
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader
            MemoryManager::DELETE(it->second);
            _shaderNameMap.erase(nameHash);
        }
    }
}

/// Return a new shader reference
Shader* Shader::getShader(const stringImpl& name, const bool recompile) {
    // Try to find the shader
    ShaderMap::iterator it = _shaderNameMap.find(_ID_RT(name));
    if (it != std::end(_shaderNameMap)) {
        if (!recompile) {
            // We don't need a ref count increase if we just recompile the shader
            it->second->AddRef();
            Console::d_printfn(Locale::get(_ID("SHADER_MANAGER_GET_INC")),
                name.c_str(), it->second->GetRef());
        }
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
Shader* Shader::loadShader(const stringImpl& name,
                           const stringImpl& source,
                           const ShaderType& type,
                           const bool parseCode,
                           const bool recompile) {
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

    shader->skipIncludes(!parseCode);
    // At this stage, we have a valid Shader object, so load the source code
    if (!shader->load(source)) {
        // If loading the source code failed, delete it
        MemoryManager::DELETE(shader);
    } else {
        ULL nameHash = _ID_RT(name);
        // If we loaded the source code successfully, either update it (if we
        // recompiled) or register it
        if (recompile) {
            _shaderNameMap[nameHash] = shader;
        } else {
            hashAlg::emplace(_shaderNameMap, nameHash, shader);
        }
    }

    return shader;
}
};