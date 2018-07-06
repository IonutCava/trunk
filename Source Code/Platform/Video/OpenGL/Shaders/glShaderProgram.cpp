#include "Headers/glShaderProgram.h"

#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/Shader.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

stringImpl glShaderProgram::_lastPathPrefix;
stringImpl glShaderProgram::_lastPathSuffix;

glShaderProgram::glShaderProgram(const bool optimise)
    : ShaderProgram(optimise),
      _loadedFromBinary(false),
      _validated(false),
      _shaderProgramIDTemp(0) {
    _validationQueued = false;
    // each API has it's own invalid id. This is OpenGL's
    _shaderProgramID = GLUtil::_invalidObjectID;
    // some basic translation tables for shade types
    _shaderStageTable[VERTEX_SHADER] = GL_VERTEX_SHADER;
    _shaderStageTable[FRAGMENT_SHADER] = GL_FRAGMENT_SHADER;
    _shaderStageTable[GEOMETRY_SHADER] = GL_GEOMETRY_SHADER;
    _shaderStageTable[TESSELATION_CTRL_SHADER] = GL_TESS_CONTROL_SHADER;
    _shaderStageTable[TESSELATION_EVAL_SHADER] = GL_TESS_EVALUATION_SHADER;
    _shaderStageTable[COMPUTE_SHADER] = GL_COMPUTE_SHADER;
    // pointers to all of our shader stages
    _shaderStage[VERTEX_SHADER] = nullptr;
    _shaderStage[FRAGMENT_SHADER] = nullptr;
    _shaderStage[GEOMETRY_SHADER] = nullptr;
    _shaderStage[TESSELATION_CTRL_SHADER] = nullptr;
    _shaderStage[TESSELATION_EVAL_SHADER] = nullptr;
    _shaderStage[COMPUTE_SHADER] = nullptr;
}

glShaderProgram::~glShaderProgram() {
    // remove shader stages
    for (ShaderIDMap::value_type& it : _shaderIDMap) {
        detachShader(it.second);
    }
    // delete shader program
    if (_shaderProgramID > 0 && _shaderProgramID != GLUtil::_invalidObjectID) {
        glDeleteProgram(_shaderProgramID);
    }
}

/// Basic OpenGL shader program validation (both in debug and in release)
void glShaderProgram::validateInternal() {
    GLint status = 0;
    glValidateProgram(_shaderProgramID);
    glGetProgramiv(_shaderProgramID, GL_VALIDATE_STATUS, &status);

    // we print errors in debug and in release, but everything else only in
    // debug
    // the validation log is only retrieved if we request it. (i.e. in release,
    // if the shader is validated, it isn't retrieved)
    if (status == 0) {
        Console::errorfn(Locale::get("GLSL_VALIDATING_PROGRAM"),
                         getName().c_str(), getLog().c_str());
    } else {
        Console::d_printfn(Locale::get("GLSL_VALIDATING_PROGRAM"),
                           getName().c_str(), getLog().c_str());
    }
    _validated = true;
}

/// Called once per frame. Used to update internal state
bool glShaderProgram::update(const U64 deltaTime) {
    // If we haven't validated the program but used it at lease once ...
    if (_validationQueued) {
        // Call the internal validation function
        validateInternal();
        // We dump the shader binary only if it wasn't loaded from one
        if (!_loadedFromBinary &&
            GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA) {
            STUBBED(
                "GLSL binary dump/load is only enabled for nVidia GPUS. "
                "Catalyst 14.x destroys uniforms on shader dump, for whatever "
                "reason. - Ionut")
            // Get the size of the binary code
            GLint binaryLength = 0;
            glGetProgramiv(_shaderProgramID, GL_PROGRAM_BINARY_LENGTH,
                           &binaryLength);
            // allocate a big enough buffer to hold it
            void* binary = (void*)malloc(binaryLength);
            DIVIDE_ASSERT(binary != NULL,
                          "glShaderProgram error: could not allocate memory "
                          "for the program binary!");
            // and fill the buffer with the binary code
            glGetProgramBinary(_shaderProgramID, binaryLength, NULL,
                               &_binaryFormat, binary);
            // dump the buffer to file
            stringImpl outFileName("shaderCache/Binary/" + getName() + ".bin");
            FILE* outFile = fopen(outFileName.c_str(), "wb");
            if (outFile != NULL) {
                fwrite(binary, binaryLength, 1, outFile);
                fclose(outFile);
            }
            // dump the format to a separate file (highly non-optimised. Should
            // dump formats to a database instead)
            outFileName += ".fmt";
            outFile = fopen(outFileName.c_str(), "wb");
            if (outFile != NULL) {
                fwrite((void*)&_binaryFormat, sizeof(GLenum), 1, outFile);
                fclose(outFile);
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
    // If we actually have something in the validation log
    if (length > 1) {
        // Delete our OK string, and start on a new line
        validationBuffer = "\n -- ";
        // This little trick avoids a "NEW/DELETE" set of calls and still gives
        // us a linear array of char's
        vectorImpl<char> shaderProgramLog(length);
        glGetProgramInfoLog(_shaderProgramIDTemp, length, NULL,
                            &shaderProgramLog[0]);
        // Append the program's log to the output message
        validationBuffer.append(&shaderProgramLog[0]);
        // To avoid overflowing the output buffers (both CEGUI and Console),
        // limit the maximum output size
        if (validationBuffer.size() > 4096 * 16) {
            // On some systems, the program's disassembly is printed, and that
            // can get quite large
            validationBuffer.resize(static_cast<stringAlg::stringSize>(
                4096 * 16 - strlen(Locale::get("GLSL_LINK_PROGRAM_LOG")) - 10));
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
    DIVIDE_ASSERT(_threadedLoadComplete,
                  "glShaderProgram error: tried to detach a shader from a "
                  "program that didn't finish loading!");
    glDetachShader(_shaderProgramID, shader->getShaderID());
    shader->removeParentProgram(this);
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
            _shaderIDMap[shaderID] = shader;
            // and detach the shader
            detachShader(shader);
        } else {
            Console::errorfn(
                Locale::get("ERROR_SHADER_RECOMPILE_NOT_FOUND_ATOM"),
                shader->getName().c_str());
        }
    } else {
        // If refresh == false, we are adding a new stage
        hashAlg::emplace(_shaderIDMap, shaderID, shader);
    }

    // Attach the shader
    glAttachShader(_shaderProgramIDTemp, shaderID);
    // And register the program with the shader
    shader->addParentProgram(this);
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
        }
        // For every possible stage that the program might use
        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i) {
            // Get the shader pointer for that stage
            Shader* shader = _shaderStage[i];
            // If a shader exists for said stage
            if (shader) {
                // Validate it
                shader->validate();
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
    // Clear texture<->binding map
    _textureSlots.clear();
    // Pass the rest of the loading steps to the parent class
    ShaderProgram::generateHWResource(name);
}

/// Linking a shader program also sets up all pre-link properties for the shader
/// (varying locations, attrib bindings, etc)
void glShaderProgram::link() {
#ifdef NDEBUG
    // Loading from binary is optional, but it using it does require sending the
    // driver a hint to give us access to it later
    if (Config::USE_SHADER_BINARY) {
        glProgramParameteri(_shaderProgramIDTemp,
                            GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                            1);
    }
#endif
    Console::d_printfn(Locale::get("GLSL_LINK_PROGRAM"), getName().c_str(),
                       _shaderProgramIDTemp);

    /*glProgramParameteri(_shaderProgramIDTemp, 
                        GL_PROGRAM_SEPARABLE,
                        1);
    */
    // If we require specific outputs from the shader for transform feedback, we
    // need to set them up before linking
    if (_outputCount > 0) {
        // This isn't as optimised as it should/could be, but it works
        vectorImpl<const char*> vars;
        for (U32 i = 0; i < _outputCount; ++i) {
            vars.push_back(strdup(("outData" + Util::toString(i)).c_str()));
        }
        // Only separate attributes are supported for now. Interleaved not top
        // prio
        glTransformFeedbackVaryings(_shaderProgramIDTemp, _outputCount,
                                    vars.data(), GL_SEPARATE_ATTRIBS);
    }
    // Link the program
    glLinkProgram(_shaderProgramIDTemp);
    // And check the result
    GLint linkStatus = 0;
    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &linkStatus);
    // If linking failed, show an error, else print the result in debug builds.
    // Same getLog() method is used
    if (linkStatus == 0) {
        Console::errorfn(Locale::get("GLSL_LINK_PROGRAM_LOG"),
                         getName().c_str(), getLog().c_str());
    } else {
        Console::d_printfn(Locale::get("GLSL_LINK_PROGRAM_LOG"),
                           getName().c_str(), getLog().c_str());
        // The linked flag is set to true only if linking succeeded
        _linked = true;
    }
}

/// Creation of a new shader program. Pass in a shader token and use glsw to
/// load the corresponding effects
bool glShaderProgram::generateHWResource(const stringImpl& name) {
    _name = name;

    // NULL_SHADER shader means use shaderProgram(0), so bypass the normal
    // loading routine
    if (name.compare("NULL_SHADER") == 0) {
        _validationQueued = false;
        _shaderProgramID = 0;
        _threadedLoadComplete = HardwareResource::generateHWResource(name);
        return true;
    }

    // Reset the linked status of the program
    _linked = _loadedFromBinary = false;

    // Check if we need to refresh an existing program, or create a new one
    bool refresh = false;
    for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i) {
        if (_refreshStage[i]) {
            refresh = true;
            break;
        }
    }

#ifdef NDEBUG
    // Load the program from the binary file, if available and allowed, to avoid
    // linking.
    if (Config::USE_SHADER_BINARY && !refresh &&
        GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA) {
        // Only available for new programs
        assert(_shaderProgramIDTemp == 0);
        stringImpl fileName("shaderCache/Binary/" + _name + ".bin");
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
        if (inFile) {
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
            // Close the file
            fclose(inFile);
            // Allocate a new handle
            _shaderProgramIDTemp = glCreateProgram();
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
                _threadedLoading = false;
            }
        }
    }
#endif
    // The program wasn't loaded from binary, so process shaders
    if (!_loadedFromBinary) {
        bool updatePath = false;
        // Use the specified shader path
        if (_lastPathPrefix.compare(getResourceLocation() + "GLSL/") != 0) {
            _lastPathPrefix = getResourceLocation() + "GLSL/";
            updatePath = true;
        }
        if (_lastPathSuffix.compare(".glsl") != 0) {
            _lastPathSuffix = ".glsl";
            updatePath = true;
        }
        if (updatePath) {
            glswSetPath(_lastPathPrefix.c_str(), _lastPathSuffix.c_str());
        }
        // Mirror initial shader defines to match line count
        GLint initialOffset = 20;
        if (GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA) {  // nVidia
                                                               // specific
            initialOffset += 6;
        }
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
            // We also take in consideration any line count offset that this
            // causes
            initialOffset++;
        }
        // Now that we have an offset for the general header, we need to move to
        // per-stage offsets
        GLint lineCountOffset[ShaderType_PLACEHOLDER];
        // Every stage has it's own uniform specific header
        stringImpl shaderSourceUniforms[ShaderType_PLACEHOLDER];
        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i) {
            // We start off from the general offset
            lineCountOffset[i] = initialOffset;
            // And add every custom uniform specified (yet again, we add all the
            // needed dressing)
            for (U8 j = 0; j < _customUniforms[i].size(); ++j) {
                shaderSourceUniforms[i].append("uniform " +
                                               _customUniforms[i][j] + ";\n");
                lineCountOffset[i]++;
            }
            // We also add a custom include to all shaders that contains GPU
            // data buffers. We need to account for its size as well
            lineCountOffset[i] += 42;
        }
        // GLSW directives are accounted here
        lineCountOffset[VERTEX_SHADER] += 18;
        lineCountOffset[FRAGMENT_SHADER] += 10;

        // Split the shader name to get the effect file name and the effect
        // properties
        // The effect file name is the part up until the first period or comma
        // symbol
        stringImpl shaderName = name.substr(0, name.find_first_of(".,"));
        // We also differentiate between general properties, and vertex
        // properties
        stringImpl shaderProperties, vertexProperties;
        // Get the position of the first "," symbol. Must be added at the end of
        // the program's name!!
        stringAlg::stringSize propPositionVertex = name.find_first_of(",");
        // Get the position of the first "." symbol
        stringAlg::stringSize propPosition = name.find_first_of(".");
        // If we have effect properties, we extract them from the name
        // (starting from the first "." symbol to the first "," symbol)
        if (propPosition != stringImpl::npos) {
            shaderProperties =
                "." +
                name.substr(propPosition + 1,
                            propPositionVertex - propPosition - 1);
        }
        // Vertex properties start off identically to the rest of the stages'
        // names
        vertexProperties += shaderProperties;
        // But we also add the shader specific properties
        if (propPositionVertex != stringImpl::npos) {
            vertexProperties += "." + name.substr(propPositionVertex + 1);
        }

        // Create an appropriate name for every shader stage
        stringImpl shaderCompileName[ShaderType_PLACEHOLDER];
        shaderCompileName[VERTEX_SHADER] =
            shaderName + ".Vertex" + vertexProperties;
        shaderCompileName[FRAGMENT_SHADER] =
            shaderName + ".Fragment" + shaderProperties;
        shaderCompileName[GEOMETRY_SHADER] =
            shaderName + ".Geometry" + shaderProperties;
        shaderCompileName[TESSELATION_CTRL_SHADER] =
            shaderName + ".TessellationC" + shaderProperties;
        shaderCompileName[TESSELATION_EVAL_SHADER] =
            shaderName + ".TessellationE" + shaderProperties;
        shaderCompileName[COMPUTE_SHADER] =
            shaderName + ".Compute" + shaderProperties;

        // For every stage
        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i) {
            // Brute force conversion to an enum
            ShaderType type = (ShaderType)(i);

            // If we request a refresh for the current stage, we need to have a
            // pointer for the stage's shader already
            if (!_refreshStage[type]) {
                // Else, we ask the shader manager to see if it was previously
                // loaded elsewhere
                _shaderStage[type] = ShaderManager::getInstance().getShader(
                    shaderCompileName[type], refresh);
            }

            // If this is the first time this shader is loaded ...
            if (!_shaderStage[type]) {
                // Use GLSW to read the appropriate part of the effect file
                // based on the specified stage and properties
                const char* sourceCode =
                    glswGetShader(shaderCompileName[type].c_str(),
                                  lineCountOffset[type], _refreshStage[type]);
                // GLSW may fail for various reasons (not a valid effect stage,
                // invalid name, etc)
                if (sourceCode) {
                    // If reading was successful, grab the entire code in a
                    // string
                    stringImpl codeString(sourceCode);
                    // And replace in place with our program's headers created
                    // earlier
                    Util::replaceStringInPlace(
                        codeString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                    Util::replaceStringInPlace(codeString,
                                               "//__CUSTOM_UNIFORMS__",
                                               shaderSourceUniforms[type]);
                    // Load our shader from the final string and save it in the
                    // manager in case a new Shader Program needs it
                    _shaderStage[type] =
                        ShaderManager::getInstance().loadShader(
                            shaderCompileName[type], codeString, type,
                            _refreshStage[type]);
                }
            }
            // Show a message, in debug, if we don't have a shader for this
            // stage
            if (!_shaderStage[type]) {
                Console::d_printfn(Locale::get("WARN_GLSL_SHADER_LOAD"),
                                   shaderCompileName[type].c_str());
            } else {
                // Try to compile the shader (it doesn't double compile shaders,
                // so it's safe to call it multiple types)
                if (!_shaderStage[type]->compile()) {
                    Console::errorfn(Locale::get("ERROR_GLSL_SHADER_COMPILE"),
                                     _shaderStage[type]->getShaderID());
                }
            }
        }
    }

    // try to link the program in a separate thread
    return GFX_DEVICE.loadInContext(/*_threadedLoading && !_loadedFromBinary ?
                                       GFX_LOADING_CONTEXT : */
                                    GFX_RENDERING_CONTEXT,
                                    DELEGATE_BIND(
                                        &glShaderProgram::threadedLoad, this,
                                        name));
}

/// Check every possible combination of flags to make sure this program can be
/// used for rendering
bool glShaderProgram::isValid() const {
    return isHWInitComplete() && _linked && _shaderProgramID != 0 &&
           _shaderProgramID != GLUtil::_invalidObjectID;
}

/// Cache uniform/attribute locations for shader programs
/// When we call this function, we check our name<->address map to see if we
/// queried the location before
/// If we didn't, ask the GPU to give us the variables address and save it for
/// later use
GLint glShaderProgram::cachedLoc(const stringImpl& name) {
    // If the shader can't be used for rendering, just return an invalid address
    if (!isValid()) {
        return -1;
    }

    DIVIDE_ASSERT(_threadedLoadComplete,
                  "glShaderProgram error: tried to query a shader program "
                  "before threaded load completed!");

    // Check the cache for the location
    ShaderVarMap::const_iterator it = _shaderVars.find(name);
    if (it != std::end(_shaderVars)) {
        return it->second;
    }

    // Cache miss. Query OpenGL for the location
    GLint location = glGetUniformLocation(_shaderProgramID, name.c_str());

    // Save it for later reference
    hashAlg::emplace(_shaderVars, name, location);

    // Return the location
    return location;
}

/// Bind this shader program
bool glShaderProgram::bind() {
    // Prevent double bind
    if (_bound) {
        return true;
    }
    // If the shader isn't ready or failed to link, stop here
    if (!isValid()) {
        return false;
    }
    // Set this program as the currently active one
    GL_API::setActiveProgram(this);
    // Pass the rest of the binding responsibilities to the parent class
    return ShaderProgram::bind();
}

/// Unbinding this program, unless forced, just clears the _bound flag
void glShaderProgram::unbind(bool resetActiveProgram) {
    // Prevent double unbind
    if (!_bound) {
        return;
    }
    // After using the shader at least once, validate the shader if needed
    if (!_validated) {
        _validationQueued = isValid();
    }
    // If forced to do so, we can clear OpenGL's active program object
    if (resetActiveProgram) {
        GL_API::setActiveProgram(nullptr);
    }
    // Pass the rest of the unbind responsibilities to the parent class
    ShaderProgram::unbind(resetActiveProgram);
}

/// This is used to set all of the subroutine indices for the specified shader
/// stage for this program
void glShaderProgram::SetSubroutines(ShaderType type,
                                     const vectorImpl<U32>& indices) const {
    // The shader must be bound before calling this!
    DIVIDE_ASSERT(_bound && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");
    // Validate data and send to GPU
    if (!indices.empty() && indices[0] != GLUtil::_invalidObjectID) {
        glUniformSubroutinesuiv(_shaderStageTable[type],
                                (GLsizei)indices.size(), indices.data());
    }
}
/// This works exactly like SetSubroutines, but for a single index.
/// If the shader has multiple subroutine uniforms, this will reset the rest!!!
void glShaderProgram::SetSubroutine(ShaderType type, U32 index) const {
    DIVIDE_ASSERT(_bound && isValid(),
                  "glShaderProgram error: tried to set subroutines on an "
                  "unbound or unlinked program!");

    if (index != GLUtil::_invalidObjectID) {
        U32 value[] = {index};
        glUniformSubroutinesuiv(_shaderStageTable[type], 1, value);
    }
}

/// Returns the number of subroutine uniforms for the specified shader stage
U32 glShaderProgram::GetSubroutineUniformCount(ShaderType type) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    I32 subroutineCount = 0;
    glGetProgramStageiv(_shaderProgramID, _shaderStageTable[type],
                        GL_ACTIVE_SUBROUTINE_UNIFORMS, &subroutineCount);

    return std::max(subroutineCount, 0);
}
/// Get the uniform location of the specified subroutine uniform for the
/// specified stage. Not cached!
U32 glShaderProgram::GetSubroutineUniformLocation(
    ShaderType type, const stringImpl& name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineUniformLocation(
        _shaderProgramID, _shaderStageTable[type], name.c_str());
}

/// Get the index of the specified subroutine name for the specified stage. Not
/// cached!
U32 glShaderProgram::GetSubroutineIndex(ShaderType type,
                                        const stringImpl& name) const {
    DIVIDE_ASSERT(isValid(),
                  "glShaderProgram error: tried to query subroutines on an "
                  "invalid program!");

    return glGetSubroutineIndex(_shaderProgramID, _shaderStageTable[type],
                                name.c_str());
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, GLuint value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform1ui(_shaderProgramID, location, value);
    } else {
        glUniform1ui(location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, GLint value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform1i(_shaderProgramID, location, value);
    } else {
        glUniform1i(location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, GLfloat value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform1f(_shaderProgramID, location, value);
    } else {
        glUniform1f(location, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vec2<GLfloat>& value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform2fv(_shaderProgramID, location, 1, value);
    } else {
        glUniform2fv(location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const vec2<GLint>& value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform2iv(_shaderProgramID, location, 1, value);
    } else {
        glUniform2iv(location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vec2<GLushort>& value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform2iv(_shaderProgramID, location, 1,
                            vec2<I32>(value.x, value.y));
    } else {
        glUniform2iv(location, 1, vec2<I32>(value.x, value.y));
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vec3<GLfloat>& value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform3fv(_shaderProgramID, location, 1, value);
    } else {
        glUniform3fv(location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vec4<GLfloat>& value) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform4fv(_shaderProgramID, location, 1, value);
    } else {
        glUniform4fv(location, 1, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const mat3<GLfloat>& value,
                              bool rowMajor) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniformMatrix3fv(_shaderProgramID, location, 1,
                                  rowMajor ? GL_TRUE : GL_FALSE,
                                  value.mat);
    } else {
        glUniformMatrix3fv(location, 1, rowMajor ? GL_TRUE : GL_FALSE, value);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location, const mat4<GLfloat>& value,
                              bool rowMajor) const {
    if (location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniformMatrix4fv(_shaderProgramID, location, 1,
                                  rowMajor ? GL_TRUE : GL_FALSE,
                                  value.mat);
    } else {
        glUniformMatrix4fv(location, 1, rowMajor ? GL_TRUE : GL_FALSE, value.mat);
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<GLint>& values) const {
    if (values.empty()) {
        return;
    }

    if (!_bound) {
        glProgramUniform1iv(_shaderProgramID, location, (GLsizei)values.size(),
                            values.data());
    } else {
        glUniform1iv(location, (GLsizei)values.size(), values.data());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<GLfloat>& values) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform1fv(_shaderProgramID, location, (GLsizei)values.size(),
                            values.data());
    } else {
        glUniform1fv(location, (GLsizei)values.size(), values.data());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<vec2<GLfloat> >& values) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform2fv(_shaderProgramID, location, (GLsizei)values.size(),
                            values.front());
    } else {
        glUniform2fv(location, (GLsizei)values.size(), values.front());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<vec3<GLfloat> >& values) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform3fv(_shaderProgramID, location, (GLsizei)values.size(),
                            values.front());
    } else {
        glUniform3fv(location, (GLsizei)values.size(), values.front());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<vec4<GLfloat> >& values) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniform4fv(_shaderProgramID, location, (GLsizei)values.size(),
                            values.front());
    } else {
        glUniform4fv(location, (GLsizei)values.size(), values.front());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<mat3<F32> >& values,
                              bool rowMajor) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniformMatrix3fv(_shaderProgramID, location,
                                  (GLsizei)values.size(),
                                  rowMajor ? GL_TRUE : GL_FALSE,
                                  values.front());
    } else {
        glUniformMatrix3fv(location, (GLsizei)values.size(),
                           rowMajor ? GL_TRUE : GL_FALSE,
                           values.front());
    }
}

/// Set an uniform value
void glShaderProgram::Uniform(GLint location,
                              const vectorImpl<mat4<GLfloat> >& values,
                              bool rowMajor) const {
    if (values.empty() || location == -1) {
        return;
    }

    if (!_bound) {
        glProgramUniformMatrix4fv(_shaderProgramID, location,
                                  (GLsizei)values.size(),
                                  rowMajor ? GL_TRUE : GL_FALSE,
                                  values.front());
    } else {
        glUniformMatrix4fv(location, (GLsizei)values.size(),
                           rowMajor ? GL_TRUE : GL_FALSE,
                           values.front());
    }
}

/// Bind a sampler to specific texture unit, checking double bind to avoid
/// programming errors in debug
void glShaderProgram::UniformTexture(GLint location, GLushort slot) {
    if (location == -1 || !checkSlotUsage(location, slot)) {
        return;
    }

    if (!_bound) {
        glProgramUniform1i(_shaderProgramID, location, slot);
    } else {
        glUniform1i(location, slot);
    }
}

/// If a slot is assigned to multiple locations, prevent binding it and print an
/// error. Only in Debug builds!
bool glShaderProgram::checkSlotUsage(GLint location, GLushort slot) {
#ifdef _DEBUG
    const TextureSlotMap::const_iterator& it = _textureSlots.find(slot);
    if (it == std::end(_textureSlots) || it->second == location) {
        _textureSlots[slot] = location;
        return true;
    }
    Console::errorfn(Locale::get("ERROR_SHADER_TEXTURE_MULTI_BIND"));
    return false;
#else
    return true;
#endif
}
};