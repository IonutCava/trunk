#include "stdafx.h"

#include "Headers/ShaderProgram.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
    bool ShaderProgram::s_useShaderTextCache = false;
    bool ShaderProgram::s_useShaderBinaryCache = false;


ShaderProgram_ptr ShaderProgram::s_imShader;
ShaderProgram_ptr ShaderProgram::s_nullShader;
ShaderProgram::ShaderQueue ShaderProgram::s_recompileQueue;
ShaderProgram::ShaderProgramMap ShaderProgram::s_shaderPrograms;

SharedMutex ShaderProgram::s_programLock;

ShaderProgram::ShaderProgram(GFXDevice& context, 
                             size_t descriptorHash,
                             const stringImpl& shaderName,
                             const stringImpl& shaderFileName,
                             const stringImpl& shaderFileLocation,
                             bool asyncLoad)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, shaderName, shaderFileName, shaderFileLocation),
      GraphicsResource(context, GraphicsResource::Type::SHADER_PROGRAM, getGUID()),
      _asyncLoad(asyncLoad),
      _shouldRecompile(false)
{
    if (shaderFileName.empty()) {
        assetName(resourceName());
    }

    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get(_ID("SHADER_PROGRAM_REMOVE")), resourceName().c_str());
}

bool ShaderProgram::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    if (!weak_from_this().expired()) {
        registerShaderProgram(std::dynamic_pointer_cast<ShaderProgram>(shared_from_this()).get());
    }

    return CachedResource::load(onLoadCallback);
}

bool ShaderProgram::unload() noexcept {
    // Unregister the program from the manager
    return unregisterShaderProgram(getDescriptorHash());
}

/// Called once per frame. Update common values used across programs
bool ShaderProgram::update(const U64 deltaTimeUS) {
    if (_shouldRecompile) {
        recompile();
    }

    return true;
}

/// Add a define to the shader. The defined must not have been added previously
void ShaderProgram::addShaderDefine(const stringImpl& define, bool appendPrefix) {
    // Find the string in the list of program defines
    auto it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, appendPrefix));
    // If we can't find it, we add it
    if (it == std::end(_definesList)) {
        _definesList.push_back(std::make_pair(define, appendPrefix));
        _shouldRecompile = getState() == ResourceState::RES_LOADED;
    } else {
        // If we did find it, we'll show an error message in debug builds about double add
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_ADD")), define.c_str(), resourceName().c_str());
    }
}

/// Remove a define from the shader. The defined must have been added previously
void ShaderProgram::removeShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    auto it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, true));
        if (it == std::end(_definesList)) {
            it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, false));
        }
    // If we find it, we remove it
    if (it != std::end(_definesList)) {
        _definesList.erase(it);
        _shouldRecompile = getState() == ResourceState::RES_LOADED;
    } else {
        // If we did not find it, we'll show an error message in debug builds
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_DELETE")), define.c_str(), resourceName().c_str());
    }
}

/// Rebuild the specified shader stages from source code
bool ShaderProgram::recompile() {
    if (getState() != ResourceState::RES_LOADED) {
        return true;
    }
    if (recompileInternal()) {
        _shouldRecompile = false;
        return true;
    }

    return false;
}

//================================ static methods ========================================
void ShaderProgram::idle() {
    // If we don't have any shaders queued for recompilation, return early
    if (!s_recompileQueue.empty()) {
        // Else, recompile the top program from the queue
        if (!s_recompileQueue.top()->recompile()) {
            // error
        }
        //Re-register because the handle is probably different by now
        registerShaderProgram(s_recompileQueue.top());
        s_recompileQueue.pop();
    }
}

/// Calling this will force a recompilation of all shader stages for the program
/// that matches the name specified
bool ShaderProgram::recompileShaderProgram(const stringImpl& name) {
    bool state = false;
    SharedLock r_lock(s_programLock);

    // Find the shader program
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        const ShaderProgramMapEntry& shader = it.second;
        
        ShaderProgram* program = shader.first;
        const stringImpl& shaderName = program->resourceName();
        // Check if the name matches any of the program's name components    
        if (shaderName.find(name) != stringImpl::npos || shaderName.compare(name) == 0) {
            // We process every partial match. So add it to the recompilation queue
            s_recompileQueue.push(program);
            // Mark as found
            state = true;
        }
    }
    // If no shaders were found, show an error
    if (!state) {
        Console::errorfn(Locale::get(_ID("ERROR_RECOMPILE_NOT_FOUND")),  name.c_str());
    }

    return state;
}

void ShaderProgram::onStartup(GFXDevice& context, ResourceCache& parentCache) {
    // Create an immediate mode rendering shader that simulates the fixed function pipeline
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    immediateModeShader.setThreadedLoading(false);
    s_imShader = CreateResource<ShaderProgram>(parentCache, immediateModeShader);
    assert(s_imShader != nullptr);

    // Create a null shader (basically telling the API to not use any shaders when bound)
    s_nullShader = CreateResource<ShaderProgram>(parentCache, ResourceDescriptor("NULL"));
    // The null shader should never be nullptr!!!!
    assert(s_nullShader != nullptr);  // LoL -Ionut
}

void ShaderProgram::onShutdown() {
    // Make sure we unload all shaders
    {
        //Lock w_lock(_programLock);
        s_shaderPrograms.clear();
    }
    s_nullShader.reset();
    s_imShader.reset();
    while (!s_recompileQueue.empty()) {
        s_recompileQueue.pop();
    }
}

bool ShaderProgram::updateAll(const U64 deltaTimeUS) {
    SharedLock r_lock(s_programLock);
    // Pass the update call to all registered programs
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        if (!it.second.first->update(deltaTimeUS)) {
            // If an update call fails, stop updating
            return false;
        }
    }
    return true;
}

/// Whenever a new program is created, it's registered with the manager
void ShaderProgram::registerShaderProgram(ShaderProgram* shaderProgram) {
    size_t shaderHash = shaderProgram->getDescriptorHash();
    unregisterShaderProgram(shaderHash);

    UniqueLockShared w_lock(s_programLock);
    s_shaderPrograms[shaderProgram->getID()] = { shaderProgram, shaderHash };
}

/// Unloading/Deleting a program will unregister it from the manager
bool ShaderProgram::unregisterShaderProgram(size_t shaderHash) {

    UniqueLockShared lock(s_programLock);
    if (s_shaderPrograms.empty()) {
        // application shutdown?
        return true;
    }

    ShaderProgramMap::const_iterator it = std::find_if(
        std::cbegin(s_shaderPrograms),
        std::cend(s_shaderPrograms),
        [shaderHash](const ShaderProgramMap::value_type& item) {
            return item.second.second == shaderHash;
        });

    if (it != std::cend(s_shaderPrograms)) {
        s_shaderPrograms.erase(it);
        return true;
    }

    // application shutdown?
    return false;
}

ShaderProgram& ShaderProgram::findShaderProgram(U32 shaderHandle, bool& success) {
    SharedLock r_lock(s_programLock);
    auto it = s_shaderPrograms.find(shaderHandle);
    if (it != std::cend(s_shaderPrograms)) {
        success = true;
        return *it->second.first;
    }

    success = false;
    return *defaultShader().get();
}

ShaderProgram& ShaderProgram::findShaderProgram(size_t shaderHash, bool& success) {
    SharedLock r_lock(s_programLock);
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        if (it.second.second == shaderHash) {
            success = true;
            return *it.second.first;
        }
    }

    success = false;
    return *defaultShader().get();
}

const ShaderProgram_ptr& ShaderProgram::defaultShader() {
    return s_imShader;
}

const ShaderProgram_ptr& ShaderProgram::nullShader() {
    return s_nullShader;
}

void ShaderProgram::rebuildAllShaders() {
    SharedLock r_lock(s_programLock);
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        s_recompileQueue.push(it.second.first);
    }
}

vector<stringImpl> ShaderProgram::getAllAtomLocations() {
    static vector<stringImpl> atomLocations;
        if (atomLocations.empty()) {
        // General
        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation);
        // GLSL
        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_comnAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_compAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_fragAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_geomAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_tescAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_teseAtomLoc);

        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::GLSL::g_parentShaderLoc +
                                   Paths::Shaders::GLSL::g_vertAtomLoc);
        // HLSL
        atomLocations.emplace_back(Paths::g_assetsLocation +
                                   Paths::g_shadersLocation +
                                   Paths::Shaders::HLSL::g_parentShaderLoc);

    }

    return atomLocations;
}
};
