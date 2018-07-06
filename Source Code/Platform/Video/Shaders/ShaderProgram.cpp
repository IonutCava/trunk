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
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
namespace {
    UpdateListener s_fileWatcherListener([](const char* atomName, FileUpdateEvent evt) {
        ShaderProgram::onAtomChange(atomName, evt);
    });
};

ShaderProgram_ptr ShaderProgram::_imShader;
ShaderProgram_ptr ShaderProgram::_nullShader;
ShaderProgram::AtomMap ShaderProgram::_atoms;
ShaderProgram::ShaderQueue ShaderProgram::_recompileQueue;
ShaderProgram::ShaderProgramMap ShaderProgram::_shaderPrograms;

SharedLock ShaderProgram::_programLock;

std::unique_ptr<FW::FileWatcher> ShaderProgram::s_shaderFileWatcher;

ShaderProgram::ShaderProgram(GFXDevice& context, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, bool asyncLoad)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, name, resourceName, resourceLocation),
      GraphicsResource(context, getGUID()),
      _asyncLoad(asyncLoad),
      _shouldRecompile(false)
{
    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get(_ID("SHADER_PROGRAM_REMOVE")), getName().c_str());
}

bool ShaderProgram::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    registerShaderProgram(std::dynamic_pointer_cast<ShaderProgram>(shared_from_this()));

    return CachedResource::load(onLoadCallback);
}

bool ShaderProgram::unload() {
    // Unregister the program from the manager
    ReadLock r_lock(_programLock);
    if (!_shaderPrograms.empty()) {
        unregisterShaderProgram(getDescriptorHash());
    }

    return true;
}

/// Called once per frame. Update common values used across programs
bool ShaderProgram::update(const U64 deltaTime) {
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
                           define.c_str(), getName().c_str());
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
                           define.c_str(), getName().c_str());
    }
}

/// Rebuild the specified shader stages from source code
bool ShaderProgram::recompile() {
    if (getState() != ResourceState::RES_LOADED) {
        return true;
    }
    _usedAtoms.clear();
    // Remember bind state
    bool wasBound = isBound();
    if (wasBound) {
        ShaderProgram::unbind();
    }
    bool state = recompileInternal();

    // Restore bind state
    if (wasBound) {
        bind();
    }
    _shouldRecompile = false;
    return state;
}

void ShaderProgram::registerAtomFile(const stringImpl& atomFile) {
    _usedAtoms.push_back(atomFile);
}

//================================ static methods ========================================
void ShaderProgram::idle() {
    // If we don't have any shaders queued for recompilation, return early
    if (!_recompileQueue.empty()) {
        // Else, recompile the top program from the queue
        if (!_recompileQueue.top()->recompile()) {
            // error
        }
        _recompileQueue.pop();
    }
    if (!Config::Build::IS_SHIPPING_BUILD) {
        s_shaderFileWatcher->update();
    }
}

/// Calling this will force a recompilation of all shader stages for the program
/// that matches the name specified
bool ShaderProgram::recompileShaderProgram(const stringImpl& name) {
    bool state = false;
    ReadLock r_lock(_programLock);

    // Find the shader program
    for (ShaderProgramMap::value_type& it : _shaderPrograms) {
        const stringImpl& shaderName = it.second->getName();
        // Check if the name matches any of the program's name components    
        if (shaderName.find(name) != stringImpl::npos || shaderName.compare(name) == 0) {
            // We process every partial match. So add it to the recompilation queue
            _recompileQueue.push(it.second);
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

/// Open the file found at 'location' matching 'atomName' and return it's source code
const stringImpl& ShaderProgram::shaderFileRead(const stringImpl& atomName, const stringImpl& location) {
    U64 atomNameHash = _ID_RT(atomName);
    // See if the atom was previously loaded and still in cache
    AtomMap::iterator it = _atoms.find(atomNameHash);
    // If that's the case, return the code from cache
    if (it != std::cend(_atoms)) {
        return it->second;
    }

    // If we forgot to specify an atom location, we have nothing to return
    assert(!location.empty());

    // Open the atom file and add the code to the atom cache for future reference
    stringImpl output;
    readFile(location + "/" + atomName, output, FileType::TEXT);
    std::pair<AtomMap::iterator, bool> result = hashAlg::insert(_atoms, atomNameHash, output);

    assert(result.second);

    // Return the source code
    return result.first->second;
}

void ShaderProgram::shaderFileRead(const stringImpl& filePath,
                                   bool buildVariant,
                                   stringImpl& sourceCodeOut) {
    if (buildVariant) {
        stringImpl variant = filePath;
        if (Config::Build::IS_DEBUG_BUILD) {
            variant.append(".debug");
        } else if (Config::Build::IS_PROFILE_BUILD) {
            variant.append(".profile");
        } else {
            variant.append(".release");
        }
        readFile(variant, sourceCodeOut, FileType::TEXT);
    } else {
        readFile(filePath, sourceCodeOut, FileType::TEXT);
    }
}

/// Dump the source code 's' of atom file 'atomName' to file
void ShaderProgram::shaderFileWrite(const stringImpl& atomName, const char* sourceCode) {
    stringImpl variant = atomName;
    if (Config::Build::IS_DEBUG_BUILD) {
        variant.append(".debug");
    } else if (Config::Build::IS_PROFILE_BUILD) {
        variant.append(".profile");
    } else {
        variant.append(".release");
    }

    writeFile(variant, (bufferPtr)sourceCode, strlen(sourceCode), FileType::TEXT);
}

void ShaderProgram::onStartup(GFXDevice& context, ResourceCache& parentCache) {
    s_shaderFileWatcher = std::make_unique<FW::FileWatcher>();

    if (!Config::Build::IS_SHIPPING_BUILD) {
        vectorImpl<stringImpl> atomLocations = getAllAtomLocations();
        for (const stringImpl& loc : atomLocations) {
            createDirectories(loc.c_str());
            s_fileWatcherListener.addIgnoredEndCharacter('~');
            s_fileWatcherListener.addIgnoredExtension("tmp");
            s_shaderFileWatcher->addWatch(loc, &s_fileWatcherListener);
        }
    }

    // Create an immediate mode rendering shader that simulates the fixed function pipeline
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    immediateModeShader.setThreadedLoading(false);
    _imShader = CreateResource<ShaderProgram>(parentCache, immediateModeShader);
    assert(_imShader != nullptr);

    // Create a null shader (basically telling the API to not use any shaders when bound)
    _nullShader = CreateResource<ShaderProgram>(parentCache, ResourceDescriptor("NULL"));
    // The null shader should never be nullptr!!!!
    assert(_nullShader != nullptr);  // LoL -Ionut

    _pushConstants = std::make_unique<PushConstants>(context);
}

void ShaderProgram::onShutdown() {
    // Make sure we unload all shaders
    _shaderPrograms.clear();
    _nullShader.reset();
    _imShader.reset();
    _pushConstants.reset();
    while (!_recompileQueue.empty()) {
        _recompileQueue.pop();
    }

    s_shaderFileWatcher.reset();
}

void ShaderProgram::preCommandSubmission() {
}

void ShaderProgram::postCommandSubmission() {
}

bool ShaderProgram::updateAll(const U64 deltaTime) {
    ReadLock r_lock(_programLock);
    // Pass the update call to all registered programs
    for (ShaderProgramMap::value_type& it : _shaderPrograms) {
        if (!it.second->update(deltaTime)) {
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

    WriteLock w_lock(_programLock);
    hashAlg::insert(_shaderPrograms, shaderHash, shaderProgram);
}

/// Unloading/Deleting a program will unregister it from the manager
bool ShaderProgram::unregisterShaderProgram(size_t shaderHash) {
    UpgradableReadLock ur_lock(_programLock);
    ShaderProgramMap::const_iterator it = _shaderPrograms.find(shaderHash);
    if (it != std::cend(_shaderPrograms)) {
        UpgradeToWriteLock w_lock(ur_lock);
        _shaderPrograms.erase(it);
        return true;
    }

    return false;
}

/// Bind the NULL shader which should have the same effect as using no shaders at all
bool ShaderProgram::unbind() {
    return _nullShader->bind();
}

const ShaderProgram_ptr& ShaderProgram::defaultShader() {
    return _imShader;
}


void ShaderProgram::rebuildAllShaders() {
    for (ShaderProgramMap::value_type& shader : _shaderPrograms) {
        _recompileQueue.push(shader.second);
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
    AtomMap::iterator it = _atoms.find(_ID_RT(atomName));
    if (it != std::cend(_atoms)) {
        it = _atoms.erase(it);
    }

    //Get list of shader programs that use the atom and rebuild all shaders in list;
    for (ShaderProgramMap::value_type& program : _shaderPrograms) {
        for (const stringImpl& atom : program.second->_usedAtoms) {
            if (Util::CompareIgnoreCase(atom, atomName)) {
                _recompileQueue.push(program.second);
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
