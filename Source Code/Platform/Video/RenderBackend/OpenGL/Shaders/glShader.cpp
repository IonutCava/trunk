#include "stdafx.h"

#include "Headers/glShader.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {

    size_t g_validationBufferMaxSize = 4096 * 16;

    ShaderType getShaderType(const UseProgramStageMask mask) noexcept {
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

    UseProgramStageMask getStageMask(const ShaderType type) noexcept {
        switch (type) {
            case ShaderType::VERTEX: return UseProgramStageMask::GL_VERTEX_SHADER_BIT;
            case ShaderType::TESSELLATION_CTRL: return UseProgramStageMask::GL_TESS_CONTROL_SHADER_BIT;
            case ShaderType::TESSELLATION_EVAL: return UseProgramStageMask::GL_TESS_EVALUATION_SHADER_BIT;
            case ShaderType::GEOMETRY: return UseProgramStageMask::GL_GEOMETRY_SHADER_BIT;
            case ShaderType::FRAGMENT: return UseProgramStageMask::GL_FRAGMENT_SHADER_BIT;
            case ShaderType::COMPUTE: return UseProgramStageMask::GL_COMPUTE_SHADER_BIT;
            default: break;
        }

        return UseProgramStageMask::GL_NONE_BIT;
    }

    template<typename T_out, size_t T_out_count, typename T_in>
    std::pair<GLsizei, const T_out*> convertData(const GLsizei byteCount, const Byte* const values) noexcept {
        static_assert(sizeof(T_out) * T_out_count == sizeof(T_in), "Invalid cast data");

        const GLsizei size = byteCount / sizeof(T_in);
        return { size, (const T_out*)(values) };
    }
};

SharedMutex glShader::_shaderNameLock;
glShader::ShaderMap glShader::_shaderNameMap;

glShader::glShader(GFXDevice& context, const Str256& name)
    : GUIDWrapper(),
      GraphicsResource(context, Type::SHADER, getGUID(), _ID(name.c_str())),
      glObject(glObjectType::TYPE_SHADER, context),
      _loadedFromBinary(false),
      _binaryFormat(GL_NONE),
      _programHandle(GLUtil::k_invalidObjectID),
      _stageCount(0),
      _stageMask(UseProgramStageMask::GL_NONE_BIT),
      _name(name)
{
    std::atomic_init(&_refCount, 0);
}

glShader::~glShader() {
    if (_programHandle != GLUtil::k_invalidObjectID) {
        Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), name().c_str());
        GL_API::deleteShaderPrograms(1, &_programHandle);
    }
}

bool glShader::embedsType(const ShaderType type) const {
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

            _programHandle = glCreateShaderProgramv(GLUtil::glShaderStageTable[shaderIdx], static_cast<GLsizei>(cstrings.size()), cstrings.data());
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
                    if (shader != 0u) {
                        glShaderSource(shader, static_cast<GLsizei>(cstrings.size()), cstrings.data(), NULL);
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

                            Console::errorfn(Locale::get(_ID("ERROR_GLSL_COMPILE")), _name.c_str(), shader, Names::shaderTypes[i], validationBuffer.c_str());

                            glDeleteShader(shader);
                        } else {
                            shaders.push_back(shader);
                        }
                    }
                }
            }

            if (!shaders.empty()) {
                for (const GLuint shader : shaders) {
                    glAttachShader(_programHandle, shader);
                }

                glProgramParameteri(_programHandle, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
                glProgramParameteri(_programHandle, GL_PROGRAM_SEPARABLE, GL_TRUE);
                glLinkProgram(_programHandle);

                for (const GLuint shader : shaders) {
                    glDetachShader(_programHandle, shader);
                    glDeleteShader(shader);
                }
                shaders.clear();
            }
        }

        // And check the result
        GLboolean linkStatus = GL_FALSE;
        glGetProgramiv(_programHandle, GL_LINK_STATUS, &linkStatus);

        // If linking failed, show an error, else print the result in debug builds.
        if (linkStatus == GL_FALSE) {
            GLint logSize = 0;
            glGetProgramiv(_programHandle, GL_INFO_LOG_LENGTH, &logSize);
            stringImpl validationBuffer;
            validationBuffer.resize(logSize);

            glGetProgramInfoLog(_programHandle, logSize, nullptr, &validationBuffer[0]);
            if (validationBuffer.size() > g_validationBufferMaxSize) {
                // On some systems, the program's disassembly is printed, and that can get quite large
                validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
                // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
                validationBuffer.append(" ... ");
            }

            Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")), _name.c_str(), validationBuffer.c_str(), getGUID());
        } else {
            if_constexpr(Config::ENABLE_GPU_VALIDATION) {
                Console::printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG_OK")), _name.c_str(), "[OK]", getGUID(), _programHandle);
                glObjectLabel(GL_PROGRAM, _programHandle, -1, _name.c_str());
            }

            _valid = true;
        }
    }
    _sourceCode.fill({});

    if (_valid) {
        cacheActiveUniforms();
    }

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

    if (!hasSourceCode) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")), name().c_str());
        return false;
    }

    return true;
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void glShader::removeShader(glShader* s) {
    assert(s != nullptr);

    // Try to find it
    const U64 nameHash = s->nameHash();
    UniqueLock<SharedMutex> w_lock(_shaderNameLock);
    const ShaderMap::iterator it = _shaderNameMap.find(nameHash);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it.
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader (as in leave it in the object arena)
            _shaderNameMap.erase(nameHash);
        }
    }
}

/// Return a new shader reference
glShader* glShader::getShader(const Str256& name) {
    // Try to find the shader
    SharedLock<SharedMutex> r_lock(_shaderNameLock);
    const ShaderMap::iterator it = _shaderNameMap.find(_ID(name.c_str()));
    if (it != std::end(_shaderNameMap)) {
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
glShader* glShader::loadShader(GFXDevice& context,
                               const Str256& name,
                               const ShaderLoadData& data) {
    // See if we have the shader already loaded
    glShader* shader = getShader(name);

    bool newShader = false;
    // If we do, and don't need a recompile, just return it
    if (shader == nullptr) {
        // If we can't find it, we create a new one
        UniqueLock<Mutex> w_lock(context.objectArenaMutex());
        shader = new (context.objectArena()) glShader(context, name);
        context.objectArena().DTOR(shader);
        newShader = true;
    }

    return loadShader(context, shader, newShader, data);
}


glShader* glShader::loadShader(GFXDevice & context,
                              glShader * shader,
                              const bool isNew,
                              const ShaderLoadData & data) {

    // At this stage, we have a valid Shader object, so load the source code
    if (shader->load(data)) {
        if (isNew) {
            // If we loaded the source code successfully,  register it
            UniqueLock<SharedMutex> w_lock(_shaderNameLock);
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
        vectorEASTL<Byte> data;
        if (readFile((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin).c_str(),
                     (glShaderProgram::decorateFileName(_name) + ".fmt").c_str(),
                     data,
                     FileType::BINARY) && 
            !data.empty())
        {
            _binaryFormat = *reinterpret_cast<GLenum*>(data.data());
            if (_binaryFormat != GL_NONE) {
                data.resize(0);
                if (readFile((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin).c_str(),
                             (glShaderProgram::decorateFileName(_name) + ".bin").c_str(),
                             data,
                             FileType::BINARY) &&
                    !data.empty())
                {
                    // Load binary code on the GPU
                    _programHandle = glCreateProgram();
                    glProgramBinary(_programHandle, _binaryFormat, (bufferPtr)data.data(), static_cast<GLint>(data.size()));
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
    if (!valid() || _loadedFromBinary) {
        return;
    }

    // Get the size of the binary code
    GLint binaryLength = 0;
    glGetProgramiv(_programHandle, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    // allocate a big enough buffer to hold it
    char* binary = MemoryManager_NEW char[binaryLength];
    DIVIDE_ASSERT(binary != nullptr, "glShader error: could not allocate memory for the program binary!");

    // and fill the buffer with the binary code
    glGetProgramBinary(_programHandle, binaryLength, nullptr, &_binaryFormat, binary);
    if (_binaryFormat != GL_NONE) {
        // dump the buffer to file
        if (writeFile((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin).c_str(),
                      (glShaderProgram::decorateFileName(_name) + ".bin").c_str(),
                      binary,
                      static_cast<size_t>(binaryLength),
                      FileType::BINARY))
        {
            // dump the format to a separate file (highly non-optimised. Should dump formats to a database instead)
            writeFile((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin).c_str(),
                      (glShaderProgram::decorateFileName(_name) + ".fmt").c_str(),
                      &_binaryFormat,
                      sizeof(GLenum),
                      FileType::BINARY);
        }
    }

    // delete our local code buffer
    MemoryManager::DELETE(binary);
}

I32 glShader::cachedValueUpdate(const GFX::PushConstant& constant, const bool force) {
    if (constant._type != GFX::PushConstantType::COUNT && constant._bindingHash > 0u &&
        _programHandle != 0 && _programHandle != GLUtil::k_invalidObjectID && valid())
    {
        // Check the cache for the location
        const auto& locationIter = _shaderVarLocation.find(constant._bindingHash);
        if (locationIter != std::cend(_shaderVarLocation)) {
            UniformsByNameHash::ShaderVarMap& map = _uniformsByNameHash._shaderVars;
            const auto& constantIter = map.find(constant._bindingHash);
            if (constantIter != std::cend(map)) {
                if (force || constantIter->second != constant) {
                    constantIter->second = constant;
                } else {
                    return -1;
                }
            } else {
                hashAlg::emplace(map, constant._bindingHash, constant);
            }

            return locationIter->second;
        }
    }

    return -1;
}

void glShader::cacheActiveUniforms() {
    // If the shader can't be used for rendering, just return an invalid address
    if (_programHandle != 0 && _programHandle != GLUtil::k_invalidObjectID && valid()) {
        _shaderVarLocation.clear();
        GLint numActiveUniforms = 0;
        glGetProgramInterfaceiv(_programHandle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numActiveUniforms);

        vectorEASTL<GLchar> nameData(256);
        vectorEASTL<GLenum> properties;
        properties.push_back(GL_NAME_LENGTH);
        properties.push_back(GL_TYPE);
        properties.push_back(GL_ARRAY_SIZE);
        properties.push_back(GL_BLOCK_INDEX);
        properties.push_back(GL_LOCATION);

        vectorEASTL<GLint> values(properties.size());

        for (GLint attrib = 0; attrib < numActiveUniforms; ++attrib) {
            glGetProgramResourceiv(_programHandle, GL_UNIFORM, attrib, static_cast<GLsizei>(properties.size()), properties.data(), static_cast<GLsizei>(values.size()), nullptr, &values[0]);
            if (values[3] != -1 || values[4] == -1) {
                continue;
            }
            nameData.resize(values[0]); //The length of the name.
            glGetProgramResourceName(_programHandle, GL_UNIFORM, attrib, static_cast<GLsizei>(nameData.size()), nullptr, &nameData[0]);
            std::string name((char*)&nameData[0], nameData.size() - 1);

            if (name.length() > 3 && Util::GetTrailingCharacters(name, 3) == "[0]") {
                const GLint arraySize = values[2];
                // Array uniform. Use non-indexed version as an equally-valid alias
                name = name.substr(0, name.length() - 3);
                const GLint startLocation = values.back();
                for (GLint i = 0; i < arraySize; ++i) {
                    //Arrays and structs will be assigned sequentially increasing locations, starting with the given location
                    hashAlg::insert(_shaderVarLocation, _ID(Util::StringFormat("%s[%d]", name.c_str(), i).c_str()), startLocation + i);
                }
            }
             
            hashAlg::insert(_shaderVarLocation, _ID(name.c_str()), values.back());
        }
    }
}

void glShader::UploadPushConstant(const GFX::PushConstant& constant, const bool force) {
    const I32 binding = cachedValueUpdate(constant, force);
    Uniform(binding, constant._type, constant._buffer.data(), static_cast<GLsizei>(constant._buffer.size()), constant._flag);
}

void glShader::reuploadUniforms() {
    cacheActiveUniforms();
    for (const UniformsByNameHash::ShaderVarMap::value_type& it : _uniformsByNameHash._shaderVars) {
        UploadPushConstant(it.second, true);
    }
}

void glShader::Uniform(const I32 binding, const GFX::PushConstantType type, const Byte* const values, const GLsizei byteCount, const bool flag) const {
    if (binding == -1) {
        return;
    }

    const GLboolean transpose = (flag ? GL_TRUE : GL_FALSE);
    switch (type) {
        case GFX::PushConstantType::BOOL: {
            const auto&[size, data] = convertData<GLint, 1, I32>(byteCount, values);
            glProgramUniform1iv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::INT: {
            const auto&[size, data] = convertData<GLint, 1, I32>(byteCount, values);
            glProgramUniform1iv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::UINT: {
            const auto&[size, data] = convertData<GLuint, 1, U32>(byteCount, values);
            glProgramUniform1uiv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::DOUBLE: {
            const auto&[size, data] = convertData<GLdouble, 1, D64>(byteCount, values);
            glProgramUniform1dv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::FLOAT: {
            const auto&[size, data] = convertData<GLfloat, 1, F32>(byteCount, values);
            glProgramUniform1fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::IVEC2: {
            const auto&[size, data] = convertData<GLint, 2, vec2<I32>>(byteCount, values);
            glProgramUniform2iv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::IVEC3: {
            const auto&[size, data] = convertData<GLint, 3, vec3<I32>>(byteCount, values);
            glProgramUniform3iv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::IVEC4: {
            const auto&[size, data] = convertData<GLint, 4, vec4<I32>>(byteCount, values);
            glProgramUniform4iv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::UVEC2: {
            const auto&[size, data] = convertData<GLuint, 2, vec2<U32>>(byteCount, values);
            glProgramUniform2uiv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::UVEC3: {
            const auto&[size, data] = convertData<GLuint, 3, vec3<U32>>(byteCount, values);
            glProgramUniform3uiv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::UVEC4: {
            const auto&[size, data] = convertData<GLuint, 4, vec4<U32>>(byteCount, values);
            glProgramUniform4uiv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::DVEC2: {
            const auto&[size, data] = convertData<GLdouble, 2, vec2<D64>>(byteCount, values);
            glProgramUniform2dv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::DVEC3: {
            const auto&[size, data] = convertData<GLdouble, 3, vec3<D64>>(byteCount, values);
            glProgramUniform3dv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::DVEC4: {
            const auto&[size, data] = convertData<GLdouble, 4, vec4<D64>>(byteCount, values);
            glProgramUniform4dv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::VEC2: {
            const auto&[size, data] = convertData<GLfloat, 2, vec2<F32>>(byteCount, values);
            glProgramUniform2fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::VEC3: {
            const auto&[size, data] = convertData<GLfloat, 3, vec3<F32>>(byteCount, values);
            glProgramUniform3fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::FCOLOUR3: {
            const auto&[size, data] = convertData<GLfloat, 3, FColour3>(byteCount, values);
            glProgramUniform3fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::VEC4: {
            const auto&[size, data] = convertData<GLfloat, 4, vec4<F32>>(byteCount, values);
            glProgramUniform4fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::FCOLOUR4: {
            const auto&[size, data] = convertData<GLfloat, 4, FColour4>(byteCount, values);
            glProgramUniform3fv(_programHandle, binding, size, data);
        } break;
        case GFX::PushConstantType::IMAT2: {
            const auto&[size, data] = convertData<GLfloat, 4, mat2<I32>>(byteCount, values);
            glProgramUniformMatrix2fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::IMAT3: {
            const auto&[size, data] = convertData<GLfloat, 9, mat3<I32>>(byteCount, values);
            glProgramUniformMatrix3fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::IMAT4: {
            const auto&[size, data] = convertData<GLfloat, 16, mat4<I32>>(byteCount, values);
            glProgramUniformMatrix4fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::UMAT2: {
            const auto&[size, data] = convertData<GLfloat, 4, mat2<U32>>(byteCount, values);
            glProgramUniformMatrix2fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::UMAT3: {
            const auto&[size, data] = convertData<GLfloat, 9, mat3<U32>>(byteCount, values);
            glProgramUniformMatrix3fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::UMAT4: {
            const auto&[size, data] = convertData<GLfloat, 16, mat4<U32>>(byteCount, values);
            glProgramUniformMatrix4fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::MAT2: {
            const auto&[size, data] = convertData<GLfloat, 4, mat2<F32>>(byteCount, values);
            glProgramUniformMatrix2fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::MAT3: {
            const auto&[size, data] = convertData<GLfloat, 9, mat3<F32>>(byteCount, values);
            glProgramUniformMatrix3fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::MAT4: {
            const auto&[size, data] = convertData<GLfloat, 16, mat4<F32>>(byteCount, values);
            glProgramUniformMatrix4fv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::DMAT2: {
            const auto&[size, data] = convertData<GLdouble, 4, mat2<D64>>(byteCount, values);
            glProgramUniformMatrix2dv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::DMAT3: {
            const auto&[size, data] = convertData<GLdouble, 9, mat3<D64>>(byteCount, values);
            glProgramUniformMatrix3dv(_programHandle, binding, size, transpose, data);
        } break;
        case GFX::PushConstantType::DMAT4: {
            const auto&[size, data] = convertData<GLdouble, 16, mat4<D64>>(byteCount, values);
            glProgramUniformMatrix4dv(_programHandle, binding, size, transpose, data);
        } break;
        default:
            DIVIDE_ASSERT(false, "glShaderProgram::Uniform error: Unhandled data type!");
    }
}

/// Add a define to the shader. The defined must not have been added previously
void glShader::addShaderDefine(const stringImpl& define, bool appendPrefix) {
    // Find the string in the list of program defines
    const auto it = std::find(std::begin(_definesList), std::end(_definesList), std::make_pair(define, appendPrefix));
    // If we can't find it, we add it
    if (it == std::end(_definesList)) {
        _definesList.emplace_back(define, appendPrefix);
        shouldRecompile(true);
    } else {
        // If we did find it, we'll show an error message in debug builds about double add
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_ADD")), define.c_str(), _name.c_str());
    }
}

/// Remove a define from the shader. The defined must have been added previously
void glShader::removeShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    auto it = eastl::find(eastl::begin(_definesList), eastl::end(_definesList), std::make_pair(define, true));
        if (it == eastl::end(_definesList)) {
            it = eastl::find(eastl::begin(_definesList), eastl::end(_definesList), std::make_pair(define, false));
        }
    // If we find it, we remove it
    if (it != eastl::end(_definesList)) {
        _definesList.erase(it);
        shouldRecompile(true);
    } else {
        // If we did not find it, we'll show an error message in debug builds
        Console::d_errorfn(Locale::get(_ID("ERROR_INVALID_DEFINE_DELETE")), define.c_str(), _name.c_str());
    }
}

};