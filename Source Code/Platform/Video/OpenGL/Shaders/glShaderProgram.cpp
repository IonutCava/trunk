#include "stdafx.h"

#include "config.h"

#include "Headers/glShaderProgram.h"

#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    size_t g_validationBufferMaxSize = 4096 * 16;
};

std::array<U32, to_base(ShaderType::COUNT)> glShaderProgram::_lineOffset;

IMPLEMENT_CUSTOM_ALLOCATOR(glShaderProgram, 0, 0);

void glShaderProgram::initStaticData() {
    glShader::initStaticData();
}

void glShaderProgram::destroyStaticData() {
    glShader::destroyStaticData();
}

glShaderProgram::glShaderProgram(GFXDevice& context,
                                 size_t descriptorHash,
                                 const stringImpl& name,
                                 const stringImpl& resourceName,
                                 const stringImpl& resourceLocation,
                                 bool asyncLoad)
    : ShaderProgram(context, descriptorHash, name, resourceName, resourceLocation, asyncLoad),
      glObject(glObjectType::TYPE_SHADER_PROGRAM, context),
      _loadedFromBinary(false),
      _validated(false),
      _shaderProgramIDTemp(0),
      _lockManager(MemoryManager_NEW glLockManager()),
      _binaryFormat(GL_NONE)
{
    _validationQueued = false;
    // each API has it's own invalid id. This is OpenGL's
    _shaderProgramID = GLUtil::_invalidObjectID;
    // pointers to all of our shader stages
    _shaderStage.fill(nullptr);
}

glShaderProgram::~glShaderProgram() {
    if (_lockManager) {
        _lockManager->Wait(true);
        MemoryManager::DELETE(_lockManager);
    }

    // delete shader program
    if (_shaderProgramID > 0 && _shaderProgramID != GLUtil::_invalidObjectID) {
        glDeleteProgram(_shaderProgramID);
        // glDeleteProgramPipelines(1, &_shaderProgramID);
    }
}

bool glShaderProgram::unload() {
    // Remove every shader attached to this program
    for (glShader* shader : _shaderStage) {
        if (shader) {
            glShader::removeShader(shader);
        }
    }

    return ShaderProgram::unload();
}

/// Basic OpenGL shader program validation (both in debug and in release)
bool glShaderProgram::validateInternal() {
    bool shaderError = false;
    for (glShader* shader : _shaderStage) {
        // If a shader exists for said stage
        if (shader) {
            // Validate it
            if (!shader->validate()) {
                shaderError = true;
            }
        }
    }

    GLint status = 0;
    glValidateProgram(_shaderProgramID);
    glGetProgramiv(_shaderProgramID, GL_VALIDATE_STATUS, &status);
    // glValidateProgramPipeline(_shaderProgramID);
    // glGetProgramPipelineiv(_shaderProgramID, GL_VALIDATE_STATUS, &status);
    // we print errors in debug and in release, but everything else only in
    // debug
    // the validation log is only retrieved if we request it. (i.e. in release,
    // if the shader is validated, it isn't retrieved)
    if (status == 0) {
        Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")),
                         _shaderProgramID, getName().c_str(), getLog().c_str());
        shaderError = true;
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")),
                           _shaderProgramID, getName().c_str(), getLog().c_str());
    }
    _validated = true;

    return shaderError;
}

/// Called once per frame. Used to update internal state
bool glShaderProgram::update(const U64 deltaTime) {
    // If we haven't validated the program but used it at lease once ...
    if (_validationQueued && _shaderProgramID != 0) {
        if (_lockManager) {
            _lockManager->Wait(true);
            MemoryManager::DELETE(_lockManager);
        }
        // Call the internal validation function
        validateInternal();

        // We dump the shader binary only if it wasn't loaded from one
        if (!_loadedFromBinary && GFXDevice::getGPUVendor() == GPUVendor::NVIDIA && false) {
            STUBBED(
                "GLSL binary dump/load is only enabled for nVidia GPUS. "
                "Catalyst 13.x  - 15.x destroys uniforms on shader dump, for whatever "
                "reason. - Ionut")
            // Get the size of the binary code
            GLint binaryLength = 0;
            glGetProgramiv(_shaderProgramID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
            // allocate a big enough buffer to hold it
            char* binary = MemoryManager_NEW char[binaryLength];
            DIVIDE_ASSERT(binary != NULL,
                          "glShaderProgram error: could not allocate memory "
                          "for the program binary!");
            // and fill the buffer with the binary code
            glGetProgramBinary(_shaderProgramID, binaryLength, NULL, &_binaryFormat, binary);
            if (_binaryFormat != GL_NONE) {
                // dump the buffer to file
                stringImpl outFileName(Paths::Shaders::g_cacheLocationBin + getName() + ".bin");
                writeFile(outFileName, binary, (size_t)binaryLength, FileType::BINARY);
                // dump the format to a separate file (highly non-optimised. Should dump formats to a database instead)
                outFileName += ".fmt";
                stringImpl binaryFormatStr = to_stringImpl(to_U32(_binaryFormat));
                writeFile(outFileName, (bufferPtr)binaryFormatStr.c_str(), binaryFormatStr.size(), FileType::BINARY);
             }
            // delete our local code buffer
            MemoryManager::DELETE(binary);
        }
        // clear validation queue flag
        _validationQueued = false;
    }

    // pass the update responsibility back to the parent class
    return ShaderProgram::update(deltaTime);
}

/// Retrieve the program's validation log if we need it
stringImpl glShaderProgram::getLog() const {
    // We default to a simple OK message if the log is empty (hopefully, that
    // means validation was successful, nVidia ... )
    stringImpl validationBuffer("[OK]");
    // Query the size of the log
    GLint length = 0;
    glGetProgramiv(_shaderProgramIDTemp, GL_INFO_LOG_LENGTH, &length);
    // glGetProgramPipelineiv(_shaderProgramIDTemp, GL_INFO_LOG_LENGTH,
    // &length);
    // If we actually have something in the validation log
    if (length > 1) {
        // Delete our OK string, and start on a new line
        validationBuffer = "\n -- ";
        // This little trick avoids a "NEW/DELETE" set of calls and still gives
        // us a linear array of char's
        vectorImpl<char> shaderProgramLog(length);
        glGetProgramInfoLog(_shaderProgramIDTemp, length, NULL,
                            &shaderProgramLog[0]);
        // glGetProgramPipelineInfoLog(_shaderProgramIDTemp, length, NULL,
        //                    &shaderProgramLog[0]);
        // Append the program's log to the output message
        validationBuffer.append(&shaderProgramLog[0]);
        // To avoid overflowing the output buffers (both CEGUI and Console),
        // limit the maximum output size
        if (validationBuffer.size() > g_validationBufferMaxSize) {
            // On some systems, the program's disassembly is printed, and that can get quite large
            validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
            // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
            validationBuffer.append(" ... ");
        }
    }
    // Return the final message, whatever it may contain
    return validationBuffer;
}

/// Remove a shader stage from this program
void glShaderProgram::detachShader(glShader* const shader) {
    if (!shader) {
        return;
    }

    glDetachShader(_shaderProgramID == GLUtil::_invalidObjectID
                                     ? _shaderProgramIDTemp
                                     : _shaderProgramID,
                   shader->getShaderID());
    // glUseProgramStages(_shaderProgramID,
    // GLUtil::glShaderStageTable[to_U32(shader->getType())], 0);
}

/// Add a new shader stage to this program
void glShaderProgram::attachShader(glShader* const shader) {
    if (!shader) {
        return;
    }
    // Attach the shader
    glAttachShader(_shaderProgramID == GLUtil::_invalidObjectID
                                     ? _shaderProgramIDTemp
                                     : _shaderProgramID,
                   shader->getShaderID());
    // glUseProgramStages(_shaderProgramIDTemp,
    // GLUtil::glShaderStageTable[to_U32(shader->getType())], shaderID);
    // Clear the 'linked' flag. Program must be re-linked before usage
    _linked = false;
}

/// This should be called in the loading thread, but some issues are still
/// present, and it's not recommended (yet)
void glShaderProgram::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback, bool skipRegister) {
    if (_asyncLoad) {
        GL_API::createOrValidateContextForCurrentThread(_context);
    }

    // Loading from binary gives us a linked program ready for usage.
    if (!_loadedFromBinary) {
        // If this wasn't loaded from binary, we need a new API specific object
        // If we try to refresh the program, we already have a handle
        if (_shaderProgramID == GLUtil::_invalidObjectID) {
            _shaderProgramIDTemp = glCreateProgram();
            // glCreateProgramPipelines(1, &_shaderProgramIDTemp);
        } else {
            _shaderProgramIDTemp = _shaderProgramID;
        }
        // For every possible stage that the program might use
        for (glShader* shader : _shaderStage) {
            // If a shader exists for said stage
            if (shader) {
                // Attach it
                attachShader(shader);
            }
        }
        // Link the program
        link();
    }
    // This was once an atomic swap. Might still be in the future
    _shaderProgramID = _shaderProgramIDTemp;
    // Pass the rest of the loading steps to the parent class
    if (_lockManager) {
        _lockManager->Lock();
    }

    if (!skipRegister) {
        ShaderProgram::load(onLoadCallback);
    } else {
        reuploadUniforms();
    }
}

/// Linking a shader program also sets up all pre-link properties for the shader
/// (varying locations, attrib bindings, etc)
void glShaderProgram::link() {
    if (!Config::Build::IS_DEBUG_BUILD) {
        // Loading from binary is optional, but it using it does require sending the
        // driver a hint to give us access to it later
        if (Config::USE_SHADER_BINARY) {
            glProgramParameteri(_shaderProgramIDTemp, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, 1);
        }
    }

    Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM")), getName().c_str(),
                       _shaderProgramIDTemp);

    /*glProgramParameteri(_shaderProgramIDTemp,
                        GL_PROGRAM_SEPARABLE,
                        1);
    */
    // Link the program
    glLinkProgram(_shaderProgramIDTemp);
    // Detach shaders after link so that the driver might free up some memory
    // remove shader stages
    for (glShader* shader : _shaderStage) {
        detachShader(shader);
    }

    _shaderVarLocation.clear();

    // And check the result
    GLint linkStatus = 0;
    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &linkStatus);
    // If linking failed, show an error, else print the result in debug builds.
    // Same getLog() method is used
    if (linkStatus == 0) {
        Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")),
                         getName().c_str(), getLog().c_str());
        if (_lockManager) {
            MemoryManager::DELETE(_lockManager);
        }
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")),
                           getName().c_str(), getLog().c_str());
        // The linked flag is set to true only if linking succeeded
        _linked = true;
        if (Config::ENABLE_GPU_VALIDATION) {
            glObjectLabel(GL_PROGRAM, _shaderProgramIDTemp, -1, getName().c_str());
        }
    }
}

bool glShaderProgram::loadFromBinary() {
    _loadedFromBinary = false;

    if (!Config::Build::IS_DEBUG_BUILD) {
        // Load the program from the binary file, if available and allowed, to avoid linking.
        if (Config::USE_SHADER_BINARY && false) {
            // Only available for new programs
            assert(_shaderProgramIDTemp == 0);
            stringImpl fileName(Paths::Shaders::g_cacheLocationBin + _resourceName + ".bin");
            // Load the program's binary format from file
            vectorImpl<Byte> data;
            bool fmtExists = readFile(fileName + ".fmt", data, FileType::BINARY);
            if (fmtExists) {
                _binaryFormat = *((GLenum const *)&data[0]);
            }

            if (fmtExists && _binaryFormat != GL_ZERO/* && _binaryFormat != GL_NONE*/) {
                data.clear();
                readFile(fileName, data, FileType::BINARY);
                   // Allocate a new handle
                _shaderProgramIDTemp = glCreateProgram();
                // glCreateProgramPipelines(1, &_shaderProgramIDTemp);
                // Load binary code on the GPU
                glProgramBinary(_shaderProgramIDTemp, _binaryFormat, data.data(), (GLint)data.size());
                // Check if the program linked successfully on load
                GLint success = 0;
                glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &success);
                // If it loaded properly set all appropriate flags (this also
                // prevents low level access to the program's shaders)
                if (success == 1) {
                    _loadedFromBinary = _linked = true;
                }
            }
        }
    }

    return _loadedFromBinary;
}

glShaderProgram::glShaderProgramLoadInfo glShaderProgram::buildLoadInfo() {
    glShaderProgramLoadInfo loadInfo;

    loadInfo._resourcePath = getResourceLocation() + "/" + Paths::Shaders::GLSL::g_parentShaderLoc;
    // Get all of the preprocessor defines and add them to the general shader header for this program
    for (U8 i = 0; i < _definesList.size(); ++i) {
        // Placeholders are ignored
        if (_definesList[i].compare("DEFINE_PLACEHOLDER") == 0) {
            continue;
        }
        // We manually add define dressing
        loadInfo._header.append("#define " + _definesList[i] + "\n");
    }

    // Split the shader name to get the effect file name and the effect properties
    // The effect file name is the part up until the first period or comma symbol
    loadInfo._programName = _resourceName.substr(0, _resourceName.find_first_of(".,"));
    // We also differentiate between general properties, and vertex properties
    // Get the position of the first "," symbol. Must be added at the end of the program's name!!
    stringAlg::stringSize propPositionVertex = _resourceName.find_first_of(",");
    // Get the position of the first "." symbol
    stringAlg::stringSize propPosition = _resourceName.find_first_of(".");
    // If we have effect properties, we extract them from the name
    // (starting from the first "." symbol to the first "," symbol)
    if (propPosition != stringImpl::npos) {
        loadInfo._programProperties = "." +
            _resourceName.substr(propPosition + 1,
                propPositionVertex - propPosition - 1);
    }
    // Vertex properties start off identically to the rest of the stages' names
    loadInfo._vertexStageProperties = loadInfo._programProperties;
    // But we also add the shader specific properties
    if (propPositionVertex != stringImpl::npos) {
        loadInfo._vertexStageProperties += "." + _resourceName.substr(propPositionVertex + 1);
    }

    return loadInfo;
}

std::pair<bool, stringImpl> glShaderProgram::loadSourceCode(ShaderType stage,
                                                            const stringImpl& stageName,
                                                            const stringImpl& header,
                                                            bool forceReParse) {
    std::pair<bool, stringImpl> sourceCode;
    sourceCode.first = false;

    if (Config::USE_SHADER_TEXT_CACHE && !forceReParse) {
        if (Config::ENABLE_GPU_VALIDATION) {
            ShaderProgram::shaderFileRead(Paths::Shaders::g_cacheLocationText + stageName,
                                          true,
                                          sourceCode.second);
        }
    }

    if (sourceCode.second.empty()) {
        sourceCode.first = true;
        // Use GLSW to read the appropriate part of the effect file
        // based on the specified stage and properties
        const char* sourceCodeStr = glswGetShader(stageName.c_str());
        sourceCode.second = sourceCodeStr ? sourceCodeStr : "";
        // GLSW may fail for various reasons (not a valid effect stage, invalid name, etc)
        if (!sourceCode.second.empty()) {
            // And replace in place with our program's headers created earlier
            Util::ReplaceStringInPlace(sourceCode.second, "//__CUSTOM_DEFINES__", header);
        }
    }

    return sourceCode;
}

/// Creation of a new shader program. Pass in a shader token and use glsw to
/// load the corresponding effects
bool glShaderProgram::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    // NULL shader means use shaderProgram(0), so bypass the normal
    // loading routine
    if (_resourceName.compare("NULL") == 0) {
        _validationQueued = false;
        _shaderProgramID = 0;
        return ShaderProgram::load(onLoadCallback);
    }

    // Reset the linked status of the program
    _linked = false;

    if (!loadFromBinary()) {
        reloadShaders(false);
    }

    // try to link the program in a separate thread
    return _context.loadInContext(
        !_loadedFromBinary && _asyncLoad
            ? CurrentContext::GFX_LOADING_CTX
            : CurrentContext::GFX_RENDERING_CTX,
        [this, onLoadCallback](const Task& parent){
            threadedLoad(std::move(onLoadCallback), false);
        });
}

void glShaderProgram::reloadShaders(bool reparseShaderSource) {
    glShaderProgramLoadInfo info = buildLoadInfo();
    registerAtomFile(info._programName + ".glsl");

    glswClearCurrentContext();
    // The program wasn't loaded from binary, so process shaders
    // Use the specified shader path
    glswSetPath(info._resourcePath.c_str(), ".glsl");

    // For every stage
    for (U32 i = 0; i < to_base(ShaderType::COUNT); ++i) {
        // Brute force conversion to an enum
        ShaderType type = static_cast<ShaderType>(i);
        stringImpl shaderCompileName(info._programName +
                                     "." +
                                     GLUtil::glShaderStageNameTable[i] +
                                     info._vertexStageProperties);
        glShader*& shader = _shaderStage[i];

        // We ask the shader manager to see if it was previously loaded elsewhere
        shader = glShader::getShader(shaderCompileName);
        // If this is the first time this shader is loaded ...
        if (!shader || reparseShaderSource) {
            std::pair<bool, stringImpl> sourceCode = loadSourceCode(type, shaderCompileName, info._header, reparseShaderSource);
            if (!sourceCode.second.empty()) {
                // Load our shader from the final string and save it in the manager in case a new Shader Program needs it
                shader = glShader::loadShader(_context,
                                              shaderCompileName,
                                              sourceCode.second,
                                              type,
                                              sourceCode.first,
                                              _lineOffset[i] + to_U32(_definesList.size()) - 1);
                if (shader) {
                    // Try to compile the shader (it doesn't double compile shaders, so it's safe to call it multiple times)
                    if (!shader->compile()) {
                        Console::errorfn(Locale::get(_ID("ERROR_GLSL_COMPILE")), shader->getShaderID(), shaderCompileName.c_str());
                    }
                }
            }
        } else {
            shader->AddRef();
            Console::d_printfn(Locale::get(_ID("SHADER_MANAGER_GET_INC")), shader->getName().c_str(), shader->GetRef());
        }

        if (shader) {
            for (const stringImpl& atoms : shader->usedAtoms()) {
                registerAtomFile(atoms);
            }
        } else {
            Console::d_printfn(Locale::get(_ID("WARN_GLSL_LOAD")), shaderCompileName.c_str());
        }
    }
}

bool glShaderProgram::recompileInternal() {
    // Remember bind state
    bool wasBound = isBound();
    if (wasBound) {
        unbind();
    }

    if (_resourceName.compare("NULL") == 0) {
        _validationQueued = false;
        _shaderProgramID = 0;
        return true;
    }
    if (_lockManager) {
        _lockManager->Wait(true);
        MemoryManager::DELETE(_lockManager);
    }

    reloadShaders(true);
    threadedLoad(DELEGATE_CBK<void, Resource_wptr>(), true);
    // Restore bind state
    if (wasBound) {
        bind();
    }
    return getState() == ResourceState::RES_LOADED;
}

/// Check every possible combination of flags to make sure this program can be used for rendering
bool glShaderProgram::isValid() const {
    // null shader is a valid shader
    return _shaderProgramID == 0 ||
           (_linked && _shaderProgramID != GLUtil::_invalidObjectID);
}

bool glShaderProgram::isBound() const {
    return GL_API::s_activeShaderProgram == _shaderProgramID;
}

/// Cache uniform/attribute locations for shader programs
/// When we call this function, we check our name<->address map to see if we queried the location before
/// If we didn't, ask the GPU to give us the variables address and save it for later use
I32 glShaderProgram::Binding(const char* name) {
    // If the shader can't be used for rendering, just return an invalid address
    if (_shaderProgramID == 0 || !isValid()) {
        return -1;
    }

    // Check the cache for the location
    U64 nameHash = _ID_RT(name);
    ShaderVarMap::const_iterator it = _shaderVarLocation.find(nameHash);
    if (it != std::end(_shaderVarLocation)) {
        return it->second;
    }

    // Cache miss. Query OpenGL for the location
    GLint location = glGetUniformLocation(_shaderProgramID, name);

    // Save it for later reference
    hashAlg::insert(_shaderVarLocation, nameHash, location);

    // Return the location
    return location;
}

/// Bind the NULL shader which should have the same effect as using no shaders at all
bool glShaderProgram::unbind() {
    return static_cast<glShaderProgram*>(ShaderProgram::nullShader().get())->bind();
}

/// Bind this shader program
bool glShaderProgram::bind() {
    // If the shader isn't ready or failed to link, stop here
    if (!isValid()) {
        return false;
    }

    if (_lockManager) {
        _lockManager->Wait(true);
        MemoryManager::DELETE(_lockManager);
    }

    // Set this program as the currently active one
    GL_API::setActiveProgram(_shaderProgramID);
    // After using the shader at least once, validate the shader if needed
    if (!_validated) {
        _validationQueued = true;
    }
    return true;
}

/// This is used to set all of the subroutine indices for the specified shader
/// stage for this program
void glShaderProgram::SetSubroutines(ShaderType type,
                                     const vectorImpl<U32>& indices) const {
    // The shader must be bound before calling this!
    DIVIDE_ASSERT(isBound() && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");
    // Validate data and send to GPU
    if (!indices.empty() && indices[0] != GLUtil::_invalidObjectID) {
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_U32(type)],
                                (GLsizei)indices.size(), indices.data());
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
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_U32(type)], 1,
                                value);
    }
}

/// Returns the number of subroutine uniforms for the specified shader stage
U32 glShaderProgram::GetSubroutineUniformCount(ShaderType type) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    I32 subroutineCount = 0;
    glGetProgramStageiv(_shaderProgramID,
                        GLUtil::glShaderStageTable[to_U32(type)],
                        GL_ACTIVE_SUBROUTINE_UNIFORMS, &subroutineCount);

    return std::max(subroutineCount, 0);
}

/// Get the uniform location of the specified subroutine uniform for the
/// specified stage. Not cached!
U32 glShaderProgram::GetSubroutineUniformLocation(
    ShaderType type, const char* name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineUniformLocation(
        _shaderProgramID, GLUtil::glShaderStageTable[to_U32(type)],
        name);
}

/// Get the index of the specified subroutine name for the specified stage. Not cached!
U32 glShaderProgram::GetSubroutineIndex(ShaderType type, const char* name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineIndex(_shaderProgramID,
                                GLUtil::glShaderStageTable[to_U32(type)],
                                name);
}

I32 glShaderProgram::cachedValueUpdate(const PushConstant& constant) {
    const char* location = constant._binding.c_str();
    U64 locationHash = _ID_RT(location);

    I32 binding = Binding(location);

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByNameHash::ShaderVarMap::iterator it = _uniformsByNameHash._shaderVars.find(locationHash);
    if (it != std::end(_uniformsByNameHash._shaderVars)) {
        if (comparePushConstants(it->second, constant)) {
            return -1;
        } else {
            it->second = constant;
        }
    } else {
        hashAlg::emplace(_uniformsByNameHash._shaderVars, locationHash, constant);
    }

    return binding;
}

void glShaderProgram::reuploadUniforms() {
    for (UniformsByNameHash::ShaderVarMap::value_type it : _uniformsByNameHash._shaderVars) {
        UploadPushConstant(it.second);
    }
}

};
