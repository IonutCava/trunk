#include "Headers/ShaderProgram.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "simplefilewatcher/includes/FileWatcher.h"

namespace Divide {
namespace {
    class UpdateListener : public FW::FileWatchListener
    {
    public:
        UpdateListener() 
        {
        }

        void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
        {
            switch (action)
            {
                case FW::Actions::Add: break;
                case FW::Actions::Delete: break;
                case FW::Actions::Modified:
                    ShaderProgram::onAtomChange(filename.c_str());
                    break;

                default:
                    DIVIDE_UNEXPECTED_CALL("Unknown file event!");
            }
        };
    } s_fileWatcherListener;
};

ShaderProgram_ptr ShaderProgram::_imShader;
ShaderProgram_ptr ShaderProgram::_nullShader;
ShaderProgram::AtomMap ShaderProgram::_atoms;
ShaderProgram::ShaderQueue ShaderProgram::_recompileQueue;
ShaderProgram::ShaderProgramMap ShaderProgram::_shaderPrograms;
SharedLock ShaderProgram::_programLock;

std::unique_ptr<FW::FileWatcher> ShaderProgram::s_shaderFileWatcher;

ShaderProgram::ShaderProgram(GFXDevice& context, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, bool asyncLoad)
    : Resource(ResourceType::GPU_OBJECT, name, resourceName, resourceLocation),
      GraphicsResource(context, getGUID()),
      _asyncLoad(asyncLoad)
{
    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get(_ID("SHADER_PROGRAM_REMOVE")), getName().c_str());
}

bool ShaderProgram::load() {
    registerShaderProgram(getName(), shared_from_this());

    return Resource::load();
}

bool ShaderProgram::unload() {
    // Unregister the program from the manager
    ReadLock r_lock(_programLock);
    if (!_shaderPrograms.empty()) {
        unregisterShaderProgram(getName());
    }

    return true;
}

/// Called once per frame. Update common values used across programs
bool ShaderProgram::update(const U64 deltaTime) {
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

    s_shaderFileWatcher->update();
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
    std::pair<AtomMap::iterator, bool> result = hashAlg::emplace(_atoms, atomNameHash, output);

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
    }
    else if (Config::Build::IS_PROFILE_BUILD) {
        variant.append(".profile");
    }
    else {
        variant.append(".release");
    }

    writeFile(variant, sourceCode, FileType::TEXT);
}

void ShaderProgram::onStartup(ResourceCache& parentCache) {
    s_shaderFileWatcher = std::make_unique<FW::FileWatcher>();

    vectorImpl<stringImpl> atomLocations = getAllAtomLocations();
    for (const stringImpl& loc : atomLocations) {
        createDirectories(loc.c_str());
        s_shaderFileWatcher->addWatch(loc, &s_fileWatcherListener);
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
}

void ShaderProgram::onShutdown() {
    // Make sure we unload all shaders
    _shaderPrograms.clear();
    _nullShader.reset();
    _imShader.reset();
    while (!_recompileQueue.empty()) {
        _recompileQueue.pop();
    }

    s_shaderFileWatcher.reset();
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
void ShaderProgram::registerShaderProgram(const stringImpl& name, const ShaderProgram_ptr& shaderProgram) {
    unregisterShaderProgram(name);

    WriteLock w_lock(_programLock);
    hashAlg::emplace(_shaderPrograms, _ID_RT(name), shaderProgram);
}

/// Unloading/Deleting a program will unregister it from the manager
bool ShaderProgram::unregisterShaderProgram(const stringImpl& name) {
    UpgradableReadLock ur_lock(_programLock);
    ShaderProgramMap::const_iterator it = _shaderPrograms.find(_ID_RT(name));
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

void ShaderProgram::onAtomChange(const stringImpl& atomName) {
    //Get list of shader programs that use the atom and rebuild all shaders in list;
    for (ShaderProgramMap::value_type& it : _shaderPrograms) {
        for (const stringImpl& atom : it.second->_usedAtoms) {
            if (atom.compare(atomName) == 0) {
                _recompileQueue.push(it.second);
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
