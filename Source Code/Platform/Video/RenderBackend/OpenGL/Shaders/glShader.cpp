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
};

SharedMutex glShader::_shaderNameLock;
glShader::ShaderMap glShader::_shaderNameMap;

glShader::glShader(GFXDevice& context,
                   const stringImpl& name,
                   const bool deferredUpload)
    : TrackedObject(),
      GraphicsResource(context, GraphicsResource::Type::SHADER, getGUID()),
     glObject(glObjectType::TYPE_SHADER, context),
     _stageMask(UseProgramStageMask::GL_NONE_BIT),
     _valid(false),
     _shouldRecompile(false),
     _loadedFromBinary(false),
     _deferredUpload(deferredUpload),
     _binaryFormat(GL_NONE),
     _shader(std::numeric_limits<U32>::max()),
     _name(name)
{
}

glShader::~glShader() {
    Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), name().c_str());
    GL_API::deleteShaderPrograms(1, &_shader);
}

bool glShader::uploadToGPU() {
    // prevent double load
    if (_sourceCode.empty()) {
        return true;
    }

    Console::d_printfn(Locale::get(_ID("GLSL_LOAD_PROGRAM")), _name.c_str(), getGUID());
    if (!loadFromBinary()) {
        const char* src[] = { _sourceCode.front().c_str() };

        if (true) {
            //UniqueLock lock(GLUtil::_driverLock);
            _shader = glCreateShaderProgramv(GLUtil::glShaderStageTable[to_base(_type)], 1, src);
        } else {
            const GLuint shader = glCreateShader(GLUtil::glShaderStageTable[to_base(_type)]);
            if (shader) {
                glShaderSource(shader, 1, src, NULL);
                glCompileShader(shader);
                _shader = glCreateProgram();
                if (_shader) {
                    GLint compiled = 0;
                    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                    glProgramParameteri(_shader, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
                    glProgramParameteri(_shader, GL_PROGRAM_SEPARABLE, GL_TRUE);
                    if (compiled) {
                        glAttachShader(_shader, shader);
                        glLinkProgram(_shader);
                        glDetachShader(_shader, shader);
                    }
                    /* append-shader-info-log-to-program-info-log */
                }
                glDeleteShader(shader);
            } else {
                _shader = 0;
            }
        }

        if (_shader != 0) {
            if (Config::ENABLE_GPU_VALIDATION) {
                // And check the result
                GLboolean linkStatus;
                glGetProgramiv(_shader, GL_LINK_STATUS, &linkStatus);

                GLint length = 0;
                glGetProgramiv(_shader, GL_INFO_LOG_LENGTH, &length);

                stringImpl validationBuffer = "[OK]";

                if (length > 1) {
                    validationBuffer.resize(length);
                    glGetProgramInfoLog(_shader, length, NULL, &validationBuffer[0]);
                    if (validationBuffer.size() > g_validationBufferMaxSize) {
                        // On some systems, the program's disassembly is printed, and that can get quite large
                        validationBuffer.resize(std::strlen(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG"))) + g_validationBufferMaxSize);
                        // Use the simple "truncate and inform user" system (a.k.a. add dots and delete the rest)
                        validationBuffer.append(" ... ");
                    }
                }
                // If linking failed, show an error, else print the result in debug builds.
                if (linkStatus == GL_FALSE) {
                    Console::errorfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG")), _name.c_str(), validationBuffer.c_str(), getGUID());
                } else {
                    Console::printfn(Locale::get(_ID("GLSL_LINK_PROGRAM_LOG_OK")), _name.c_str(), validationBuffer.c_str(), getGUID(), _shader);
                    if (Config::ENABLE_GPU_VALIDATION) {
                        glObjectLabel(GL_PROGRAM, _shader, -1, _name.c_str());
                    }
                    _valid = true;
                }
            } else {
                _valid = true;
            }
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_GLSL_CREATE_PROGRAM")), _name.c_str());
        }
    }

    _sourceCode.clear();

    return _valid;
}

bool glShader::load(const ShaderLoadData& data) {
    if (data.empty() || data.front().sourceCode.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")), name().c_str());
        return false;
    }

    _shaderVarLocation.clear();

    for (auto it : data) {
        for (const stringImpl& source : it.sourceCode) {
            _sourceCode[to_base(it._type)].push_back(source);
        }
        _usedAtoms.insert(std::cend(_usedAtoms), std::cbegin(it.atoms), std::cend(it.atoms));
    }

    if (!_deferredUpload) {
        return uploadToGPU();
    }

    return true;
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void glShader::removeShader(glShader* s) {
    assert(s != nullptr);

    // Try to find it
    U64 nameHash = _ID(s->name().c_str());
    UniqueLockShared w_lock(_shaderNameLock);
    ShaderMap::iterator it = _shaderNameMap.find(nameHash);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it.
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader
            MemoryManager::DELETE(s);
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
                               const ShaderLoadData& data,
                               bool deferredUpload) {
    // See if we have the shader already loaded
    glShader* shader = getShader(name);
    
    bool newShader = false;
    // If we do, and don't need a recompile, just return it
    if (shader == nullptr) {
        // If we can't find it, we create a new one
        shader = MemoryManager_NEW glShader(context, name, deferredUpload);
        newShader = true;
    }

    // At this stage, we have a valid Shader object, so load the source code
    if (!shader->load(data)) {
        // If loading the source code failed, delete it
        if (newShader) {
            MemoryManager::DELETE(shader);
        }
    } else {
        if (newShader) {
            U64 nameHash = _ID(name.c_str());
            // If we loaded the source code successfully,  register it
            UniqueLockShared w_lock(_shaderNameLock);
            hashAlg::insert(_shaderNameMap, nameHash, shader);
        }
    }

    shader->reuploadUniforms();

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
                    _shader = glCreateProgram();
                    glProgramBinary(_shader, _binaryFormat, (bufferPtr)data.data(), (GLint)data.size());
                    // Check if the program linked successfully on load
                    GLboolean success = 0;
                    glGetProgramiv(_shader, GL_LINK_STATUS, &success);
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
    if (!isValid()) {
        return;
    }

    // Get the size of the binary code
    GLint binaryLength = 0;
    glGetProgramiv(_shader, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    // allocate a big enough buffer to hold it
    char* binary = MemoryManager_NEW char[binaryLength];
    DIVIDE_ASSERT(binary != NULL, "glShader error: could not allocate memory for the program binary!");

    // and fill the buffer with the binary code
    glGetProgramBinary(_shader, binaryLength, NULL, &_binaryFormat, binary);
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
I32 glShader::binding(const char* name) {
    // If the shader can't be used for rendering, just return an invalid address
    if (_shader == 0 || _shader == GLUtil::_invalidObjectID || !isValid()) {
        return -1;
    }

    // Check the cache for the location
    U64 nameHash = _ID(name);
    ShaderVarMap::const_iterator it = _shaderVarLocation.find(nameHash);
    if (it != std::end(_shaderVarLocation)) {
        return it->second;
    }

    // Cache miss. Query OpenGL for the location
    GLint location = glGetUniformLocation(_shader, name);

    // Save it for later reference
    hashAlg::insert(_shaderVarLocation, nameHash, location);

    // Return the location
    return location;
}

I32 glShader::cachedValueUpdate(const GFX::PushConstant& constant) {
    if (constant._binding.empty() || constant._type == GFX::PushConstantType::COUNT) {
        return -1;
    }

    U64 locationHash = constant._bindingHash;

    I32 bindingLoc = binding(constant._binding.c_str());

    if (bindingLoc == -1 || _shader == 0 || _shader == GLUtil::_invalidObjectID) {
        return -1;
    }

    UniformsByNameHash::ShaderVarMap::iterator it = _uniformsByNameHash._shaderVars.find(locationHash);
    if (it != std::end(_uniformsByNameHash._shaderVars)) {
        if (it->second == constant) {
            return -1;
        }
        else {
            it->second = constant;
        }
    } else {
        hashAlg::emplace(_uniformsByNameHash._shaderVars, locationHash, constant);
    }

    return bindingLoc;
}

void glShader::UploadPushConstant(const GFX::PushConstant& constant) {
    I32 binding = cachedValueUpdate(constant);

    if (binding != -1) {
        Uniform(binding, constant._type, constant._buffer, constant._flag);
    }
}

void glShader::reuploadUniforms() {
    for (UniformsByNameHash::ShaderVarMap::value_type it : _uniformsByNameHash._shaderVars) {
        UploadPushConstant(it.second);
    }
}

void glShader::Uniform(I32 binding, GFX::PushConstantType type, const vectorEASTL<char>& values, bool flag) const {
    GLsizei byteCount = (GLsizei)values.size();
    switch (type) {
        case GFX::PushConstantType::BOOL:
            glProgramUniform1iv(_shader, binding, byteCount / (1 * sizeof(GLbyte)), (GLint*)castData<GLbyte, 1, char>(values));
            break;
        case GFX::PushConstantType::INT:
            glProgramUniform1iv(_shader, binding, byteCount / (1 * sizeof(GLint)), castData<GLint, 1, I32>(values));
            break;
        case GFX::PushConstantType::UINT:
            glProgramUniform1uiv(_shader, binding, byteCount / (1 * sizeof(GLuint)), castData<GLuint, 1, U32>(values));
            break;
        case GFX::PushConstantType::DOUBLE:
            glProgramUniform1dv(_shader, binding, byteCount / (1 * sizeof(GLdouble)), castData<GLdouble, 1, D64>(values));
            break;
        case GFX::PushConstantType::FLOAT:
            glProgramUniform1fv(_shader, binding, byteCount / (1 * sizeof(GLfloat)), castData<GLfloat, 1, F32>(values));
            break;
        case GFX::PushConstantType::IVEC2:
            glProgramUniform2iv(_shader, binding, byteCount / (2 * sizeof(GLint)), castData<GLint, 2, vec2<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC3:
            glProgramUniform3iv(_shader, binding, byteCount / (3 * sizeof(GLint)), castData<GLint, 3, vec3<I32>>(values));
            break;
        case GFX::PushConstantType::IVEC4:
            glProgramUniform4iv(_shader, binding, byteCount / (4 * sizeof(GLint)), castData<GLint, 4, vec4<I32>>(values));
            break;
        case GFX::PushConstantType::UVEC2:
            glProgramUniform2uiv(_shader, binding, byteCount / (2 * sizeof(GLuint)), castData<GLuint, 2, vec2<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC3:
            glProgramUniform3uiv(_shader, binding, byteCount / (3 * sizeof(GLuint)), castData<GLuint, 3, vec3<U32>>(values));
            break;
        case GFX::PushConstantType::UVEC4:
            glProgramUniform4uiv(_shader, binding, byteCount / (4 * sizeof(GLuint)), castData<GLuint, 4, vec4<U32>>(values));
            break;
        case GFX::PushConstantType::DVEC2:
            glProgramUniform2dv(_shader, binding, byteCount / (2 * sizeof(GLdouble)), castData<GLdouble, 2, vec2<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC3:
            glProgramUniform3dv(_shader, binding, byteCount / (3 * sizeof(GLdouble)), castData<GLdouble, 3, vec3<D64>>(values));
            break;
        case GFX::PushConstantType::DVEC4:
            glProgramUniform4dv(_shader, binding, byteCount / (4 * sizeof(GLdouble)), castData<GLdouble, 4, vec4<D64>>(values));
            break;
        case GFX::PushConstantType::VEC2:
            glProgramUniform2fv(_shader, binding, byteCount / (2 * 4), castData<GLfloat, 2, vec2<F32>>(values));
            break;
        case GFX::PushConstantType::VEC3:
            glProgramUniform3fv(_shader, binding, byteCount / (3 * 4), castData<GLfloat, 3, vec3<F32>>(values));
            break;
        case GFX::PushConstantType::VEC4:
            glProgramUniform4fv(_shader, binding, byteCount / (4 * 4), castData<GLfloat, 4, vec4<F32>>(values));
            break;
        case GFX::PushConstantType::IMAT2:
            glProgramUniformMatrix2fv(_shader, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT3:
            glProgramUniformMatrix3fv(_shader, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<I32>>(values));
            break;
        case GFX::PushConstantType::IMAT4:
            glProgramUniformMatrix4fv(_shader, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<I32>>(values));
            break;
        case GFX::PushConstantType::UMAT2:
            glProgramUniformMatrix2fv(_shader, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT3:
            glProgramUniformMatrix3fv(_shader, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<U32>>(values));
            break;
        case GFX::PushConstantType::UMAT4:
            glProgramUniformMatrix4fv(_shader, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<U32>>(values));
            break;
        case GFX::PushConstantType::MAT2:
            glProgramUniformMatrix2fv(_shader, binding, byteCount / (2 * 2 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<F32>>(values));
            break;
        case GFX::PushConstantType::MAT3:
            glProgramUniformMatrix3fv(_shader, binding, byteCount / (3 * 3 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<F32>>(values));
            break;
        case GFX::PushConstantType::MAT4:
            glProgramUniformMatrix4fv(_shader, binding, byteCount / (4 * 4 * sizeof(GLfloat)), flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<F32>>(values));
            break;
        case GFX::PushConstantType::DMAT2:
            glProgramUniformMatrix2dv(_shader, binding, byteCount / (2 * 2 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 4, mat2<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT3:
            glProgramUniformMatrix3dv(_shader, binding, byteCount / (3 * 3 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 9, mat3<D64>>(values));
            break;
        case GFX::PushConstantType::DMAT4:
            glProgramUniformMatrix4dv(_shader, binding, byteCount / (4 * 4 * sizeof(GLdouble)), flag ? GL_TRUE : GL_FALSE, castData<GLdouble, 16, mat4<D64>>(values));
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