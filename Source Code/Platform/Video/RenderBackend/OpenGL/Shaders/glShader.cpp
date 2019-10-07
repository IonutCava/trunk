#include "stdafx.h"

#include "Headers/glShader.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {

    size_t g_validationBufferMaxSize = 4096 * 16;

    ShaderType getShaderType(UseProgramStageMask mask) {
        if (BitCompare(to_U32(mask), UseProgramStageMask::GL_VERTEX_SHADER_BIT)) {
            return ShaderType::VERTEX;
        } else if (BitCompare(to_U32(mask), UseProgramStageMask::GL_TESS_CONTROL_SHADER_BIT)) {
            return ShaderType::TESSELLATION_CTRL;
        } else if (BitCompare(to_U32(mask), UseProgramStageMask::GL_TESS_EVALUATION_SHADER_BIT)) {
            return ShaderType::TESSELLATION_EVAL;
        } else if (BitCompare(to_U32(mask), UseProgramStageMask::GL_GEOMETRY_SHADER_BIT)) {
            return ShaderType::GEOMETRY;
        } else if (BitCompare(to_U32(mask), UseProgramStageMask::GL_FRAGMENT_SHADER_BIT)) {
            return ShaderType::FRAGMENT;
        } else if (BitCompare(to_U32(mask), UseProgramStageMask::GL_COMPUTE_SHADER_BIT)) {
            return ShaderType::COMPUTE;
        }
        
        // Multiple stages!
        return ShaderType::COUNT;
    }

    UseProgramStageMask getStageMask(ShaderType type) {
        switch (type) {
            case ShaderType::VERTEX: return UseProgramStageMask::GL_VERTEX_SHADER_BIT;
            case ShaderType::TESSELLATION_CTRL: return UseProgramStageMask::GL_TESS_CONTROL_SHADER_BIT;
            case ShaderType::TESSELLATION_EVAL: return UseProgramStageMask::GL_TESS_EVALUATION_SHADER_BIT;
            case ShaderType::GEOMETRY: return UseProgramStageMask::GL_GEOMETRY_SHADER_BIT;
            case ShaderType::FRAGMENT: return UseProgramStageMask::GL_FRAGMENT_SHADER_BIT;
            case ShaderType::COMPUTE: return UseProgramStageMask::GL_COMPUTE_SHADER_BIT;
        }

        return UseProgramStageMask::GL_NONE_BIT;
    }
};

SharedMutex glShader::_shaderNameLock;
glShader::ShaderMap glShader::_shaderNameMap;

glShader::glShader(GFXDevice& context, const stringImpl& name)
    : TrackedObject(),
      GraphicsResource(context, GraphicsResource::Type::SHADER, getGUID(), _ID(name.c_str())),
      glObject(glObjectType::TYPE_SHADER, context),
      _stageMask(UseProgramStageMask::GL_NONE_BIT),
      _stageCount(0),
      _valid(false),
      _shouldRecompile(false),
      _loadedFromBinary(false),
      _binaryFormat(GL_NONE),
      _programHandle(GLUtil::k_invalidObjectID),
      _name(name)
{
}

glShader::~glShader() {
    if (_programHandle != GLUtil::k_invalidObjectID) {
        Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), name().c_str());
        GL_API::deleteShaderPrograms(1, &_programHandle);
    }
}

bool glShader::embedsType(ShaderType type) const {
    return BitCompare(to_U32(stageMask()), getStageMask(type));
}

bool glShader::uploadToGPU(bool& previouslyUploaded) {
    // prevent double load
    if (_valid) {
        previouslyUploaded = true;
        return true;
    }

    previouslyUploaded = false;

    Console::d_printfn(Locale::get(_ID("GLSL_LOAD_PROGRAM")), _name.c_str(), getGUID());
    if (!loadFromBinary()) {
        vectorEASTL<const char*> cstrings;
        if (_stageCount == 1) {
            const U8 shaderIdx = to_base(getShaderType(_stageMask));
            const vectorEASTL<stringImpl>& src = _sourceCode[shaderIdx];
            cstrings.reserve(src.size());
            eastl::transform(eastl::cbegin(src), eastl::cend(src),
                             eastl::back_inserter(cstrings), std::mem_fn(&stringImpl::c_str));

            if (_programHandle != GLUtil::k_invalidObjectID) {
                GL_API::deleteShaderPrograms(1, &_programHandle);
            }

            _programHandle = glCreateShaderProgramv(GLUtil::glShaderStageTable[shaderIdx], (GLsizei)cstrings.size(), cstrings.data());
            if (_programHandle == 0 || _programHandle == GLUtil::k_invalidObjectID) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_CREATE_PROGRAM")), _name.c_str());
                _valid = false;
                return false;
            }
        } else {
            if (_programHandle == GLUtil::k_invalidObjectID) {
                _programHandle = glCreateProgram();
            }
            if (_programHandle == 0 || _programHandle == GLUtil::k_invalidObjectID) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_CREATE_PROGRAM")), _name.c_str());
                _valid = false;
                return false;
            }

            vectorEASTL<GLuint> shaders;
            shaders.reserve(to_base(ShaderType::COUNT));

            for (U8 i = 0; i < to_base(ShaderType::COUNT); ++i) {
                const vectorEASTL<stringImpl>& src = _sourceCode[i];
                if (!src.empty()) {
                    cstrings.resize(0);
                    cstrings.reserve(src.size());
                    eastl::transform(eastl::cbegin(src), eastl::cend(src),
                                     eastl::back_inserter(cstrings), std::mem_fn(&stringImpl::c_str));

                    const GLuint shader = glCreateShader(GLUtil::glShaderStageTable[i]);
                    if (shader) {
                        glShaderSource(shader, (GLsizei)cstrings.size(), cstrings.data(), NULL);
                        glCompileShader(shader);

                        GLboolean compiled = 0;
                        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                        if (compiled == GL_FALSE) {
                            // error
                            GLint logSize = 0;
                            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
                            stringImpl validationBuffer = "";
                            validationBuffer.resize(logSize);

                            glGetShaderInfoLog(shader, logSize, &logSize, &validationBuffer[0]);
                            if (validationBuffer.size() > g_validationBufferMaxSize) {
                                // On some systems, the program's disassembly is printed, and that can get quite large
                                validationBuffer.resize(std::strlen(Locale::get(_ID("ERROR_GLSL_COMPILE"))) * 2 + g_validationBufferMaxSize);
                                // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
                                validationBuffer.append(" ... ");
                            }

                            Console::errorfn(Locale::get(_ID("ERROR_GLSL_COMPILE")), _name.c_str(), shader, GLUtil::glShaderStageNameTable[i].c_str(), validationBuffer.c_str());

                            glDeleteShader(shader);
                        } else {
                            shaders.push_back(shader);
                        }
                    }
                }
            }

            for (GLuint shader : shaders) {
                glAttachShader(_programHandle, shader);
            }

            glProgramParameteri(_programHandle, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
            glProgramParameteri(_programHandle, GL_PROGRAM_SEPARABLE, GL_TRUE);
            glLinkProgram(_programHandle);

            for (GLuint shader : shaders) {
                glDetachShader(_programHandle, shader);
                glDeleteShader(shader);
            }
            shaders.clear();
        }

        // And check the result
        GLboolean linkStatus = GL_FALSE;
        glGetProgramiv(_programHandle, GL_LINK_STATUS, &linkStatus);

        // If linking failed, show an error, else print the result in debug builds.
        if (linkStatus == GL_FALSE) {
            GLint logSize = 0;
            glGetProgramiv(_programHandle, GL_INFO_LOG_LENGTH, &logSize);
            stringImpl validationBuffer = "";
            validationBuffer.resize(logSize);

            glGetProgramInfoLog(_programHandle, logSize, NULL, &validationBuffer[0]);
            if (validationBuffer.size() > g_validationBufferMaxSize) {
                // On some systems, the program's disassembly is printed, and that can get quite large
                validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
                // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
                validationBuffer.append(" ... ");
            }

            Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")), _name.c_str(), validationBuffer.c_str(), getGUID());
        } else {
            if (Config::ENABLE_GPU_VALIDATION) {
                Console::printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG_OK")), _name.c_str(), "[OK]", getGUID(), _programHandle);
                glObjectLabel(GL_PROGRAM, _programHandle, -1, _name.c_str());
            }

            _valid = true;
        }
    }
    _sourceCode.fill({});

    return _valid;
}

bool glShader::load(const ShaderLoadData& data) {
    bool hasSourceCode = false;

    // Reset all state first
    {
        _valid = false;
        _stageCount = 0;
        _stageMask = UseProgramStageMask::GL_NONE_BIT;
        _sourceCode.fill({});
        _usedAtoms.clear();
        _shaderVarLocation.clear();
    }

    for (const LoadData& it : data) {
        if (it._type != ShaderType::COUNT) {
            _stageMask |= getStageMask(it._type);
            _stageCount++;
            for (const stringImpl& source : it.sourceCode) {
                _sourceCode[to_base(it._type)].push_back(source);
                if (!source.empty()) {
                    hasSourceCode = true;
                }
            }
            for (auto atomIt : it.atoms) {
                _usedAtoms.insert(atomIt);
            }
        }
    }

    if (data.empty() || !hasSourceCode) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")), name().c_str());
        return false;
    }

#if 0
    bool previouslyUploaded = false;
    return uploadToGPU(previouslyUploaded);
#else
    return true;
#endif
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void glShader::removeShader(glShader* s) {
    assert(s != nullptr);

    // Try to find it
    U64 nameHash = s->nameHash();
    UniqueLockShared w_lock(_shaderNameLock);
    ShaderMap::iterator it = _shaderNameMap.find(nameHash);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it.
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader (as in leave it in the object arena)
            _shaderNameMap.erase(nameHash);
        }
    }
}

/// Return a new shader reference
glShader* glShader::getShader(const stringImpl& name) {
    // Try to find the shader
    SharedLock r_lock(_shaderNameLock);
    ShaderMap::iterator it = _shaderNameMap.find(_ID(name.c_str()));
    if (it != std::end(_shaderNameMap)) {
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
glShader* glShader::loadShader(GFXDevice& context,
                               const stringImpl& name,
                               const ShaderLoadData& data) {
    // See if we have the shader already loaded
    glShader* shader = getShader(name);

    bool newShader = false;
    // If we do, and don't need a recompile, just return it
    if (shader == nullptr) {
        // If we can't find it, we create a new one
        context.lockObjectArena();
        shader = new (context.objectArena()) glShader(context, name);
        context.objectArena().DTOR(shader);
        context.unlockObjectArena();

        newShader = true;
    }

    return loadShader(context, shader, newShader, data);
}


glShader* glShader::loadShader(GFXDevice & context,
                              glShader * shader,
                              bool isNew,
                              const ShaderLoadData & data) {

    // At this stage, we have a valid Shader object, so load the source code
    if (shader->load(data)) {
        if (isNew) {
            // If we loaded the source code successfully,  register it
            UniqueLockShared w_lock(_shaderNameLock);
            _shaderNameMap.insert({ shader->nameHash(), shader });
        }
    }//else ignore. it's somewhere in the object arena

    return shader;
}

bool glShader::loadFromBinary() {
    _loadedFromBinary = false;

    // Load the program from the binary file, if available and allowed, to avoid linking.
    if (ShaderProgram::useShaderBinaryCache()) {
        // Load the program's binary format from file
        vector<Byte> data;
        if (readFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin, glShaderProgram::decorateFileName(_name) + ".fmt", data, FileType::BINARY) && !data.empty()) {
            _binaryFormat = *reinterpret_cast<GLenum*>(data.data());
            if (_binaryFormat != GL_NONE) {
                data.resize(0);
                if (readFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin, glShaderProgram::decorateFileName(_name) + ".bin", data, FileType::BINARY) && !data.empty()) {
                    // Load binary code on the GPU
                    _programHandle = glCreateProgram();
                    glProgramBinary(_programHandle, _binaryFormat, (bufferPtr)data.data(), (GLint)data.size());
                    // Check if the program linked successfully on load
                    GLboolean success = 0;
                    glGetProgramiv(_programHandle, GL_LINK_STATUS, &success);
                    // If it loaded properly set all appropriate flags (this also prevents low level access to the program's shaders)
                    if (success == GL_TRUE) {
                        _loadedFromBinary = true;
                        _valid = true;
                    }
                }
            }
        }
    }

    return _loadedFromBinary;
}

void glShader::dumpBinary() {
    if (!valid()) {
        return;
    }

    // Get the size of the binary code
    GLint binaryLength = 0;
    glGetProgramiv(_programHandle, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    // allocate a big enough buffer to hold it
    char* binary = MemoryManager_NEW char[binaryLength];
    DIVIDE_ASSERT(binary != NULL, "glShader error: could not allocate memory for the program binary!");

    // and fill the buffer with the binary code
    glGetProgramBinary(_programHandle, binaryLength, NULL, &_binaryFormat, binary);
    if (_binaryFormat != GL_NONE) {
        // dump the buffer to file
        if (writeFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin,
                      glShaderProgram::decorateFileName(_name) + ".bin",
                      binary,
                      (size_t)binaryLength,
                      FileType::BINARY))
        {
            // dump the format to a separate file (highly non-optimised. Should dump formats to a database instead)
            writeFile(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin,
                      glShaderProgram::decorateFileName(_name) + ".fmt",
                      &_binaryFormat,
                      sizeof(GLenum),
                      FileType::BINARY);
        }
    }

    // delete our local code buffer
    MemoryManager::DELETE(binary);
}


/// Cache uniform/attribute locations for shader programs
/// When we call this function, we check our name<->address map to see if we queried the location before
/// If we didn't, ask the GPU to give us the variables address and save it for later use
I32 glShader::binding(const char* name, U64 bindingHash) {
    // If the shader can't be used for rendering, just return an invalid address
    if (_programHandle == 0 || _programHandle == GLUtil::k_invalidObjectID || !valid()) {
        return -1;
    }

    // Check the cache for the location
    ShaderVarMap::const_iterator it = _shaderVarLocation.find(bindingHash);
    if (it != std::end(_shaderVarLocation)) {
        return it->second;
    }

    // Cache miss. Query OpenGL for the location
    GLint location = glGetUniformLocation(_programHandle, name);

    // Save it for later reference
    hashAlg::insert(_shaderVarLocation, bindingHash, location);

    // Return the location
    return location;
}

I32 glShader::cachedValueUpdate(const GFX::PushConstant& constant, bool force) {
    if (constant._type == GFX::PushConstantType::COUNT || constant._bindingHash == 0u) {
        return -1;
    }
    assert(!constant._binding.empty());

    const U64 locationHash = constant._bindingHash;

    const I32 bindingLoc = binding(constant._binding.c_str(), locationHash);

    if (bindingLoc == -1 || _programHandle == 0 || _programHandle == GLUtil::k_invalidObjectID) {
        return -1;
    }

    UniformsByNameHash::ShaderVarMap& map = _uniformsByNameHash._shaderVars;
    UniformsByNameHash::ShaderVarMap::iterator it = map.find(locationHash);
    if (it != std::cend(map)) {
        if (!force && it->second == constant) {
            return -1;
        } else {
            it->second = constant;
        }
    } else {
        hashAlg::emplace(map, locationHash, constant);
    }

    return bindingLoc;
}

void glShader::UploadPushConstant(const GFX::PushConstant& constant, bool force) {
    I32 binding = cachedValueUpdate(constant, force);

    if (binding != -1) {
        Uniform(binding, constant._type, constant._buffer, constant._flag);
    }
}

void glShader::reuploadUniforms(bool force) {
    for (const UniformsByNameHash::ShaderVarMap::value_type& it : _uniformsByNameHash._shaderVars) {
        UploadPushConstant(it.second, force);
    }
}

void glShader::Uniform(I32 binding, GFX::PushConstantType type, const vectorEASTL<char>& values, bool flag) const {
    GLsizei byteCount = (GLsizei)values.size();
    switch (type) {
        case GFX::PushConstantType::BOOL:
            glProgramUniform1iv(_programHandle, binding, byteCount / (1 * sizeof(GLbyte)), (GLint*)castData<GLbyte, 1, char>(values));
            break;
        case GFX::PushConstantType::INT:
            glProgramUniform1iv(_programHandle, binding, byteCount / (1 * sizeof(GLint)), castData<GLint, 1, I32>(values));
            break;
        case GFX::PushConstantType::UINT:
            glProgramUniform1uiv(_programHandle, binding, byteCount / (1 * sizeof(GLuint)), castData<GLuint, 1, U32>(values));
            break;
        case GFX::PushConstantType::DOUBLE:
            glProgramUniform1dv(_programHandle, binding, byteCount / (1 * sizeof(GLdouble)), castData<GLdouble, 1, D64>(values));
            break;
        case GFX::PushConstantType::FLOAT:
            glProgramUniform1fv(_programHandle, binding, byteCount / (1 * sizeof(GLfloat)), castData<GLfloat, 1, F32>(values));
            break;
        case GFX::PushConstantType::IVEC2:
            glProgramUniform2iv(_programHandle, binding, byteCount / (2 * sizeof(GLint)), castData<GLint, 2, vec2<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC3:
            glProgramUniform3iv(_programHandle, binding, byteCount / (3 * sizeof(GLint)), castData<GLint, 3, vec3<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC4:
            glProgramUniform4iv(_programHandle, binding, byteCount / (4 * sizeof(GLint)), castData<GLint, 4, vec4<I32>>(values));
            break;
        case GFX::PushConstantType::UVEC2:
            glProgramUniform2uiv(_programHandle, binding, byteCount / (2 * sizeof(GLuint)), castData<GLuint, 2, vec2<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC3:
            glProgramUniform3uiv(_programHandle, binding, byteCount / (3 * sizeof(GLuint)), castData<GLuint, 3, vec3<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC4:
            glProgramUniform4uiv(_programHandle, binding, byteCount / (4 * sizeof(GLuint)), castData<GLuint, 4, vec4<U32>>(values));
            break;
        case GFX::PushConstantType::DVEC2:
            glProgramUniform2dv(_programHandle, binding, byteCount / (2 * sizeof(GLdouble)), castData<GLdouble, 2, vec2<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC3:
            glProgramUniform3dv(_programHandle, binding, byteCount / (3 * sizeof(GLdouble)), castData<GLdouble, 3, vec3<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC4:
            glProgramUniform4dv(_programHandle, binding, byteCount / (4 * sizeof(GLdouble)), castData<GLdouble, 4, vec4<D64>>(values));
            break;
        case GFX::PushConstantType::VEC2:
            glProgramUniform2fv(_programHandle, binding, byteCount / (2 * 4), castData<GLfloat, 2, vec2<F32>>(values));
            break;
        case GFX::PushConstantType::VEC3:
            glProgramUniform3fv(_programHandle, binding, byteCount / (3 * 4), castData<GLfloat, 3, vec3<F32>>(values));
            break;
        case GFX::PushConstantType::FCOLOUR3:
            glProgramUniform3fv(_programHandle, binding, byteCount / (3 * 4), castData<GLfloat, 3, FColour3>(values));
            break;
        case GFX::PushConstantType::VEC4:
            glProgramUniform4fv(_programHandle, binding, byteCount / (4 * 4), castData<GLfloat, 4, vec4<F32>>(values));
            break;
        case GFX::PushConstantType::FCOLOUR4:
            glProgramUniform3fv(_programHandle, binding, byteCount / (4 * 4), castData<GLfloat, 4, FColour4>(values));
            break;
        case GFX::PushConstantType::IMAT2:
            glProgramUniformMatrix2fv(_programHandle, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT3:
            glProgramUniformMatrix3fv(_programHandle, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT4:
            glProgramUniformMatrix4fv(_programHandle, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<I32>>(values));
            break;
        case GFX::PushConstantType::UMAT2:
            glProgramUniformMatrix2fv(_programHandle, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT3:
            glProgramUniformMatrix3fv(_programHandle, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT4:
            glProgramUniformMatrix4fv(_programHandle, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<U32>>(values));
            break;
        case GFX::PushConstantType::MAT2:
            glProgramUniformMatrix2fv(_programHandle, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<F32>>(values));
            break;
        case GFX::PushConstantType::MAT3:
            glProgramUniformMatrix3fv(_programHandle, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<F32>>(values));
            break;
        case GFX::PushConstantType::MAT4:
            glProgramUniformMatrix4fv(_programHandle, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<F32>>(values));
            break;
        case GFX::PushConstantType::DMAT2:
            glProgramUniformMatrix2dv(_programHandle, binding, byteCount / (2 * 2 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 4, mat2<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT3:
            glProgramUniformMatrix3dv(_programHandle, binding, byteCount / (3 * 3 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 9, mat3<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT4:
            glProgramUniformMatrix4dv(_programHandle, binding, byteCount / (4 * 4 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 16, mat4<D64>>(values));
            break;
        default:
            DIVIDE_ASSERT(false, "glShaderProgram::Uniform error: Unhandled data type!");
    }
}

/// Add a define to the shader. The defined must not have been added previously
void glShader::addShaderDefine(const stringImpl& define, bool appendPrefix) {
    // Find the string in the list of program defines
    auto it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, appendPrefix));
    // If we can't find it, we add it
    if (it == std::end(_definesList)) {
        _definesList.push_back(std::make_pair(define, appendPrefix));
        _shouldRecompile = true;
    } else {
        // If we did find it, we'll show an error message in debug builds about double add
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_ADD")), define.c_str(), _name.c_str());
    }
}

/// Remove a define from the shader. The defined must have been added previously
void glShader::removeShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    auto it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, true));
        if (it == std::end(_definesList)) {
            it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, false));
        }
    // If we find it, we remove it
    if (it != std::end(_definesList)) {
        _definesList.erase(it);
        _shouldRecompile = true;
    } else {
        // If we did not find it, we'll show an error message in debug builds
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_DELETE")), define.c_str(), _name.c_str());
    }
}

};