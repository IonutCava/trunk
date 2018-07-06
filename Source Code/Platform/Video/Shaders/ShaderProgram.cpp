#include "stdafx.h"

#include "Headers/ShaderProgram.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileUpdateMonitor.h"
#include "Platform/File/Headers/FileWatcherManager.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
    bool ShaderProgram::s_useShaderTextCache = false;
    bool ShaderProgram::s_useShaderBinaryCache = false;

namespace {
    UpdateListener s_fileWatcherListener([](const char* atomName, FileUpdateEvent evt) {
        ShaderProgram::onAtomChange(atomName, evt);
    });
};

ShaderProgram_ptr ShaderProgram::s_imShader;
ShaderProgram_ptr ShaderProgram::s_nullShader;
ShaderProgram::AtomMap ShaderProgram::s_atoms;
ShaderProgram::ShaderQueue ShaderProgram::s_recompileQueue;
ShaderProgram::ShaderProgramMap ShaderProgram::s_shaderPrograms;

SharedLock ShaderProgram::s_atomLock;
SharedLock ShaderProgram::s_programLock;

I64 ShaderProgram::s_shaderFileWatcherID = -1;

ShaderProgram::ShaderProgram(GFXDevice& context, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, bool asyncLoad)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, name, resourceName, resourceLocation),
      GraphicsResource(context, GraphicsResource::Type::SHADER_PROGRAM, getGUID()),
      _asyncLoad(asyncLoad),
      _shouldRecompile(false)
{
    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get(_ID("SHADER_PROGRAM_REMOVE")), name().c_str());
}

bool ShaderProgram::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    registerShaderProgram(std::dynamic_pointer_cast<ShaderProgram>(shared_from_this()));

    return CachedResource::load(onLoadCallback);
}

bool ShaderProgram::unload() {
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
void ShaderProgram::addShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    vectorImpl<stringImpl>::iterator it =
        std::find(std::begin(_definesList), std::end(_definesList), define);
    // If we can't find it, we add it
    if (it == std::end(_definesList)) {
        _definesList.push_back(define);
        _shouldRecompile = getState() == ResourceState::RES_LOADED;
    } else {
        // If we did find it, we'll show an error message in debug builds about
        // double add
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_ADD")),
                           define.c_str(), name().c_str());
    }
}

/// Remove a define from the shader. The defined must have been added previously
void ShaderProgram::removeShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    vectorImpl<stringImpl>::iterator it =
        std::find(std::begin(_definesList), std::end(_definesList), define);
    // If we find it, we remove it
    if (it != std::end(_definesList)) {
        _definesList.erase(it);
        _shouldRecompile = getState() == ResourceState::RES_LOADED;
    } else {
        // If we did not find it, we'll show an error message in debug builds
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_DELETE")),
                           define.c_str(), name().c_str());
    }
}

/// Rebuild the specified shader stages from source code
bool ShaderProgram::recompile() {
    if (getState() != ResourceState::RES_LOADED) {
        return true;
    }
    _usedAtoms.clear();
    if (recompileInternal()) {
        _shouldRecompile = false;
        return true;
    }

    return false;
}

void ShaderProgram::registerAtomFile(const stringImpl& atomFile) {
    _usedAtoms.push_back(atomFile);
}

//================================ static methods ========================================
void ShaderProgram::idle() {
    // If we don't have any shaders queued for recompilation, return early
    if (!s_recompileQueue.empty()) {
        // Else, recompile the top program from the queue
        if (!s_recompileQueue.top()->recompile()) {
            // error
        }
        s_recompileQueue.pop();
    }
}

/// Calling this will force a recompilation of all shader stages for the program
/// that matches the name specified
bool ShaderProgram::recompileShaderProgram(const stringImpl& name) {
    bool state = false;
    ReadLock r_lock(s_programLock);

    // Find the shader program
    for (const ShaderProgram_wptr& shader : s_shaderPrograms) {
        assert(!shader.expired());
        
        const stringImpl& shaderName = shader.lock()->name();
        // Check if the name matches any of the program's name components    
        if (shaderName.find(name) != stringImpl::npos || shaderName.compare(name) == 0) {
            // We process every partial match. So add it to the recompilation queue
            s_recompileQueue.push(shader.lock());
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

/// Open the file found at 'filePath' matching 'atomName' and return it's source code
const stringImpl& ShaderProgram::shaderFileRead(const stringImpl& filePath, const stringImpl& atomName) {
    U64 atomNameHash = _ID_RT(atomName);
    // See if the atom was previously loaded and still in cache
    UpgradableReadLock ur_lock(s_atomLock);
    AtomMap::iterator it = s_atoms.find(atomNameHash);
    // If that's the case, return the code from cache
    if (it != std::cend(s_atoms)) {
        return it->second;
    }

    // If we forgot to specify an atom location, we have nothing to return
    assert(!filePath.empty());

    // Open the atom file and add the code to the atom cache for future reference
    stringImpl output;
    readFile(filePath, atomName, output, FileType::TEXT);

    UpgradeToWriteLock w_lock(ur_lock);
    auto result = hashAlg::insert(s_atoms, atomNameHash, output);

    assert(result.second);

    // Return the source code
    return result.first->second;
}

void ShaderProgram::shaderFileRead(const stringImpl& filePath,
                                   const stringImpl& fileName,
                                   stringImpl& sourceCodeOut) {
    stringImpl variant = fileName;
    if (Config::Build::IS_DEBUG_BUILD) {
        variant.append(".debug");
    } else if (Config::Build::IS_PROFILE_BUILD) {
        variant.append(".profile");
    } else {
        variant.append(".release");
    }
    readFile(filePath, variant, sourceCodeOut, FileType::TEXT);
}

/// Dump the source code 's' of atom file 'atomName' to file
void ShaderProgram::shaderFileWrite(const stringImpl& filePath,
                                    const stringImpl& fileName,
                                    const char* sourceCode) {
    stringImpl variant = fileName;
    if (Config::Build::IS_DEBUG_BUILD) {
        variant.append(".debug");
    } else if (Config::Build::IS_PROFILE_BUILD) {
        variant.append(".profile");
    } else {
        variant.append(".release");
    }

    writeFile(filePath,
              variant,
              (bufferPtr)sourceCode,
              strlen(sourceCode),
              FileType::TEXT);
}

void ShaderProgram::onStartup(GFXDevice& context, ResourceCache& parentCache) {
    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcher& watcher = FileWatcherManager::allocateWatcher();
        s_shaderFileWatcherID = watcher.getGUID();
        s_fileWatcherListener.addIgnoredEndCharacter('~');
        s_fileWatcherListener.addIgnoredExtension("tmp");

        vectorImpl<stringImpl> atomLocations = getAllAtomLocations();
        for (const stringImpl& loc : atomLocations) {
            createDirectories(loc.c_str());
            watcher().addWatch(loc, &s_fileWatcherListener);
        }
    }

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
        //WriteLock w_lock(_programLock);
        s_shaderPrograms.clear();
    }
    s_nullShader.reset();
    s_imShader.reset();
    while (!s_recompileQueue.empty()) {
        s_recompileQueue.pop();
    }
    FileWatcherManager::deallocateWatcher(s_shaderFileWatcherID);
    s_shaderFileWatcherID = -1;
}

bool ShaderProgram::updateAll(const U64 deltaTimeUS) {
    ReadLock r_lock(s_programLock);
    // Pass the update call to all registered programs
    for (ShaderProgram_wptr& shader : s_shaderPrograms) {
        if (!shader.lock()->update(deltaTimeUS)) {
            // If an update call fails, stop updating
            return false;
        }
    }
    return true;
}

/// Whenever a new program is created, it's registered with the manager
void ShaderProgram::registerShaderProgram(const ShaderProgram_ptr& shaderProgram) {
    size_t shaderHash = shaderProgram->getDescriptorHash();
    unregisterShaderProgram(shaderHash);

    WriteLock w_lock(s_programLock);
    s_shaderPrograms.push_back(shaderProgram);
}

/// Unloading/Deleting a program will unregister it from the manager
bool ShaderProgram::unregisterShaderProgram(size_t shaderHash) {
    UpgradableReadLock ur_lock(s_programLock);
    if (s_shaderPrograms.empty()) {
        // application shutdown?
        return true;
    }

    ShaderProgramMap::const_iterator it = std::find_if(std::cbegin(s_shaderPrograms),
        std::cend(s_shaderPrograms),
        [shaderHash](const ShaderProgram_wptr& item) {
            return item.expired() || item.lock()->getDescriptorHash() == shaderHash;
        });

    if (it != std::cend(s_shaderPrograms)) {
        UpgradeToWriteLock w_lock(ur_lock);
        s_shaderPrograms.erase(it);
        return true;
    }

    return false;
}

ShaderProgram_wptr ShaderProgram::findShaderProgram(U32 shaderHandle) {
    for (const ShaderProgram_wptr& shader : s_shaderPrograms) {
        assert(!shader.expired());
        if (shader.lock()->getID() == shaderHandle) {
            return shader;
        }
    }

    return ShaderProgram_wptr();
}

ShaderProgram_wptr ShaderProgram::findShaderProgram(size_t shaderHash) {
    for (const ShaderProgram_wptr& shader : s_shaderPrograms) {
        assert(!shader.expired());
        if (shader.lock()->getDescriptorHash() == shaderHash) {
            return shader;
        }
    }

    return ShaderProgram_wptr();
}

const ShaderProgram_ptr& ShaderProgram::defaultShader() {
    return s_imShader;
}

const ShaderProgram_ptr& ShaderProgram::nullShader() {
    return s_nullShader;
}

void ShaderProgram::rebuildAllShaders() {
    ReadLock r_lock(s_programLock);
    for (const ShaderProgram_wptr& shader : s_shaderPrograms) {
        s_recompileQueue.push(shader.lock());
    }
}

void ShaderProgram::onAtomChange(const char* atomName, FileUpdateEvent evt) {
    if (evt == FileUpdateEvent::DELETE) {
        // Do nothing if the specified file is "deleted"
        // We do not want to break running programs
        return;
    }
    // ADD and MODIFY events should get processed as usual

    // Clear the atom from the cache

    {
        WriteLock w_lock(s_atomLock);
        AtomMap::iterator it = s_atoms.find(_ID_RT(atomName));
        if (it != std::cend(s_atoms)) {
            it = s_atoms.erase(it);
        }
    }
    //Get list of shader programs that use the atom and rebuild all shaders in list;
    ReadLock r_lock(s_programLock);
    for (const ShaderProgram_wptr& shader : s_shaderPrograms) {
        const ShaderProgram_ptr& shaderProgram = shader.lock();
        for (const stringImpl& atom : shaderProgram->_usedAtoms) {
            if (Util::CompareIgnoreCase(atom, atomName)) {
                s_recompileQueue.push(shaderProgram);
                break;
            }
        }
    }
}

vectorImpl<stringImpl> ShaderProgram::getAllAtomLocations() {
    static vectorImpl<stringImpl> atomLocations;
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
