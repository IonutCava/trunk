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
    constexpr U32 s_maxShaderUploadsPerFrame = 3;

    //ref: https://stackoverflow.com/questions/14858017/using-boost-wave
    class custom_directives_hooks : public boost::wave::context_policies::default_preprocessing_hooks
    {
    public:
        template <typename ContextT, typename ContainerT>
        bool found_unknown_directive(ContextT const& ctx, ContainerT const& line, ContainerT& pending)
        {
            namespace wave = boost::wave;

            using iterator_type = typename ContainerT::const_iterator;
            iterator_type it = line.begin();
            wave::token_id id = wave::util::impl::skip_whitespace(it, line.end());

            if (id != wave::T_IDENTIFIER) {
                return false;       // nothing we could do
            }

            if ((*it).get_value() == "version" || (*it).get_value() == "extension")
            {
                // handle #version and #extension directives
                std::copy(line.begin(), line.end(), std::back_inserter(pending));
                return true;
            }

            return false;           // unknown directive
        }
    };

    stringImpl preProcess(std::string input, const char* name) {
        using token_type = boost::wave::cpplexer::lex_token<>;
        using lex_iterator_type = boost::wave::cpplexer::lex_iterator<token_type>;
        using context_type=  boost::wave::context<std::string::iterator, lex_iterator_type,
                                                  boost::wave::iteration_context_policies::load_file_to_string,
                                                  custom_directives_hooks>;

        context_type ctx(input.begin(), input.end(), name);

        ctx.set_language(boost::wave::enable_long_long(ctx.get_language()));
        ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
        ctx.set_language(boost::wave::enable_prefer_pp_numbers(ctx.get_language()));
        ctx.set_language(boost::wave::enable_emit_line_directives(ctx.get_language(), false));

        context_type::iterator_type first = ctx.begin();
        context_type::iterator_type last = ctx.end();


        stringImpl ret = "";
        while (first != last) {
            ret.append((*first).get_value().c_str());
            ++first;
        }
        return ret.c_str();
    }
    size_t g_validationBufferMaxSize = 4096 * 16;
    UpdateListener s_fileWatcherListener([](const char* atomName, FileUpdateEvent evt) {
        glShaderProgram::onAtomChange(atomName, evt);
    });
};

SharedMutex glShaderProgram::s_atomLock;
ShaderProgram::AtomMap glShaderProgram::s_atoms;
I64 glShaderProgram::s_shaderFileWatcherID = -1;
GLuint glShaderProgram::s_shadersUploadedThisFrame = 0;
std::array<U32, to_base(ShaderType::COUNT)> glShaderProgram::_lineOffset;
Str256 glShaderProgram::shaderAtomLocationPrefix[to_base(ShaderType::COUNT) + 1];
U64 glShaderProgram::shaderAtomExtensionHash[to_base(ShaderType::COUNT) + 1];
Str8 glShaderProgram::shaderAtomExtensionName[to_base(ShaderType::COUNT) + 1];

void glShaderProgram::initStaticData() {
    const Str256 locPrefix = Paths::g_assetsLocation + Paths::g_shadersLocation + Paths::Shaders::GLSL::g_parentShaderLoc;

    shaderAtomLocationPrefix[to_base(ShaderType::FRAGMENT)] = locPrefix + Paths::Shaders::GLSL::g_fragAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::VERTEX)] = locPrefix + Paths::Shaders::GLSL::g_vertAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::GEOMETRY)] = locPrefix + Paths::Shaders::GLSL::g_geomAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::TESSELLATION_CTRL)] = locPrefix + Paths::Shaders::GLSL::g_tescAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::TESSELLATION_EVAL)] = locPrefix + Paths::Shaders::GLSL::g_teseAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::COMPUTE)] = locPrefix + Paths::Shaders::GLSL::g_compAtomLoc;
    shaderAtomLocationPrefix[to_base(ShaderType::COUNT)] = locPrefix + Paths::Shaders::GLSL::g_comnAtomLoc;

    shaderAtomExtensionName[to_base(ShaderType::FRAGMENT)] = Paths::Shaders::GLSL::g_fragAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::VERTEX)] = Paths::Shaders::GLSL::g_vertAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::GEOMETRY)] = Paths::Shaders::GLSL::g_geomAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::TESSELLATION_CTRL)] = Paths::Shaders::GLSL::g_tescAtomExt;
    shaderAtomExtensionName[to_base(ShaderType::TESSELLATION_EVAL)] = Paths::Shaders::GLSL::g_teseAtomExt;
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

        const vector<Str256> atomLocations = getAllAtomLocations();
        for (const Str256& loc : atomLocations) {
            createDirectories(loc.c_str());
            watcher().addWatch(loc.c_str(), &s_fileWatcherListener);
        }
    }
}

void glShaderProgram::onShutdown() {
    FileWatcherManager::deallocateWatcher(s_shaderFileWatcherID);
    s_shaderFileWatcherID = -1;
}

glShaderProgram::glShaderProgram(GFXDevice& context,
                                 size_t descriptorHash,
                                 const Str128& name,
                                 const Str128& assetName,
                                 const stringImpl& assetLocation,
                                 const ShaderProgramDescriptor& descriptor,
                                 bool asyncLoad)
    : ShaderProgram(context, descriptorHash, name, assetName, assetLocation, descriptor, asyncLoad),
      glObject(glObjectType::TYPE_SHADER_PROGRAM, context),
      _stageMask(UseProgramStageMask::GL_NONE_BIT),
      _validated(false),
      _validationQueued(false),
      _descriptor(descriptor),
      _handle(GLUtil::k_invalidObjectID)
{
}

glShaderProgram::~glShaderProgram()
{
    // delete shader program
#if 0
    assert(_shaderStage.empty());
#else
    unload();
#endif

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

bool glShaderProgram::rebindStages() {
    _stageMask = UseProgramStageMask::GL_NONE_BIT;
    for (glShader* shader : _shaderStage) {
        // If a shader exists for said stage, attach it
        assert(shader != nullptr);
        assert(_handle != GLUtil::k_invalidObjectID);

        bool previouslyUploaded = false;
        if (shader->uploadToGPU(previouslyUploaded)) {
            glUseProgramStages(
                _handle,
                shader->stageMask(),
                shader->getProgramHandle());
            _stageMask |= shader->stageMask();
            if (!previouslyUploaded && !_highPriority) {
                return false;
            }
        }
    }
    return true;
}

void glShaderProgram::validatePreBind() {
    if (!_highPriority && s_shadersUploadedThisFrame++ > s_maxShaderUploadsPerFrame) {
        //return;
    }

    if (!isValid()) {
        assert(getState() == ResourceState::RES_LOADED);
        glCreateProgramPipelines(1, &_handle);
        glObjectLabel(GL_PROGRAM_PIPELINE, _handle, -1, resourceName().c_str());
    }

    if (!_validated && rebindStages()) {
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
void glShaderProgram::threadedLoad(bool reloadExisting) {
    OPTICK_EVENT();

    if (!mWeakPtr.expired()) {
        registerShaderProgram(eastl::dynamic_pointer_cast<ShaderProgram>(shared_from_this()).get());
    }

    // NULL shader means use shaderProgram(0), so bypass the normal loading routine
    if (resourceName().compare("NULL") == 0) {
        _validationQueued = false;
        _handle = 0;
    } else {
        reloadShaders(reloadExisting);
    }
    // Pass the rest of the loading steps to the parent class
    if (!reloadExisting) {
        ShaderProgram::load();
    }
}

vector<Str64> glShaderProgram::loadSourceCode(ShaderType stage,
                                              const Str64& stageName,
                                              const Str8& extension,
                                              const stringImpl& header,
                                              U32 lineOffset,
                                              bool reloadExisting,
                                              std::pair<bool, stringImpl>& sourceCodeOut) {

    vector<Str64> atoms = {};

    sourceCodeOut.first = false;
    sourceCodeOut.second.resize(0);

    stringImpl fileName = stageName.c_str();
    if (!header.empty()) {
        fileName.append("." + to_stringImpl(_ID(header.c_str())));
    }
    fileName.append("." + extension);

    if (s_useShaderTextCache && !reloadExisting) {
        shaderFileRead((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText).c_str(),
                       fileName.c_str(),
                       sourceCodeOut.second);
    }

    if (sourceCodeOut.second.empty()) {
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

        stringImpl srcTemp = preprocessIncludes(resourceName(), sourceCodeOut.second, 0, atoms, true);
        if (!srcTemp.empty()) {
            sourceCodeOut.first = true;
            if (Config::Build::IS_DEBUG_BUILD) {
                sourceCodeOut.second = srcTemp;
            } else {
                sourceCodeOut.second = preProcess(srcTemp, fileName.c_str());
            }
            shaderFileWrite((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText).c_str(),
                            fileName.c_str(),
                            sourceCodeOut.second.c_str());
        }
    }

    return atoms;
}

/// Creation of a new shader piepline. Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::load() {
    if (_asyncLoad) {
        Start(*CreateTask(_context.context().taskPool(TaskPoolType::HIGH_PRIORITY),
            [this](const Task & parent) {
                threadedLoad(false);
            },
            Util::StringFormat("Shader load task [ %s ]", resourceName().c_str()).c_str()));
    } else {
        threadedLoad(false);
    }

    return true;
}

bool glShaderProgram::reloadShaders(bool reloadExisting) {
    //glswClearCurrentContext();
    glswSetPath((assetLocation() + "/" + Paths::Shaders::GLSL::g_parentShaderLoc.c_str()).c_str(), ".glsl");

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

        Str256 programName = modules.front()._sourceFile;
        programName = programName.substr(0, programName.find_first_of(".,"));

        bool hasData = false;
        for (const ShaderModuleDescriptor& shaderDescriptor : modules) {
            const ShaderType type = shaderDescriptor._moduleType;
            assert(type != ShaderType::COUNT);

            const U8 shaderIdx = to_U8(type);
            stringImpl header = "";
            for (auto define : shaderDescriptor._defines) {
                // Placeholders are ignored
                if (define.first.compare("DEFINE_PLACEHOLDER") != 0) {
                    // We manually add define dressing if needed
                    header.append((define.second ? "#define " : ""));
                    header.append((define.first + "\n").c_str());
                }
            }

            programName.append("-" + GLUtil::glShaderStageNameTable[shaderIdx]);
            if (!shaderDescriptor._variant.empty()) {
                programName.append("." + shaderDescriptor._variant);
            }
            if (!shaderDescriptor._defines.empty()) {
                programName.append(Util::StringFormat(".%zu", ShaderProgram::definesHash(shaderDescriptor._defines)));
            }

            glShader::LoadData& stageData = loadData[shaderIdx];
            stageData._type = shaderDescriptor._moduleType;
            stageData._name = shaderDescriptor._sourceFile.substr(0, shaderDescriptor._sourceFile.find_first_of(".,"));
            stageData._name.append("." + GLUtil::glShaderStageNameTable[shaderIdx]);
            if (!shaderDescriptor._variant.empty()) {
                stageData._name.append("." + shaderDescriptor._variant);
            }

            std::pair<bool, stringImpl> sourceCode;
            vector<Str64> atomsTemp = loadSourceCode(type, stageData._name, shaderAtomExtensionName[shaderIdx], header, _lineOffset[shaderIdx] + to_U32(shaderDescriptor._defines.size()), reloadExisting, sourceCode);
            if (!sourceCode.first) {
                continue;
            }
            stageData.atoms.insert(_ID(shaderDescriptor._sourceFile.c_str()));
            for (auto atomIt : atomsTemp) {
                stageData.atoms.insert(_ID(atomIt.c_str()));
            }
            stageData.sourceCode.push_back(sourceCode.second);
            hasData = true;
        }

        if (!hasData) {
            continue;
        }

        if (reloadExisting) {
            const U64 targetNameHash = _ID(programName.c_str());
            for (glShader* tempShader : _shaderStage) {
                if (tempShader->nameHash() == targetNameHash) {
                    assert(tempShader != nullptr);
                    glShader::loadShader(_context, tempShader, false, loadData);
                    break;
                }
            }
            rebindStages();
            for (glShader* shader : _shaderStage) {
                shader->reuploadUniforms(true);
            }
            _validated = false;
        } else {
            glShader* shader = glShader::getShader(programName);
            if (shader == nullptr) {
                shader = glShader::loadShader(_context, programName, loadData);
                assert(shader != nullptr);
            } else {
                shader->AddRef();
                Console::d_printfn(Locale::get(_ID("SHADER_MANAGER_GET_INC")), shader->name().c_str(), shader->GetRef());
            }

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

void glShaderProgram::update(const U64 deltaTimeUS) {
    ACKNOWLEDGE_UNUSED(deltaTimeUS);

    if (s_shadersUploadedThisFrame > 0u) {
        s_shadersUploadedThisFrame = 0u;
    }
}

bool glShaderProgram::recompileInternal(bool force) {
    // Invalid or not loaded yet
    if (_handle != GLUtil::k_invalidObjectID) {
        if (force || shouldRecompile()) {
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
        }

        return getState() == ResourceState::RES_LOADED;
    }

    return true;
}

/// Check every possible combination of flags to make sure this program can be used for rendering
bool glShaderProgram::isValid() const {
    // null shader is a valid shader
    return _handle != GLUtil::k_invalidObjectID;
}

bool glShaderProgram::isBound() const {
    return GL_API::getStateTracker()._activeShaderPipeline == _handle;
}

/// Bind this shader program
bool glShaderProgram::bind(bool& wasBound) {
    validatePreBind();
    // If the shader isn't ready or failed to link, stop here
    if (_validated || _validationQueued) {
        // Set this program as the currently active one
        wasBound = GL_API::getStateTracker().setActivePipeline(_handle);
        validatePostBind();
        return true;
    }
    return false;
}

/// This is used to set all of the subroutine indices for the specified shader stage for this program
void glShaderProgram::SetSubroutines(ShaderType type, const vector<U32>& indices) const {
    if (indices.empty()) {
        return;
    }

    // The shader must be bound before calling this!
    DIVIDE_ASSERT(isBound() && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");
    // Validate data and send to GPU
    if (indices[0] != GLUtil::k_invalidObjectID) {
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_U32(type)], (GLsizei)indices.size(), indices.data());
    }
}

/// This works exactly like SetSubroutines, but for a single index.
/// If the shader has multiple subroutine uniforms, this will reset the rest!!!
void glShaderProgram::SetSubroutine(ShaderType type, U32 index) const {
    DIVIDE_ASSERT(isBound() && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");

    if (index != GLUtil::k_invalidObjectID) {
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
        if (shader->valid() && shader->embedsType(type)) {
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
        if (shader->valid() && shader->embedsType(type)) {
            return glGetSubroutineIndex(shader->getProgramHandle(), GLUtil::glShaderStageTable[to_U32(type)], name);
        }
    }

    return 0u;
}


void glShaderProgram::UploadPushConstant(const GFX::PushConstant& constant) {
    assert(isValid());

    for (glShader* shader : _shaderStage) {
        assert(shader != nullptr);
        if (shader->valid()) {
            shader->UploadPushConstant(constant, false);
        }
    }
}

void glShaderProgram::UploadPushConstants(const PushConstants& constants) {
    const vectorEASTL<GFX::PushConstant>& data = constants.data();

    for (const GFX::PushConstant& constant : data) {
        UploadPushConstant(constant);
    }
}

stringImpl glShaderProgram::preprocessIncludes(const Str128& name,
                                               const stringImpl& source,
                                               GLint level,
                                               vector<Str64>& foundAtoms,
                                               bool lock) {
    if (level > 32) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_INCLUD_LIMIT")));
    }

    size_t line_number = 1;
    std::smatch matches;

    stringImpl output, line, include_string;
    Str64 include_file;

    istringstreamImpl input(source);

    while (std::getline(input, line)) {
        if (!std::regex_search(line, matches, Paths::g_includePattern)) {
            output.append(line);
        } else {
            include_file = Util::Trim(matches[1].str()).c_str();
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
                output.append(preprocessIncludes(name, include_string, level + 1, foundAtoms, lock));
            }
        }

        output.append("\n");
        ++line_number;
    }

    return output;
}

const stringImpl& glShaderProgram::shaderFileRead(const Str256& filePath, const Str64& atomName, bool recurse, U32 level, vector<Str64>& foundAtoms, bool& wasParsed) {
    UniqueLockShared w_lock(s_atomLock);
    return shaderFileReadLocked(filePath, atomName, recurse, level, foundAtoms, wasParsed);
}

/// Open the file found at 'filePath' matching 'atomName' and return it's source code
const stringImpl& glShaderProgram::shaderFileReadLocked(const Str256& filePath, const Str64& atomName, bool recurse, U32 level, vector<Str64>& foundAtoms, bool& wasParsed) {
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
    readFile(filePath.c_str(), atomName.c_str(), output, FileType::TEXT);

    if (recurse) {
        output = preprocessIncludes(atomName, output, 0, foundAtoms, false);
    }

    auto result = s_atoms.insert({ atomNameHash, output });
    assert(result.second);

    // Return the source code
    return result.first->second;
}

void glShaderProgram::shaderFileRead(const Str256& filePath, const Str128& fileName, stringImpl& sourceCodeOut) {
    readFile(filePath.c_str(), decorateFileName(fileName).c_str(), sourceCodeOut, FileType::TEXT);
}

/// Dump the source code 's' of atom file 'atomName' to file
void glShaderProgram::shaderFileWrite(const Str256& filePath, const Str128& fileName, const char* sourceCode) {
    writeFile(filePath.c_str(), decorateFileName(fileName).c_str(), (bufferPtr)sourceCode, strlen(sourceCode), FileType::TEXT);
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

        glShaderProgram* shaderProgram = static_cast<glShaderProgram*>(it.second.first);

        bool skip = false;
        for (glShader* shader : shaderProgram->_shaderStage) {
            if (skip) {
                break;
            }

            assert(shader != nullptr);
            for (U64 atomHash : shader->_usedAtoms) {
                if (atomHash == atomNameHash) {
                    s_recompileQueue.push(shaderProgram);
                    skip = true;
                    break;
                }
            }
        }
    }
}

};
