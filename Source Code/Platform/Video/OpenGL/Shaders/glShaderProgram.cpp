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
      _binaryFormat(GL_NONE),
      _stageMask(UseProgramStageMask::GL_NONE_BIT)
{
    _validationQueued = false;
    // each API has it's own invalid id. This is OpenGL's
    _shaderProgramID = GLUtil::_invalidObjectID;
    // pointers to all of our shader stages
    _shaderStage.fill(nullptr);
}

glShaderProgram::~glShaderProgram()
{
    if (_lockManager) {
        _lockManager->Wait(true);
        MemoryManager::DELETE(_lockManager);
    }

    if (isBound()) {
        unbind();
    }

    // delete shader program
    if (_shaderProgramID > 0 && _shaderProgramID != GLUtil::_invalidObjectID) {
        if (Config::USE_SEPARATE_SHADER_OBJECTS) {
            glDeleteProgramPipelines(1, &_shaderProgramID);
        } else {
            GL_API::deleteShaderPrograms(1, &_shaderProgramID);
        }
        
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

    if (Config::USE_SEPARATE_SHADER_OBJECTS) {
        glValidateProgramPipeline(_shaderProgramID);
        glGetProgramPipelineiv(_shaderProgramID, GL_VALIDATE_STATUS, &status);
    } else {
        glValidateProgram(_shaderProgramID);
        glGetProgramiv(_shaderProgramID, GL_VALIDATE_STATUS, &status);
    }
    
    // we print errors in debug and in release, but everything else only in debug
    // the validation log is only retrieved if we request it. (i.e. in release,
    // if the shader is validated, it isn't retrieved)
    if (status == 0) {
        Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")),
                         _shaderProgramID, name().c_str(), getLog().c_str());
        shaderError = true;
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")),
                           _shaderProgramID, name().c_str(), getLog().c_str());
    }
    _validated = true;

    return shaderError;
}

/// Called once per frame. Used to update internal state
bool glShaderProgram::update(const U64 deltaTimeUS) {
    // If we haven't validated the program but used it at lease once ...
    if (_validationQueued && _shaderProgramID != 0) {
        if (_lockManager) {
            _lockManager->Wait(true);
            MemoryManager::DELETE(_lockManager);
        }
        // Call the internal validation function
        validateInternal();

        // We dump the shader binary only if it wasn't loaded from one
        if (!_loadedFromBinary) {
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
                if (writeFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin,
                              name() + ".bin",
                              binary,
                              (size_t)binaryLength,
                              FileType::BINARY))
                {
                    // dump the format to a separate file (highly non-optimised. Should dump formats to a database instead)
                    writeFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin,
                              name() + ".fmt",
                              &_binaryFormat,
                              sizeof(GLenum),
                              FileType::BINARY);
                }
             }
            // delete our local code buffer
            MemoryManager::DELETE(binary);
        }
        // clear validation queue flag
        _validationQueued = false;
    }

    // pass the update responsibility back to the parent class
    return ShaderProgram::update(deltaTimeUS);
}

/// Retrieve the program's validation log if we need it
stringImpl glShaderProgram::getLog() const {
    // Query the size of the log
    GLint length = 0;

    if (Config::USE_SEPARATE_SHADER_OBJECTS) {
        glGetProgramPipelineiv(_shaderProgramIDTemp, GL_INFO_LOG_LENGTH, &length);
    } else {
        glGetProgramiv(_shaderProgramIDTemp, GL_INFO_LOG_LENGTH, &length);
    }

    // If we actually have something in the validation log
    if (length > 1) {
        stringImpl validationBuffer;
        validationBuffer.resize(length);
        if (Config::USE_SEPARATE_SHADER_OBJECTS) {
            glGetProgramPipelineInfoLog(_shaderProgramIDTemp, length, NULL,  &validationBuffer[0]);
        } else {
            glGetProgramInfoLog(_shaderProgramIDTemp, length, NULL, &validationBuffer[0]);
        }
        
        // To avoid overflowing the output buffers (both CEGUI and Console), limit the maximum output size
        if (validationBuffer.size() > g_validationBufferMaxSize) {
            // On some systems, the program's disassembly is printed, and that can get quite large
            validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
            // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
            validationBuffer.append(" ... ");
        }
        // Return the final message, whatever it may contain
        return validationBuffer;
    } else {
        return "[OK]";
    }
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

    _stageMask |= GLUtil::glProgramStageMask[to_U32(shader->getType())];

    // Clear the 'linked' flag. Program must be re-linked before usage
    _linked = false;
}

/// This should be called in the loading thread, but some issues are still present, and it's not recommended (yet)
void glShaderProgram::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback, bool skipRegister) {
    if (_asyncLoad) {
        GL_API::createOrValidateContextForCurrentThread(_context);
    }

    // We need a new API specific object
    // If we try to refresh the program, we already have a handle
    if (_shaderProgramID == GLUtil::_invalidObjectID) {
        if (Config::USE_SEPARATE_SHADER_OBJECTS) {
            glCreateProgramPipelines(1, &_shaderProgramIDTemp);
        }
        else {
            _shaderProgramIDTemp = glCreateProgram();
        }

        // Loading from binary is optional, but using it does require sending the driver a hint to give us access to it later
        if (s_useShaderBinaryCache) {
            glProgramParameteri(_shaderProgramIDTemp, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        }
        if (Config::USE_SEPARATE_SHADER_OBJECTS) {
            glProgramParameteri(_shaderProgramIDTemp, GL_PROGRAM_SEPARABLE, GL_TRUE);
        }
    } else {
        _shaderProgramIDTemp = _shaderProgramID;
    }

    if (!loadFromBinary()) {
        reloadShaders(false);
    }

    // Loading from binary gives us a linked program ready for usage.
    if (!_loadedFromBinary) {
        _stageMask = UseProgramStageMask::GL_NONE_BIT;
        // For every possible stage that the program might use
        for (glShader* shader : _shaderStage) {
            // If a shader exists for said stage, attach it
            attachShader(shader);
        }
        if (Config::USE_SEPARATE_SHADER_OBJECTS) {
            glUseProgramStages(_shaderProgramIDTemp, _stageMask, _shaderProgramIDTemp);
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

/// Linking a shader program also sets up all pre-link properties for the shader (varying locations, attrib bindings, etc)
void glShaderProgram::link() {
    Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM")), name().c_str(), _shaderProgramIDTemp);

    // Link the program
    glLinkProgram(_shaderProgramIDTemp);
    // Detach shaders after link so that the driver might free up some memory remove shader stages
    for (glShader* shader : _shaderStage) {
        detachShader(shader);
    }

    _shaderVarLocation.clear();

    // And check the result
    GLboolean linkStatus;
    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &linkStatus);
    // If linking failed, show an error, else print the result in debug builds.
    // Same getLog() method is used
    if (linkStatus == GL_FALSE) {
        Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")), name().c_str(), getLog().c_str());
        MemoryManager::SAFE_DELETE(_lockManager);
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")), name().c_str(), getLog().c_str());
        // The linked flag is set to true only if linking succeeded
        _linked = true;
        if (Config::ENABLE_GPU_VALIDATION) {
            glObjectLabel(GL_PROGRAM, _shaderProgramIDTemp, -1, name().c_str());
        }
    }
}

bool glShaderProgram::loadFromBinary() {
    _loadedFromBinary = _linked = false;

    // Load the program from the binary file, if available and allowed, to avoid linking.
    if (s_useShaderBinaryCache) {
        // Load the program's binary format from file
        vectorImpl<Byte> data;
        if (readFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin, _resourceName + ".fmt", data, FileType::BINARY) && !data.empty()) {
            _binaryFormat = *reinterpret_cast<GLenum*>(data.data());
            if (_binaryFormat != GL_NONE) {
                data.resize(0);
                if (readFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin, _resourceName + ".bin", data, FileType::BINARY) && !data.empty()) {
                    // Load binary code on the GPU
                    glProgramBinary(_shaderProgramIDTemp, _binaryFormat, (bufferPtr)data.data(), (GLint)data.size());
                    // Check if the program linked successfully on load
                    GLboolean success = 0;
                    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &success);
                    // If it loaded properly set all appropriate flags (this also prevents low level access to the program's shaders)
                    if (success == GL_TRUE) {
                        _loadedFromBinary = _linked = true;
                    }
                }
            }
        }
    }

    return _loadedFromBinary;
}

glShaderProgram::glShaderProgramLoadInfo glShaderProgram::buildLoadInfo() {

    glShaderProgramLoadInfo loadInfo;
    loadInfo._resourcePath = getResourceLocation() + "/" + Paths::Shaders::GLSL::g_parentShaderLoc;

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
        loadInfo._programProperties = "." + _resourceName.substr(propPosition + 1, propPositionVertex - propPosition - 1);
    }
    // Vertex properties start off identically to the rest of the stages' names
    loadInfo._vertexStageProperties = loadInfo._programProperties;
    // But we also add the shader specific properties
    if (propPositionVertex != stringImpl::npos) {
        loadInfo._vertexStageProperties += "." + _resourceName.substr(propPositionVertex + 1);
    }

    // Get all of the preprocessor defines and add them to the general shader header for this program
    for (const stringImpl& define : _definesList) {
        // Placeholders are ignored
        if (define.compare("DEFINE_PLACEHOLDER") != 0) {
            // We manually add define dressing
            loadInfo._header.append("#define " + define + "\n");
        }
    }

    return loadInfo;
}

void glShaderProgram::loadSourceCode(ShaderType stage,
                                     const stringImpl& stageName,
                                     const stringImpl& header,
                                     bool forceReParse,
                                     std::pair<bool, stringImpl>& sourceCodeOut) {

    sourceCodeOut.first = false;
    sourceCodeOut.second.resize(0);

    if (s_useShaderTextCache && !forceReParse) {
        ShaderProgram::shaderFileRead(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText,
                                      stageName,
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
        }
    }
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

    //glswClearCurrentContext();
    // The program wasn't loaded from binary, so process shaders
    // Use the specified shader path
    glswSetPath(info._resourcePath.c_str(), ".glsl");

    std::pair<bool, stringImpl> sourceCode;
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
             loadSourceCode(type, shaderCompileName, info._header, reparseShaderSource, sourceCode);
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
            Console::d_printfn(Locale::get(_ID("SHADER_MANAGER_GET_INC")), shader->name().c_str(), shader->GetRef());
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
    return GL_API::setActiveProgram(0u);
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
U32 glShaderProgram::GetSubroutineUniformCount(ShaderType type) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    I32 subroutineCount = 0;
    glGetProgramStageiv(_shaderProgramID, GLUtil::glShaderStageTable[to_U32(type)], GL_ACTIVE_SUBROUTINE_UNIFORMS, &subroutineCount);

    return std::max(subroutineCount, 0);
}

/// Get the uniform location of the specified subroutine uniform for the
/// specified stage. Not cached!
U32 glShaderProgram::GetSubroutineUniformLocation(
    ShaderType type, const char* name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineUniformLocation(_shaderProgramID, GLUtil::glShaderStageTable[to_U32(type)], name);
}

/// Get the index of the specified subroutine name for the specified stage. Not cached!
U32 glShaderProgram::GetSubroutineIndex(ShaderType type, const char* name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineIndex(_shaderProgramID, GLUtil::glShaderStageTable[to_U32(type)], name);
}

I32 glShaderProgram::cachedValueUpdate(const GFX::PushConstant& constant) {

    const char* location = constant._binding.c_str();
    U64 locationHash = _ID_RT(location);

    I32 binding = Binding(location);

    if (binding == -1 || _shaderProgramID == 0) {
        return -1;
    }

    UniformsByNameHash::ShaderVarMap::iterator it = _uniformsByNameHash._shaderVars.find(locationHash);
    if (it != std::end(_uniformsByNameHash._shaderVars)) {
        if (it->second == constant) {
            return -1;
        } else {
            it->second.assign(constant);
        }
    } else {
        hashAlg::emplace(_uniformsByNameHash._shaderVars, locationHash, constant);
    }

    return binding;
}

void glShaderProgram::Uniform(I32 binding, GFX::PushConstantType type, const vectorImpl<char>& values, bool flag) const {
    GLsizei byteCount = (GLsizei)values.size();
    switch (type) {
        case GFX::PushConstantType::BOOL:
            glProgramUniform1iv(_shaderProgramID, binding, byteCount / (1 * 1), (GLint*)castData<GLbyte, 1, char>(values));
            break;
        case GFX::PushConstantType::INT:
            glProgramUniform1iv(_shaderProgramID, binding, byteCount / (1 * 4), castData<GLint, 1, I32>(values));
            break;
        case GFX::PushConstantType::UINT:
            glProgramUniform1uiv(_shaderProgramID, binding, byteCount / (1 * 4), castData<GLuint, 1, U32>(values));
            break;
        case GFX::PushConstantType::DOUBLE:
            glProgramUniform1dv(_shaderProgramID, binding, byteCount / (1 * 8), castData<GLdouble, 1, D64>(values));
            break;
        case GFX::PushConstantType::FLOAT:
            glProgramUniform1fv(_shaderProgramID, binding, byteCount / (1 * 4), castData<GLfloat, 1, F32>(values));
            break;
        case GFX::PushConstantType::IVEC2:
            glProgramUniform2iv(_shaderProgramID, binding, byteCount / (2 * 4), castData<GLint, 2, vec2<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC3:
            glProgramUniform3iv(_shaderProgramID, binding, byteCount / (3 * 4), castData<GLint, 3, vec3<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC4:
            glProgramUniform4iv(_shaderProgramID, binding, byteCount / (4 * 4), castData<GLint, 4, vec4<I32>>(values));
            break;
        case GFX::PushConstantType::UVEC2:
            glProgramUniform2uiv(_shaderProgramID, binding, byteCount / (2 * 4), castData<GLuint, 2, vec2<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC3:
            glProgramUniform3uiv(_shaderProgramID, binding, byteCount / (3 * 4), castData<GLuint, 3, vec3<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC4:
            glProgramUniform4uiv(_shaderProgramID, binding, byteCount / (4 * 4), castData<GLuint, 4, vec4<U32>>(values));
            break;
        case GFX::PushConstantType::DVEC2:
            glProgramUniform2dv(_shaderProgramID, binding, byteCount / (2 * 8), castData<GLdouble, 2, vec2<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC3:
            glProgramUniform3dv(_shaderProgramID, binding, byteCount / (3 * 8), castData<GLdouble, 3, vec3<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC4:
            glProgramUniform4dv(_shaderProgramID, binding, byteCount / (4 * 8), castData<GLdouble, 4, vec4<D64>>(values));
            break;
        case GFX::PushConstantType::VEC2:
            glProgramUniform2fv(_shaderProgramID, binding, byteCount / (2 * 4), castData<GLfloat, 2, vec2<F32>>(values));
            break;
        case GFX::PushConstantType::VEC3:
            glProgramUniform3fv(_shaderProgramID, binding, byteCount / (3 * 4), castData<GLfloat, 3, vec3<F32>>(values));
            break;
        case GFX::PushConstantType::VEC4:
            glProgramUniform4fv(_shaderProgramID, binding, byteCount / (4 * 4), castData<GLfloat, 4, vec4<F32>>(values));
            break;
        case GFX::PushConstantType::IMAT2:
            glProgramUniformMatrix2fv(_shaderProgramID, binding, byteCount / (2 * 2 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT3:
            glProgramUniformMatrix3fv(_shaderProgramID, binding, byteCount / (3 * 3 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT4:
            glProgramUniformMatrix4fv(_shaderProgramID, binding, byteCount / (4 * 4 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<I32>>(values));
            break;
        case GFX::PushConstantType::UMAT2:
            glProgramUniformMatrix2fv(_shaderProgramID, binding, byteCount / (2 * 2 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT3:
            glProgramUniformMatrix3fv(_shaderProgramID, binding, byteCount / (3 * 3 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT4:
            glProgramUniformMatrix4fv(_shaderProgramID, binding, byteCount / (4 * 4 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<U32>>(values));
            break;
        case GFX::PushConstantType::MAT2:
            glProgramUniformMatrix2fv(_shaderProgramID, binding, byteCount / (2 * 2 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<F32>>(values));
            break;
        case GFX::PushConstantType::MAT3:
            glProgramUniformMatrix3fv(_shaderProgramID, binding, byteCount / (3 * 3 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<F32>>(values));
            break;
        case GFX::PushConstantType::MAT4:
            glProgramUniformMatrix4fv(_shaderProgramID, binding, byteCount / (4 * 4 * 4), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<F32>>(values));
            break;
        case GFX::PushConstantType::DMAT2:
            glProgramUniformMatrix2dv(_shaderProgramID, binding, byteCount / (2 * 2 * 8), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 4, mat2<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT3:
            glProgramUniformMatrix3dv(_shaderProgramID, binding, byteCount / (3 * 3 * 8), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 9, mat3<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT4:
            glProgramUniformMatrix4dv(_shaderProgramID, binding, byteCount / (4 * 4 * 8), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 16, mat4<D64>>(values));
            break;
        default:
            DIVIDE_ASSERT(false, "glShaderProgram::Uniform error: Unhandled data type!");
    }
}

void glShaderProgram::UploadPushConstant(const GFX::PushConstant& constant) {
    I32 binding = cachedValueUpdate(constant);

    if (binding != -1) {
        Uniform(binding, constant._type, constant._buffer, constant._flag);
    }
}

void glShaderProgram::UploadPushConstants(const PushConstants& constants) {
    for (const GFX::PushConstant& constant : constants.data()) {
        if (!constant._binding.empty() && constant._type != GFX::PushConstantType::COUNT) {
            UploadPushConstant(constant);
        }
    }
}

void glShaderProgram::reuploadUniforms() {
    for (UniformsByNameHash::ShaderVarMap::value_type it : _uniformsByNameHash._shaderVars) {
        UploadPushConstant(it.second);
    }
}

};
