#include "Headers/glShaderProgram.h"

#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/Shader.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {

std::array<U32, to_const_uint(ShaderType::COUNT)> glShaderProgram::_lineOffset;

IMPLEMENT_ALLOCATOR(glShaderProgram, 0, 0);
glShaderProgram::glShaderProgram(GFXDevice& context, bool asyncLoad)
    : ShaderProgram(context, asyncLoad),
      _loadedFromBinary(false),
      _validated(false),
      _shaderProgramIDTemp(0),
     _lockManager(new glLockManager())
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
    }

    // remove shader stages
    for (ShaderIDMap::value_type& it : _shaderIDMap) {
        detachShader(it.second);
    }
    // delete shader program
    if (_shaderProgramID > 0 && _shaderProgramID != GLUtil::_invalidObjectID) {
        glDeleteProgram(_shaderProgramID);
        // glDeleteProgramPipelines(1, &_shaderProgramID);
    }
}

/// Basic OpenGL shader program validation (both in debug and in release)
void glShaderProgram::validateInternal() {
    for (U32 i = 0; i < to_const_uint(ShaderType::COUNT); ++i) {
        // Get the shader pointer for that stage
        Shader* shader = _shaderStage[i];
        // If a shader exists for said stage
        if (shader) {
            // Validate it
            shader->validate();
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
                         getName().c_str(), getLog().c_str());
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_PROGRAM")),
                           getName().c_str(), getLog().c_str());
    }
    _validated = true;
}

/// Called once per frame. Used to update internal state
bool glShaderProgram::update(const U64 deltaTime) {
    // If we haven't validated the program but used it at lease once ...
    if (_validationQueued && _shaderProgramID != 0) {
        if (_lockManager) {
            _lockManager->Wait(true);
            _lockManager.reset(nullptr);
        }
        // Call the internal validation function
        validateInternal();
        // We dump the shader binary only if it wasn't loaded from one
        if (!_loadedFromBinary && _context.getGPUVendor() == GPUVendor::NVIDIA && false) {
            STUBBED(
                "GLSL binary dump/load is only enabled for nVidia GPUS. "
                "Catalyst 13.x  - 15.x destroys uniforms on shader dump, for whatever "
                "reason. - Ionut")
            // Get the size of the binary code
            GLint binaryLength = 0;
            glGetProgramiv(_shaderProgramID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
            // allocate a big enough buffer to hold it
            void* binary = (void*)malloc(binaryLength);
            DIVIDE_ASSERT(binary != NULL,
                          "glShaderProgram error: could not allocate memory "
                          "for the program binary!");
            // and fill the buffer with the binary code
            glGetProgramBinary(_shaderProgramID, binaryLength, NULL, &_binaryFormat, binary);
            if (_binaryFormat != GL_NONE) {
                // dump the buffer to file
                stringImpl outFileName(Shader::CACHE_LOCATION_BIN + getName() + ".bin");
                FILE* outFile = fopen(outFileName.c_str(), "wb");
                if (outFile != NULL) {
                    fwrite(binary, binaryLength, 1, outFile);
                    fclose(outFile);
                }
                // dump the format to a separate file (highly non-optimised. Should dump formats to a database instead)
                outFileName += ".fmt";
                outFile = fopen(outFileName.c_str(), "wb");
                if (outFile != NULL) {
                    fwrite((void*)&_binaryFormat, sizeof(GLenum), 1, outFile);
                    fclose(outFile);
                }
            }
            // delete our local code buffer
            free(binary);
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
        if (validationBuffer.size() > 4096 * 16) {
            // On some systems, the program's disassembly is printed, and that
            // can get quite large
            validationBuffer.resize(static_cast<stringAlg::stringSize>(
                4096 * 16 - strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) - 10));
            // Use the simple "truncate and inform user" system (a.k.a. add dots
            // and delete the rest)
            validationBuffer.append(" ... ");
        }
    }
    // Return the final message, whatever it may contain
    return validationBuffer;
}

/// Remove a shader stage from this program
void glShaderProgram::detachShader(Shader* const shader) {
    glDetachShader(_shaderProgramID, shader->getShaderID());
    // glUseProgramStages(_shaderProgramID,
    // GLUtil::glShaderStageTable[to_uint(shader->getType())], 0);
}

/// Add a new shader stage to this program
void glShaderProgram::attachShader(Shader* const shader, const bool refresh) {
    if (!shader) {
        return;
    }

    GLuint shaderID = shader->getShaderID();
    // If refresh == true, than we are re-attaching an existing shader (possibly
    // after re-compilation)
    if (refresh) {
        // Find the previous iteration (and print an error if not found)
        ShaderIDMap::iterator it = _shaderIDMap.find(shaderID);
        if (it != std::end(_shaderIDMap)) {
            // Update the internal pointer
            it->second = shader;
            // and detach the shader
            detachShader(shader);
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_RECOMPILE_NOT_FOUND_ATOM")),
                             shader->getName().c_str());
        }
    } else {
        // If refresh == false, we are adding a new stage
        hashAlg::emplace(_shaderIDMap, shaderID, shader);
    }

    // Attach the shader
    glAttachShader(_shaderProgramIDTemp, shaderID);
    // glUseProgramStages(_shaderProgramIDTemp,
    // GLUtil::glShaderStageTable[to_uint(shader->getType())], shaderID);
    // Clear the 'linked' flag. Program must be re-linked before usage
    _linked = false;
}

/// This should be called in the loading thread, but some issues are still
/// present, and it's not recommended (yet)
void glShaderProgram::threadedLoad(const stringImpl& name) {
    // Loading from binary gives us a linked program ready for usage.
    if (!_loadedFromBinary) {
        // If this wasn't loaded from binary, we need a new API specific object
        // If we try to refresh the program, we already have a handle
        if (_shaderProgramID == GLUtil::_invalidObjectID) {
            _shaderProgramIDTemp = glCreateProgram();
            // glCreateProgramPipelines(1, &_shaderProgramIDTemp);
        }
        // For every possible stage that the program might use
        for (U32 i = 0; i < to_const_uint(ShaderType::COUNT); ++i) {
            // Get the shader pointer for that stage
            Shader* shader = _shaderStage[i];
            // If a shader exists for said stage
            if (shader) {
                // Attach it
                attachShader(shader, _refreshStage[i]);
                // Clear the refresh flag for this stage
                _refreshStage[i] = false;
            }
        }
        // Link the program
        link();
    }
    // This was once an atomic swap. Might still be in the future
    _shaderProgramID = _shaderProgramIDTemp;
    // Pass the rest of the loading steps to the parent class
    _lockManager->Lock(_asyncLoad);
    ShaderProgram::load();
}

/// Linking a shader program also sets up all pre-link properties for the shader
/// (varying locations, attrib bindings, etc)
void glShaderProgram::link() {
#if !defined(_DEBUG)
    // Loading from binary is optional, but it using it does require sending the
    // driver a hint to give us access to it later
    if (Config::USE_SHADER_BINARY) {
        glProgramParameteri(_shaderProgramIDTemp,
                            GL_PROGRAM_BINARY_RETRIEVABLE_HINT, 1);
    }
#endif
    Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM")), getName().c_str(),
                       _shaderProgramIDTemp);

    /*glProgramParameteri(_shaderProgramIDTemp,
                        GL_PROGRAM_SEPARABLE,
                        1);
    */
    // Link the program
    glLinkProgram(_shaderProgramIDTemp);
    // And check the result
    GLint linkStatus = 0;
    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &linkStatus);
    // If linking failed, show an error, else print the result in debug builds.
    // Same getLog() method is used
    if (linkStatus == 0) {
        Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")),
                         getName().c_str(), getLog().c_str());
    } else {
        Console::d_printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")),
                           getName().c_str(), getLog().c_str());
        // The linked flag is set to true only if linking succeeded
        _linked = true;
    }

    _shaderVarsU32.clear();
    _shaderVarsI32.clear();
    _shaderVarsF32.clear();
    _shaderVarsVec2F32.clear();
    _shaderVarsVec2I32.clear();
    _shaderVarsVec3F32.clear();
    _shaderVarsVec4F32.clear();
    _shaderVarsMat3.clear();
    _shaderVarsMat4.clear();
    _shaderVarLocation.clear();
}

/// Creation of a new shader program. Pass in a shader token and use glsw to
/// load the corresponding effects
bool glShaderProgram::load() {
    ShaderManager& shaderMgr = ShaderManager::instance();

    // NULL shader means use shaderProgram(0), so bypass the normal
    // loading routine
    if (_name.compare("NULL") == 0) {
        _validationQueued = false;
        _shaderProgramID = 0;
        ShaderProgram::load();
        return true;
    }

    // Reset the linked status of the program
    _linked = _loadedFromBinary = false;

    // Check if we need to refresh an existing program, or create a new one
    bool refresh = false;
    for (U32 i = 0; i < to_const_uint(ShaderType::COUNT); ++i) {
        if (_refreshStage[i]) {
            refresh = true;
            break;
        }
    }

#if !defined(_DEBUG)
    // Load the program from the binary file, if available and allowed, to avoid linking.
    if (Config::USE_SHADER_BINARY && !refresh && false && _context.getGPUVendor() == GPUVendor::NVIDIA) {
        // Only available for new programs
        assert(_shaderProgramIDTemp == 0);
        stringImpl fileName(Shader::CACHE_LOCATION_BIN + _name + ".bin");
        // Load the program's binary format from file
        FILE* inFile = fopen((fileName + ".fmt").c_str(), "wb");
        if (inFile) {
            fread(&_binaryFormat, sizeof(GLenum), 1, inFile);
            fclose(inFile);
            // If we loaded the binary format successfully, load the binary
            inFile = fopen(fileName.c_str(), "rb");
        } else {
            // If the binary format load failed, we don't need to load the
            // binary code as it's useless without a proper format
            inFile = nullptr;
        }
        if (inFile && _binaryFormat != GL_ZERO && _binaryFormat != GL_NONE) {
            // Jump to the end of the file
            fseek(inFile, 0, SEEK_END);
            // And get the file's content size
            GLint binaryLength = (GLint)ftell(inFile);
            // Allocate a sufficiently large local buffer to hold the contents
            void* binary = (void*)malloc(binaryLength);
            // Jump back to the start of the file
            fseek(inFile, 0, SEEK_SET);
            // Read the contents from the file and save them locally
            fread(binary, binaryLength, 1, inFile);
            // Allocate a new handle
            _shaderProgramIDTemp = glCreateProgram();
            // glCreateProgramPipelines(1, &_shaderProgramIDTemp);
            // Load binary code on the GPU
            glProgramBinary(_shaderProgramIDTemp, _binaryFormat, binary,
                            binaryLength);
            // Delete the local binary code buffer
            free(binary);
            // Check if the program linked successfully on load
            GLint success = 0;
            glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &success);
            // If it loaded properly set all appropriate flags (this also
            // prevents low level access to the program's shaders)
            if (success == 1) {
                _loadedFromBinary = _linked = true;
            }
        }
        // Close the file
        fclose(inFile);
    }
#endif
    // The program wasn't loaded from binary, so process shaders
    if (!_loadedFromBinary) {
        // Use the specified shader path
        glswSetPath((getResourceLocation() + "GLSL/").c_str(), ".glsl");
        // Get all of the preprocessor defines and add them to the general
        // shader header for this program
        stringImpl shaderSourceHeader;
        for (U8 i = 0; i < _definesList.size(); ++i) {
            // Placeholders are ignored
            if (_definesList[i].compare("DEFINE_PLACEHOLDER") == 0) {
                continue;
            }
            // We manually add define dressing
            shaderSourceHeader.append("#define " + _definesList[i] + "\n");
        }
        // Split the shader name to get the effect file name and the effect properties
        // The effect file name is the part up until the first period or comma symbol
        stringImpl shaderName = _name.substr(0, _name.find_first_of(".,"));
        // We also differentiate between general properties, and vertex properties
        stringImpl shaderProperties;
        // Get the position of the first "," symbol. Must be added at the end of the program's name!!
        stringAlg::stringSize propPositionVertex = _name.find_first_of(",");
        // Get the position of the first "." symbol
        stringAlg::stringSize propPosition = _name.find_first_of(".");
        // If we have effect properties, we extract them from the name
        // (starting from the first "." symbol to the first "," symbol)
        if (propPosition != stringImpl::npos) {
            shaderProperties =
                "." +
                _name.substr(propPosition + 1,
                             propPositionVertex - propPosition - 1);
        }
        // Vertex properties start off identically to the rest of the stages' names
        stringImpl vertexProperties(shaderProperties);
        // But we also add the shader specific properties
        if (propPositionVertex != stringImpl::npos) {
            vertexProperties += "." + _name.substr(propPositionVertex + 1);
        }

        // For every stage
        for (U32 i = 0; i < to_const_uint(ShaderType::COUNT); ++i) {
            // Brute force conversion to an enum
            ShaderType type = static_cast<ShaderType>(i);
            stringImpl shaderCompileName(shaderName + "." + GLUtil::glShaderStageNameTable[to_uint(type)] + vertexProperties);
            // If we request a refresh for the current stage, we need to have a
            // pointer for the stage's shader already
            if (!_refreshStage[i]) {
                // Else, we ask the shader manager to see if it was previously loaded elsewhere
                _shaderStage[i] = shaderMgr.getShader(shaderCompileName, refresh);
            }

            // If this is the first time this shader is loaded ...
            if (!_shaderStage[i]) {
                bool parseIncludes = false;

                stringImpl sourceCode;
                if (Config::USE_SHADER_TEXT_CACHE) {
                    shaderMgr.shaderFileRead(Shader::CACHE_LOCATION_TEXT + shaderCompileName,
                                             true,
                                             sourceCode);
                }

                if (sourceCode.empty()) {
                    parseIncludes = true;
                    // Use GLSW to read the appropriate part of the effect file
                    // based on the specified stage and properties
                    const char* sourceCodeStr = glswGetShader(
                        shaderCompileName.c_str(),
                        _refreshStage[i]);
                    sourceCode = sourceCodeStr ? sourceCodeStr : "";
                    // GLSW may fail for various reasons (not a valid effect stage, invalid name, etc)
                    if (!sourceCode.empty()) {
                        // And replace in place with our program's headers created earlier
                        Util::ReplaceStringInPlace(sourceCode, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                        Util::ReplaceStringInPlace(sourceCode, "//__LINE_OFFSET_", 
                            Util::StringFormat("#line %d\n", 1 + _lineOffset[i] + to_uint(_definesList.size())));
                    }
                }
                if (!sourceCode.empty()){
                    // Load our shader from the final string and save it in the manager in case a new Shader Program needs it
                    _shaderStage[i] = shaderMgr.loadShader(shaderCompileName, sourceCode, type, parseIncludes, _refreshStage[i]);
                }
            }
            // Show a message, in debug, if we don't have a shader for this stage
            if (!_shaderStage[i]) {
                Console::d_printfn(Locale::get(_ID("WARN_GLSL_LOAD")),
                                   shaderCompileName.c_str());
            } else {
                // Try to compile the shader (it doesn't double compile shaders,
                // so it's safe to call it multiple types)
                if (!_shaderStage[i]->compile()) {
                    Console::errorfn(Locale::get(_ID("ERROR_GLSL_COMPILE")),
                                     _shaderStage[i]->getShaderID());
                }
            }
        }
    }

    // try to link the program in a separate thread
    return _context.loadInContext(
        !_loadedFromBinary && _asyncLoad
            ? CurrentContext::GFX_LOADING_CTX
            : CurrentContext::GFX_RENDERING_CTX,
        [&](){
            threadedLoad(_name);
        });
}

/// Check every possible combination of flags to make sure this program can be
/// used for rendering
bool glShaderProgram::isValid() const {
    // null shader is a valid shader
    return _shaderProgramID == 0 ||
           (_linked && _shaderProgramID != GLUtil::_invalidObjectID);
}

bool glShaderProgram::isBound() const {
    return GL_API::_activeShaderProgram == _shaderProgramID;
}

/// Cache uniform/attribute locations for shader programs
/// When we call this function, we check our name<->address map to see if we
/// queried the location before
/// If we didn't, ask the GPU to give us the variables address and save it for
/// later use
I32 glShaderProgram::getUniformLocation(const char* name) {
    // If the shader can't be used for rendering, just return an invalid address
    if (_shaderProgramID == 0 || !isValid()) {
        return -1;
    }

    // Check the cache for the location
    ULL nameHash = _ID_RT(name);
    ShaderVarMap::const_iterator it = _shaderVarLocation.find(nameHash);
    if (it != std::end(_shaderVarLocation)) {
        return it->second;
    }

    // Cache miss. Query OpenGL for the location
    GLint location = glGetUniformLocation(_shaderProgramID, name);

    // Save it for later reference
    hashAlg::emplace(_shaderVarLocation, nameHash, location);

    // Return the location
    return location;
}

/// Bind this shader program
bool glShaderProgram::bind() {
    // If the shader isn't ready or failed to link, stop here
    if (!isValid()) {
        return false;
    }

    if (_lockManager) {
        _lockManager->Wait(true);
        _lockManager.reset(nullptr);
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
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_uint(type)],
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
        glUniformSubroutinesuiv(GLUtil::glShaderStageTable[to_uint(type)], 1,
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
                        GLUtil::glShaderStageTable[to_uint(type)],
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
        _shaderProgramID, GLUtil::glShaderStageTable[to_uint(type)],
        name);
}

/// Get the index of the specified subroutine name for the specified stage. Not
/// cached!
U32 glShaderProgram::GetSubroutineIndex(ShaderType type,
                                        const char* name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineIndex(_shaderProgramID,
                                GLUtil::glShaderStageTable[to_uint(type)],
                                name);
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, U32 value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform1ui(_shaderProgramID, location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, I32 value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform1i(_shaderProgramID, location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, F32 value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform1f(_shaderProgramID, location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vec2<F32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform2fv(_shaderProgramID, location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vec2<I32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform2iv(_shaderProgramID, location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vec3<F32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform3fv(_shaderProgramID, location, 1, value);
    }
}

void glShaderProgram::Uniform(I32 location, const vec3<I32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform3iv(_shaderProgramID, location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vec4<F32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform4fv(_shaderProgramID, location, 1, value);
    }
}

void glShaderProgram::Uniform(I32 location, const vec4<I32>& value) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniform4iv(_shaderProgramID, location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const mat3<F32>& value,
                              bool transpose) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniformMatrix3fv(_shaderProgramID, location, 1,
                                  transpose ? GL_TRUE : GL_FALSE, value.mat);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const mat4<F32>& value,
                              bool transpose) {
    if (cachedValueUpdate(location, value)) {
        glProgramUniformMatrix4fv(_shaderProgramID, location, 1,
                                  transpose ? GL_TRUE : GL_FALSE, value.mat);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vectorImpl<I32>& values) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniform1iv(_shaderProgramID, location, (GLsizei)values.size(),
                        values.data());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vectorImpl<F32>& values) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniform1fv(_shaderProgramID, location, (GLsizei)values.size(),
                        values.data());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<vec2<F32> >& values) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniform2fv(_shaderProgramID, location, (GLsizei)values.size(),
                        values.front());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<vec3<F32> >& values) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniform3fv(_shaderProgramID, location, (GLsizei)values.size(),
                        values.front());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vectorImplAligned<vec4<F32> >& values) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniform4fv(_shaderProgramID, location, (GLsizei)values.size(),
                        values.front());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<mat3<F32> >& values,
                              bool transpose) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniformMatrix3fv(_shaderProgramID, location,
                              (GLsizei)values.size(),
                              transpose ? GL_TRUE : GL_FALSE, values.front());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImplAligned<mat4<F32> >& values,
                              bool transpose) {
    if (values.empty() || location == -1) {
        return;
    }

    glProgramUniformMatrix4fv(_shaderProgramID, location,
                              (GLsizei)values.size(),
                              transpose ? GL_TRUE : GL_FALSE, values.front());
}

void glShaderProgram::DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) {
    glDispatchCompute(xGroups, yGroups, zGroups);
}

void glShaderProgram::SetMemoryBarrier(MemoryBarrierType type) {
    MemoryBarrierMask barrierType = MemoryBarrierMask::GL_ALL_BARRIER_BITS;
    switch (type) {
        case MemoryBarrierType::ALL :
            break;
        case MemoryBarrierType::BUFFER :
            barrierType = MemoryBarrierMask::GL_BUFFER_UPDATE_BARRIER_BIT;
            break;
        case MemoryBarrierType::SHADER_BUFFER :
            barrierType = MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
            break;
        case MemoryBarrierType::COUNTER:
            barrierType = MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
            break;
        case MemoryBarrierType::QUERY:
            barrierType = MemoryBarrierMask::GL_QUERY_BUFFER_BARRIER_BIT;
            break;
        case MemoryBarrierType::RENDER_TARGET:
            barrierType = MemoryBarrierMask::GL_FRAMEBUFFER_BARRIER_BIT;
            break;
        case MemoryBarrierType::TEXTURE:
            barrierType = MemoryBarrierMask::GL_TEXTURE_UPDATE_BARRIER_BIT;
            break;
        case MemoryBarrierType::TRANSFORM_FEEDBACK:
            barrierType = MemoryBarrierMask::GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
            break;
    }

    glMemoryBarrier(barrierType);
}

};
