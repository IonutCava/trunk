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
std::atomic_int ShaderProgram::s_shaderCount;

ShaderProgram::ShaderProgram(GFXDevice& context, 
                             size_t descriptorHash,
                             const Str128& shaderName,
                             const Str128& shaderFileName,
                             const stringImpl& shaderFileLocation,
                             const ShaderProgramDescriptor& descriptor,
                             bool asyncLoad)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, shaderName, shaderFileName, shaderFileLocation),
      GraphicsResource(context, GraphicsResource::Type::SHADER_PROGRAM, getGUID(), _ID(shaderName.c_str())),
      _asyncLoad(asyncLoad),
      _shouldRecompile(false)
{
    if (shaderFileName.empty()) {
        assetName(resourceName());
    }
    s_shaderCount.fetch_add(1, std::memory_order_relaxed);
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get(_ID("SHADER_PROGRAM_REMOVE")), resourceName().c_str());
    s_shaderCount.fetch_sub(1, std::memory_order_relaxed);
}

bool ShaderProgram::load() {
    return CachedResource::load();
}

bool ShaderProgram::unload() noexcept {
    // Unregister the program from the manager
    return unregisterShaderProgram(descriptorHash());
}


/// Rebuild the specified shader stages from source code
bool ShaderProgram::recompile(bool force) {
    if (getState() != ResourceState::RES_LOADED) {
        return true;
    }
    if (recompileInternal(force)) {
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
        if (!s_recompileQueue.top()->recompile(true)) {
            // error
        }
        //Re-register because the handle is probably different by now
        registerShaderProgram(s_recompileQueue.top());
        s_recompileQueue.pop();
    }
}

/// Calling this will force a recompilation of all shader stages for the program
/// that matches the name specified
bool ShaderProgram::recompileShaderProgram(const Str128& name) {
    bool state = false;
    SharedLock r_lock(s_programLock);

    // Find the shader program
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        const ShaderProgramMapEntry& shader = it.second;
        
        ShaderProgram* program = shader.first;
        const Str128& shaderName = program->resourceName();
        // Check if the name matches any of the program's name components    
        if (shaderName.find(name) != Str128::npos || shaderName.compare(name) == 0) {
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

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "ImmediateModeEmulation.glsl";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "ImmediateModeEmulation.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    // Create an immediate mode rendering shader that simulates the fixed function pipeline
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    immediateModeShader.threaded(false);
    immediateModeShader.propertyDescriptor(shaderDescriptor);
    s_imShader = CreateResource<ShaderProgram>(parentCache, immediateModeShader);
    assert(s_imShader != nullptr);

    shaderDescriptor._modules.clear();
    ResourceDescriptor shaderDesc("NULL");
    shaderDesc.threaded(false);
    shaderDesc.propertyDescriptor(shaderDescriptor);
    // Create a null shader (basically telling the API to not use any shaders when bound)
    s_nullShader = CreateResource<ShaderProgram>(parentCache, shaderDesc);
    // The null shader should never be nullptr!!!!
    assert(s_nullShader != nullptr);  // LoL -Ionut
}

void ShaderProgram::onShutdown() {
    // Make sure we unload all shaders
    s_nullShader.reset();
    s_imShader.reset();
    while (!s_recompileQueue.empty()) {
        s_recompileQueue.pop();
    }
    s_shaderPrograms.clear();
}

bool ShaderProgram::updateAll(const U64 deltaTimeUS) {
    static bool onOddFrame = false;

    onOddFrame = !onOddFrame;
    SharedLock r_lock(s_programLock);
    // Pass the update call to all registered programs
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        if (!Config::Build::IS_RELEASE_BUILD && onOddFrame) {
            it.second.first->recompile(false);
        }
        it.second.first->update(deltaTimeUS);
    }

    return true;
}

/// Whenever a new program is created, it's registered with the manager
void ShaderProgram::registerShaderProgram(ShaderProgram* shaderProgram) {
    size_t shaderHash = shaderProgram->descriptorHash();
    unregisterShaderProgram(shaderHash);

    UniqueLockShared w_lock(s_programLock);
    s_shaderPrograms[shaderProgram->getGUID()] = { shaderProgram, shaderHash };
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

ShaderProgram* ShaderProgram::findShaderProgram(I64 shaderHandle) {
    SharedLock r_lock(s_programLock);
    const auto& it = s_shaderPrograms.find(shaderHandle);
    if (it != std::cend(s_shaderPrograms)) {
        return it->second.first;
    }

    return nullptr;
}

ShaderProgram* ShaderProgram::findShaderProgram(size_t shaderHash) {
    SharedLock r_lock(s_programLock);
    for (const ShaderProgramMap::value_type& it : s_shaderPrograms) {
        if (it.second.second == shaderHash) {
            return it.second.first;
        }
    }

    return nullptr;
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

vector<Str256> ShaderProgram::getAllAtomLocations() {
    static vector<Str256> atomLocations;
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

size_t ShaderProgram::definesHash(const ModuleDefines& defines) {
    size_t hash = 499;
    for (const auto& entry : defines) {
        Util::Hash_combine(hash, _ID(entry.first.c_str()));
        Util::Hash_combine(hash, entry.second);
    }
    return hash;
}

};
