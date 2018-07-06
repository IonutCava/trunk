#include "Headers/glShader.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/File/Headers/FileManagement.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

SharedLock glShader::_shaderNameLock;
glShader::ShaderMap glShader::_shaderNameMap;
stringImpl glShader::shaderAtomLocationPrefix[to_const_uint(ShaderType::COUNT) + 1];

IMPLEMENT_CUSTOM_ALLOCATOR(glShader, 0, 0);
glShader::glShader(GFXDevice& context,
                   const stringImpl& name,
                   const ShaderType& type,
                   const bool optimise)
    : TrackedObject(),
      GraphicsResource(context, getGUID()),
     _skipIncludes(false),
     _shader(std::numeric_limits<U32>::max()),
     _name(name),
     _type(type)
{
    _compiled = false;

    switch (type) {
        default:
            Console::errorfn(Locale::get(_ID("ERROR_GLSL_UNKNOWN_ShaderType")), type);
            break;
        case ShaderType::VERTEX:
            _shader = glCreateShader(GL_VERTEX_SHADER);
            break;
        case ShaderType::FRAGMENT:
            _shader = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        case ShaderType::GEOMETRY:
            _shader = glCreateShader(GL_GEOMETRY_SHADER);
            break;
        case ShaderType::TESSELATION_CTRL:
            _shader = glCreateShader(GL_TESS_CONTROL_SHADER);
            break;
        case ShaderType::TESSELATION_EVAL:
            _shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            break;
        case ShaderType::COMPUTE:
            _shader = glCreateShader(GL_COMPUTE_SHADER);
            break;
    };

    if (shaderAtomLocationPrefix[to_const_uint(ShaderType::VERTEX)].empty()) {
        stringImpl locPrefix(Paths::g_assetsLocation + Paths::g_shadersLocation + Paths::Shaders::GLSL::g_parentShaderLoc);

        shaderAtomLocationPrefix[to_const_uint(ShaderType::FRAGMENT)] = locPrefix + Paths::Shaders::GLSL::g_fragAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::VERTEX)] = locPrefix + Paths::Shaders::GLSL::g_vertAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::GEOMETRY)] = locPrefix + Paths::Shaders::GLSL::g_geomAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::TESSELATION_CTRL)] = locPrefix + Paths::Shaders::GLSL::g_tescAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::TESSELATION_EVAL)] = locPrefix + Paths::Shaders::GLSL::g_teseAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::COMPUTE)] = locPrefix + Paths::Shaders::GLSL::g_compAtomLoc;
        shaderAtomLocationPrefix[to_const_uint(ShaderType::COUNT)] = locPrefix + Paths::Shaders::GLSL::g_comnAtomLoc;
    }
}

glShader::~glShader() {
    Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), getName().c_str());
    glDeleteShader(_shader);
}

bool glShader::load(const stringImpl& source) {
    if (source.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")), getName().c_str());
        return false;
    }

    _usedAtoms.clear();
    stringImpl parsedSource = _skipIncludes ? source
                                            : preprocessIncludes(source, getName(), 0);

    const char* src = parsedSource.c_str();

    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);

    if (!_skipIncludes) {
        ShaderProgram::shaderFileWrite(Paths::Shaders::g_CacheLocationText + getName(), src);
    }

    _compiled = false;
    return true;
}

bool glShader::compile() {
    if (_compiled) {
        return true;
    }

    glCompileShader(_shader);
    _compiled = true;
    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_SHADER, _shader, -1, getName().c_str());
    }
    return validate();
}

bool glShader::validate() {
    if (Config::ENABLE_GPU_VALIDATION) {
        GLint length = 0, status = 0;

        glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
        glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1) {
            return true;
        }
        vectorImpl<char> shaderLog(length);
        glGetShaderInfoLog(_shader, length, NULL, &shaderLog[0]);
        shaderLog.push_back('\n');
        if (status == 0) {
            Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _shader, _name.c_str(), &shaderLog[0]);
            return false;
        } else {
            Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _shader, _name.c_str(), &shaderLog[0]);
            return true;
        }
    }

    return true;
}

stringImpl glShader::preprocessIncludes(const stringImpl& source,
                                        const stringImpl& filename,
                                        I32 level /*= 0 */) {
    if (level > 32) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_INCLUD_LIMIT")));
    }

    size_t line_number = 1;
    std::smatch matches;

    stringImpl output, line;
    stringImpl include_file, include_string;

    istringstreamImpl input(source);

    while (std::getline(input, line)) {
        if (std::regex_search(line, matches, Paths::g_includePattern)) {
            include_file = matches[1].str().c_str();
            _usedAtoms.push_back(include_file);

            ShaderType typeIndex = ShaderType::COUNT;
            // switch will throw warnings due to promotion to int
            U64 extHash = _ID_RT(Util::GetTrailingCharacters(include_file, 4));
            if (extHash == _ID_RT(Paths::Shaders::GLSL::g_fragAtomExt)) {
                typeIndex = ShaderType::FRAGMENT;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_vertAtomExt)){
                typeIndex = ShaderType::VERTEX;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_geomAtomExt)) {
                typeIndex = ShaderType::GEOMETRY;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_tescAtomExt)) {
                typeIndex = ShaderType::TESSELATION_CTRL;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_teseAtomExt)) {
                typeIndex = ShaderType::TESSELATION_EVAL;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_compAtomExt)) {
                typeIndex = ShaderType::COMPUTE;
            } else if (extHash == _ID_RT(Paths::Shaders::GLSL::g_comnAtomExt)) {
                typeIndex = ShaderType::COUNT;
            } else {
                DIVIDE_UNEXPECTED_CALL("Invalid shader include type");
            }

            include_string = ShaderProgram::shaderFileRead(include_file, shaderAtomLocationPrefix[to_uint(typeIndex)]);
            
            if (include_string.empty()) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_NO_INCLUDE_FILE")),
                                 getName().c_str(),
                                 line_number,
                                 include_file.c_str());
            }
            output.append(preprocessIncludes(include_string, include_file, level + 1));
        } else {
            output.append(line);
        }
        output.append("\n");
        ++line_number;
    }

    return output;
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void glShader::removeShader(glShader* s) {
    // Keep a copy of it's name
    stringImpl name(s->getName());
    // Try to find it
    U64 nameHash = _ID_RT(name);
    UpgradableReadLock ur_lock(_shaderNameLock);
    ShaderMap::iterator it = _shaderNameMap.find(nameHash);
    if (it != std::end(_shaderNameMap)) {
        // Subtract one reference from it.
        if (s->SubRef()) {
            // If the new reference count is 0, delete the shader
            MemoryManager::DELETE(it->second);
            UpgradeToWriteLock w_lock(ur_lock);
            _shaderNameMap.erase(nameHash);
        }
    }
}

/// Return a new shader reference
glShader* glShader::getShader(const stringImpl& name) {
    // Try to find the shader
    ReadLock r_lock(_shaderNameLock);
    ShaderMap::iterator it = _shaderNameMap.find(_ID_RT(name));
    if (it != std::end(_shaderNameMap)) {
        return it->second;
    }

    return nullptr;
}

/// Load a shader by name, source code and stage
glShader* glShader::loadShader(GFXDevice& context,
                               const stringImpl& name,
                               const stringImpl& source,
                               const ShaderType& type,
                               const bool parseCode) {
    // See if we have the shader already loaded
    glShader* shader = getShader(name);
    
    bool newShader = false;
    // If we do, and don't need a recompile, just return it
    if (shader == nullptr) {
        // If we can't find it, we create a new one
        shader = MemoryManager_NEW glShader(context, name, type, false);
        newShader = true;
    }

    shader->skipIncludes(!parseCode);
    // At this stage, we have a valid Shader object, so load the source code
    if (!shader->load(source)) {
        // If loading the source code failed, delete it
        if (newShader) {
            MemoryManager::DELETE(shader);
        }
    } else {
        if (newShader) {
            U64 nameHash = _ID_RT(name);
            // If we loaded the source code successfully,  register it
            WriteLock w_lock(_shaderNameLock);
            hashAlg::emplace(_shaderNameMap, nameHash, shader);
        }
    }

    return shader;
}
};