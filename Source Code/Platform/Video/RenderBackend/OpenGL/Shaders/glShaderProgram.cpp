#include "stdafx.h"

#include "config.h"

#include "Headers/glShaderProgram.h"
#include "Core/Headers/PlatformContext.h"

#include "Platform/Video/RenderBackend/OpenGL/glsw/Headers/glsw.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/File/Headers/FileUpdateMonitor.h"
#include "Platform/File/Headers/FileWatcherManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    size_t g_validationBufferMaxSize = 4096 * 16;
    UpdateListener s_fileWatcherListener([](const char* atomName, FileUpdateEvent evt) {
        glShaderProgram::onAtomChange(atomName, evt);
    });
};

SharedMutex glShaderProgram::s_atomLock;
ShaderProgram::AtomMap glShaderProgram::s_atoms;
I64 glShaderProgram::s_shaderFileWatcherID = -1;
std::array<U32, to_base(ShaderType::COUNT)> glShaderProgram::_lineOffset;
stringImpl glShaderProgram::shaderAtomLocationPrefix[to_base(ShaderType::COUNT) + 1];
U64 glShaderProgram::shaderAtomExtensionHash[to_base(ShaderType::COUNT) + 1];
stringImpl glShaderProgram::shaderAtomExtensionName[to_base(ShaderType::COUNT) + 1];

void glShaderProgram::initStaticData() {
    const stringImpl locPrefix(Paths::g_assetsLocation + Paths::g_shadersLocation + Paths::Shaders::GLSL::g_parentShaderLoc);

    shaderAtomLocationPrefix[to_base(ShaderType::FRAGMENT)] = locPrefix + Paths::Shaders::GLSL::g_fragAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::VERTEX)] = locPrefix + Paths::Shaders::GLSL::g_vertAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::GEOMETRY)] = locPrefix + Paths::Shaders::GLSL::g_geomAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::TESSELATION_CTRL)] = locPrefix + Paths::Shaders::GLSL::g_tescAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::TESSELATION_EVAL)] = locPrefix + Paths::Shaders::GLSL::g_teseAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::COMPUTE)] = locPrefix + Paths::Shaders::GLSL::g_compAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::COUNT)] = locPrefix + Paths::Shaders::GLSL::g_comnAtomLoc;

    shaderAtomExtensionName[to_base(ShaderType::FRAGMENT)] = Paths::Shaders::GLSL::g_fragAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::VERTEX)] = Paths::Shaders::GLSL::g_vertAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::GEOMETRY)] = Paths::Shaders::GLSL::g_geomAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::TESSELATION_CTRL)] = Paths::Shaders::GLSL::g_tescAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::TESSELATION_EVAL)] = Paths::Shaders::GLSL::g_teseAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::COMPUTE)] = Paths::Shaders::GLSL::g_compAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::COUNT)] = Paths::Shaders::GLSL::g_comnAtomExt;

    for (U8 i = 0; i < to_base(ShaderType::COUNT) + 1; ++i) {
        shaderAtomExtensionHash[i] = _ID(shaderAtomExtensionName[i].c_str());
    }
}

void glShaderProgram::destroyStaticData() {
}

void glShaderProgram::onStartup(GFXDevice& context, ResourceCache& parentCache) {
    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcher& watcher = FileWatcherManager::allocateWatcher();
        s_shaderFileWatcherID = watcher.getGUID();
        s_fileWatcherListener.addIgnoredEndCharacter('~');
        s_fileWatcherListener.addIgnoredExtension("tmp");

        const vector<stringImpl> atomLocations = getAllAtomLocations();
        for (const stringImpl& loc : atomLocations) {
            createDirectories(loc.c_str());
            watcher().addWatch(loc, &s_fileWatcherListener);
        }
    }
}

void glShaderProgram::onShutdown() {
    FileWatcherManager::deallocateWatcher(s_shaderFileWatcherID);
    s_shaderFileWatcherID = -1;
}

glShaderProgram::glShaderProgram(GFXDevice& context,
                                 size_t descriptorHash,
                                 const stringImpl& name,
                                 const stringImpl& resourceName,
                                 const stringImpl& resourceLocation,
                                 const ShaderProgramDescriptor& descriptor,
                                 bool asyncLoad)
    : ShaderProgram(context, descriptorHash, name, resourceName, resourceLocation, descriptor, asyncLoad),
      glObject(glObjectType::TYPE_SHADER_PROGRAM, context),
      _stageMask(UseProgramStageMask::GL_NONE_BIT),
      _validated(false),
      _validationQueued(false),
      _descriptor(descriptor),
      _handle(GLUtil::_invalidObjectID)
{
}

glShaderProgram::~glShaderProgram()
{
    // delete shader program
    GL_API::deleteShaderPipelines(1, &_handle);
}

bool glShaderProgram::unload() noexcept {
    // Remove every shader attached to this program
    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        glShader::removeShader(shader);
    }
    _shaderStage.clear();

    return ShaderProgram::unload();
}

void glShaderProgram::validatePreBind() {
    if (!isValid()) {
        assert(getState() == ResourceState::RES_LOADED);
        glCreateProgramPipelines(1, &_handle);
        glObjectLabel(GL_PROGRAM_PIPELINE, _handle, -1, resourceName().c_str());
        _stageMask = UseProgramStageMask::GL_NONE_BIT;
        for (glShader* shader : _shaderStage) {
            // If a shader exists for said stage, attach it
            assert(shader != nullptr);
            if (shader->uploadToGPU()) {
                glUseProgramStages(
                    _handle,
                    shader->stageMask(),
                    shader->getProgramHandle());
                _stageMask |= shader->stageMask();
            }
        }

        registerShaderProgram(std::dynamic_pointer_cast<ShaderProgram>(shared_from_this()).get());
    }

    if (!_validated) {
        _validationQueued = true;
    }
}

void glShaderProgram::validatePostBind() {
    // If we haven't validated the program but used it at lease once ...
    if (_validationQueued && isValid()) {
        // clear validation queue flag
        _validationQueued = false;

        // Call the internal validation function
        if (Config::ENABLE_GPU_VALIDATION) {
            glValidateProgramPipeline(_handle);

            GLint status = 0;
            if (_stageMask != UseProgramStageMask::GL_COMPUTE_SHADER_BIT) {
                glGetProgramPipelineiv(_handle, GL_VALIDATE_STATUS, &status);
            } else {
                status = 1;
            }

            // we print errors in debug and in release, but everything else only in debug
            // the validation log is only retrieved if we request it. (i.e. in release,
            // if the shader is validated, it isn't retrieved)
            if (status == 0) {
                // Query the size of the log
                GLint length = 0;
                glGetProgramPipelineiv(_handle, GL_INFO_LOG_LENGTH, &length);

                // If we actually have something in the validation log
                if (length > 1) {
                    stringImpl validationBuffer = "";
                    validationBuffer.resize(length);
                    glGetProgramPipelineInfoLog(_handle, length, NULL, &validationBuffer[0]);

                    // To avoid overflowing the output buffers (both CEGUI and Console), limit the maximum output size
                    if (validationBuffer.size() > g_validationBufferMaxSize) {
                        // On some systems, the program's disassembly is printed, and that can get quite large
                        validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
                        // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
                        validationBuffer.append(" ... ");
                    }
                    // Return the final message, whatever it may contain
                    Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")), _handle, resourceName().c_str(), validationBuffer.c_str());
                } else {
                    Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")), _handle, resourceName().c_str(), "[ Couldn't retrieve info log! ]");
                }
            }
        }
        _validated = true;

        for (glShader* shader : _shaderStage) {
            assert(shader != nullptr);
            shader->dumpBinary();
        }
    }
}

/// This should be called in the loading thread, but some issues are still present, and it's not recommended (yet)
void glShaderProgram::threadedLoad(bool skipRegister) {
    reloadShaders(skipRegister);
    
    // Pass the rest of the loading steps to the parent class
    if (!skipRegister) {
        ShaderProgram::load();
    }
}

vector<stringImpl> glShaderProgram::loadSourceCode(ShaderType stage,
                                                   const stringImpl& stageName,
                                                   const stringImpl& extension,
                                                   const stringImpl& header,
                                                   U32 lineOffset,
                                                   bool forceReParse,
                                                   std::pair<bool, stringImpl>& sourceCodeOut) {

    vector<stringImpl> atoms = {};

    sourceCodeOut.first = false;
    sourceCodeOut.second.resize(0);

    stringImpl fileName = stageName;
    if (!header.empty()) {
        fileName.append("." + to_stringImpl(_ID(header.c_str())));
    }
    fileName.append("." + extension);

    if (s_useShaderTextCache && !forceReParse) {
        shaderFileRead(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText,
                       fileName,
                       sourceCodeOut.second);
    }

    if (sourceCodeOut.second.empty()) {
        sourceCodeOut.first = true;
        // Use GLSW to read the appropriate part of the effect file
        // based on the specified stage and properties
        const char* sourceCodeStr = glswGetShader(stageName.c_str());
        sourceCodeOut.second = sourceCodeStr ? sourceCodeStr : "";
        // GLSW may fail for various reasons (not a valid effect stage, invalid name, etc)
        if (!sourceCodeOut.second.empty()) {
            // And replace in place with our program's headers created earlier
            Util::ReplaceStringInPlace(sourceCodeOut.second, "//__CUSTOM_DEFINES__", header);
            Util::ReplaceStringInPlace(sourceCodeOut.second, "//__LINE_OFFSET_", Util::StringFormat("#line %d", lineOffset));
        }
        vector<stringImpl> foundDefines;
        foundDefines.reserve(20);
        sourceCodeOut.second = preprocessIncludes(resourceName(), sourceCodeOut.second, 0, atoms, foundDefines, true);

        shaderFileWrite(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText,
                        fileName,
                        sourceCodeOut.second.c_str());
    }

    return atoms;
}

/// Creation of a new shader piepline. Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::load() {
    // NULL shader means use shaderProgram(0), so bypass the normal loading routine
    if (resourceName().compare("NULL") == 0) {
        _validationQueued = false;
        _handle = 0;
        return ShaderProgram::load();
    }

    if (_asyncLoad) {
        Start(*CreateTask(_context.context().taskPool(TaskPoolType::HIGH_PRIORITY),
            [this](const Task & parent) {
                threadedLoad(false);
            },
            ("Shader load task [ " + resourceName() + " ]").c_str()));
    } else {
        threadedLoad(false);
    }

    return true;
}

bool glShaderProgram::reloadShaders(bool reparseShaderSource) {
    for (glShader* shader : _shaderStage) {
        glShader::removeShader(shader);
    }
    _shaderStage.clear();

    //glswClearCurrentContext();
    glswSetPath((assetLocation() + "/" + Paths::Shaders::GLSL::g_parentShaderLoc).c_str(), ".glsl");

    U64 batchCounter = 0;
    hashMap<U64, vectorEASTL<ShaderModuleDescriptor>> modulesByFile;
    for (const ShaderModuleDescriptor& shaderDescriptor : _descriptor._modules) {
        const U64 fileHash = shaderDescriptor._batchSameFile ? _ID(shaderDescriptor._sourceFile.c_str()) : batchCounter++;
        vectorEASTL<ShaderModuleDescriptor>& modules = modulesByFile[fileHash];
        modules.push_back(shaderDescriptor);
    }

    for (auto it : modulesByFile) {
        const vectorEASTL<ShaderModuleDescriptor>& modules = it.second;
        assert(!modules.empty());

        glShader::ShaderLoadData loadData;
        stringImpl programName = modules.front()._sourceFile;
        programName = programName.substr(0, programName.find_first_of(".,"));

        for (const ShaderModuleDescriptor& shaderDescriptor : modules) {
            const ShaderType type = shaderDescriptor._moduleType;
            assert(type != ShaderType::COUNT);

            const U8 shaderIdx = to_U8(type);

            programName.append("-" + GLUtil::glShaderStageNameTable[shaderIdx]);
            if (!shaderDescriptor._variant.empty()) {
                programName.append("." + shaderDescriptor._variant);
            }
            if (!shaderDescriptor._defines.empty()) {
                programName.append("." + ShaderProgram::getDefinesHash(shaderDescriptor._defines));
            }

            stringImpl header = "";
            for (auto define : shaderDescriptor._defines) {
                // Placeholders are ignored
                if (define.first.compare("DEFINE_PLACEHOLDER") != 0) {
                    // We manually add define dressing if needed
                    header.append((define.second ? "#define " : "") + define.first + "\n");
                }
            }

            glShader::LoadData& stageData = loadData[shaderIdx];
            stageData._type = shaderDescriptor._moduleType;
            stageData._name = shaderDescriptor._sourceFile.substr(0, shaderDescriptor._sourceFile.find_first_of(".,"));
            stageData._name.append("." + GLUtil::glShaderStageNameTable[shaderIdx]);
            if (!shaderDescriptor._variant.empty()) {
                stageData._name.append("." + shaderDescriptor._variant);
            }

            std::pair<bool, stringImpl> sourceCode;
            vector<stringImpl> atomsTemp = loadSourceCode(type, stageData._name, shaderAtomExtensionName[shaderIdx], header, _lineOffset[shaderIdx] + to_U32(shaderDescriptor._defines.size()), reparseShaderSource, sourceCode);
            stageData.atoms.insert(_ID(shaderDescriptor._sourceFile.c_str()));
            for (auto atomIt : atomsTemp) {
                stageData.atoms.insert(_ID(atomIt.c_str()));
            }
            stageData.sourceCode.push_back(sourceCode.second);
        }

        glShader* shader = glShader::getShader(programName);
        if (!shader) {
            shader = glShader::loadShader(_context, programName, loadData);
            assert(shader != nullptr);
        } else if (!reparseShaderSource) {
            shader->AddRef();
            Console::d_printfn(Locale::get(_ID("SHADER_MANAGER_GET_INC")), shader->name().c_str(), shader->GetRef());
        }
        if (!reparseShaderSource) {
            _shaderStage.push_back(shader);
        }
    }

    return !_shaderStage.empty();
}

bool glShaderProgram::shouldRecompile() const {
    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        if (shader->shouldRecompile()) {
            return true;
        }
    }

    return false;
}

bool glShaderProgram::recompileInternal() {
    // Remember bind state
    bool wasBound = isBound();
    if (wasBound) {
        GL_API::getStateTracker().setActivePipeline(0u);
    }

    if (resourceName().compare("NULL") == 0) {
        _validationQueued = false;
        _handle = 0;
        return true;
    }

    threadedLoad(true);
    // Restore bind state
    if (wasBound) {
        bind(wasBound);
    }
    return getState() == ResourceState::RES_LOADED;
}

/// Check every possible combination of flags to make sure this program can be used for rendering
bool glShaderProgram::isValid() const {
    // null shader is a valid shader
    return _handle != GLUtil::_invalidObjectID;
}

bool glShaderProgram::isBound() const {
    return GL_API::s_activeStateTracker->_activeShaderPipeline == _handle;
}

/// Bind this shader program
bool glShaderProgram::bind(bool& wasBound) {
    validatePreBind();
    // If the shader isn't ready or failed to link, stop here
    if (!isValid()) {
        return false;
    }
    // Set this program as the currently active one
    wasBound = GL_API::getStateTracker().setActivePipeline(_handle);
    validatePostBind();

    return true;
}

/// This is used to set all of the subroutine indices for the specified shader stage for this program
void glShaderProgram::SetSubroutines(ShaderType type, const vector<U32>& indices) const {
    // The shader must be bound before calling this!
    DIVIDE_ASSERT(isBound() && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");
    // Validate data and send to GPU
    if (!indices.empty() && indices[0] != GLUtil::_invalidObjectID) {
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_U32(type)], (GLsizei)indices.size(), indices.data());
    }
}

/// This works exactly like SetSubroutines, but for a single index.
/// If the shader has multiple subroutine uniforms, this will reset the rest!!!
void glShaderProgram::SetSubroutine(ShaderType type, U32 index) const {
    DIVIDE_ASSERT(isBound() && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");

    if (index != GLUtil::_invalidObjectID) {
        U32 value[] = {index};
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_U32(type)], 1, value);
    }
}

/// Returns the number of subroutine uniforms for the specified shader stage
U32 glShaderProgram::GetSubroutineUniformCount(ShaderType type) {
    I32 subroutineCount = 0;

    validatePreBind();
    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        if (shader->isValid() && shader->embedsType(type)) {
            glGetProgramStageiv(shader->getProgramHandle(), GLUtil::glShaderStageTable[to_U32(type)], GL_ACTIVE_SUBROUTINE_UNIFORMS, &subroutineCount);
            break;
        }
    }

    return std::max(subroutineCount, 0);
}

/// Get the index of the specified subroutine name for the specified stage. Not cached!
U32 glShaderProgram::GetSubroutineIndex(ShaderType type, const char* name) {

    validatePreBind();
    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        if (shader->isValid() && shader->embedsType(type)) {
            return glGetSubroutineIndex(shader->getProgramHandle(), GLUtil::glShaderStageTable[to_U32(type)], name);
        }
    }

    return 0u;
}


void glShaderProgram::UploadPushConstant(const GFX::PushConstant& constant) {
    assert(isValid());

    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        if (shader->isValid()) {
            shader->UploadPushConstant(constant);
        }
    }
}

void glShaderProgram::UploadPushConstants(const PushConstants& constants) {
    const vectorEASTL<GFX::PushConstant>& data = constants.data();

    for (const GFX::PushConstant& constant : data) {
        UploadPushConstant(constant);
    }
}

stringImpl glShaderProgram::preprocessIncludes(const stringImpl& name,
                                               const stringImpl& source,
                                               GLint level,
                                               vector<stringImpl>& foundAtoms,
                                               vector<stringImpl>& foundDefines,
                                               bool lock) {
    if (level > 32) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_INCLUD_LIMIT")));
    }

    size_t line_number = 1;
    std::smatch matches;

    stringImpl output, line;
    stringImpl include_file, include_string;

    istringstreamImpl input(source);

    while (std::getline(input, line)) {
        if (!std::regex_search(line, matches, Paths::g_includePattern)) {
            if (std::regex_search(line, matches, Paths::g_definePattern)) {
                foundDefines.push_back(Util::Trim(matches[1].str()));
            }
            output.append(line);
        } else {
            include_file = Util::Trim(matches[1].str().c_str());
            foundAtoms.push_back(include_file);

            ShaderType typeIndex = ShaderType::COUNT;
            bool found = false;
            // switch will throw warnings due to promotion to int
            U64 extHash = _ID(Util::GetTrailingCharacters(include_file, 4).c_str());
            for (U8 i = 0; i < to_base(ShaderType::COUNT) + 1; ++i) {
                if (extHash == shaderAtomExtensionHash[i]) {
                    typeIndex = static_cast<ShaderType>(i);
                    found = true;
                    break;
                }
            }

            DIVIDE_ASSERT(found, "Invalid shader include type");
            bool wasParsed = false;
            if (lock) {
                include_string = glShaderProgram::shaderFileRead(shaderAtomLocationPrefix[to_U32(typeIndex)], include_file, true, level, foundAtoms, wasParsed);
            } else {
                include_string = glShaderProgram::shaderFileReadLocked(shaderAtomLocationPrefix[to_U32(typeIndex)], include_file, true, level, foundAtoms, wasParsed);
            }
            if (include_string.empty()) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_NO_INCLUDE_FILE")),
                    name.c_str(),
                    line_number,
                    include_file.c_str());
            }
            if (wasParsed) {
                output.append(include_string);
            } else {
                output.append(preprocessIncludes(name, include_string, level + 1, foundAtoms, foundDefines, lock));
            }
        }

        output.append("\n");
        ++line_number;
    }

    return output;
}

const stringImpl& glShaderProgram::shaderFileRead(const stringImpl& filePath, const stringImpl& atomName, bool recurse, U32 level, vector<stringImpl>& foundAtoms, bool& wasParsed) {
    UniqueLockShared w_lock(s_atomLock);
    return shaderFileReadLocked(filePath, atomName, recurse, level, foundAtoms, wasParsed);
}

/// Open the file found at 'filePath' matching 'atomName' and return it's source code
const stringImpl& glShaderProgram::shaderFileReadLocked(const stringImpl& filePath, const stringImpl& atomName, bool recurse, U32 level, vector<stringImpl>& foundAtoms, bool& wasParsed) {
    U64 atomNameHash = _ID(atomName.c_str());
    // See if the atom was previously loaded and still in cache
    AtomMap::iterator it = s_atoms.find(atomNameHash);
    // If that's the case, return the code from cache
    if (it != std::cend(s_atoms)) {
        wasParsed = true;
        return it->second;
    }

    wasParsed = false;
    // If we forgot to specify an atom location, we have nothing to return
    assert(!filePath.empty());

    // Open the atom file and add the code to the atom cache for future reference
    stringImpl output;
    readFile(filePath, atomName, output, FileType::TEXT);

    if (recurse) {
        vector<stringImpl> foundDefines;
        foundDefines.reserve(20);
        output = preprocessIncludes(atomName, output, 0, foundAtoms, foundDefines, false);
    }

    auto result = s_atoms.insert({ atomNameHash, output });
    assert(result.second);

    // Return the source code
    return result.first->second;
}

stringImpl glShaderProgram::decorateFileName(const stringImpl& name) {
    if (Config::Build::IS_DEBUG_BUILD) {
        return "DEBUG." + name;
    } else if (Config::Build::IS_PROFILE_BUILD) {
        return "PROFILE." + name;
    }

    return "RELEASE." + name;
}

void glShaderProgram::shaderFileRead(const stringImpl& filePath, const stringImpl& fileName, stringImpl& sourceCodeOut) {
    readFile(filePath, decorateFileName(fileName), sourceCodeOut, FileType::TEXT);
}

/// Dump the source code 's' of atom file 'atomName' to file
void glShaderProgram::shaderFileWrite(const stringImpl& filePath, const stringImpl& fileName, const char* sourceCode) {
    writeFile(filePath, decorateFileName(fileName), (bufferPtr)sourceCode, strlen(sourceCode), FileType::TEXT);
}

void glShaderProgram::onAtomChange(const char* atomName, FileUpdateEvent evt) {
    // Do nothing if the specified file is "deleted". We do not want to break running programs
    if (evt == FileUpdateEvent::DELETE) {
        return;
    }

    const U64 atomNameHash = _ID(atomName);

    // ADD and MODIFY events should get processed as usual
    {
        // Clear the atom from the cache
        UniqueLockShared w_lock(s_atomLock);
        AtomMap::iterator it = s_atoms.find(atomNameHash);
        if (it != std::cend(s_atoms)) {
            it = s_atoms.erase(it);
        }
    }

    //Get list of shader programs that use the atom and rebuild all shaders in list;
    SharedLock r_lock(s_programLock);
    for (auto it : s_shaderPrograms) {
        ShaderProgram* programPtr = it.second.first;
        glShaderProgram* shaderProgram = static_cast<glShaderProgram*>(programPtr);

        bool skip = false;
        for (glShader* shader : shaderProgram->_shaderStage) {
            if (skip) {
                break;
            }

            assert(shader != nullptr);
            for (U64 atomHash : shader->_usedAtoms) {
                if (atomHash == atomNameHash) {
                    s_recompileQueue.push(programPtr);
                    skip = true;
                    break;
                }
            }
        }
    }
}

};
