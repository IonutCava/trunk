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

SharedMutex glShader::_shaderNameLock;
glShader::ShaderMap glShader::_shaderNameMap;


IMPLEMENT_CUSTOM_ALLOCATOR(glShader, 0, 0);

void glShader::initStaticData() {

}

void glShader::destroyStaticData() {

}

glShader::glShader(GFXDevice& context,
                   const stringImpl& name,
                   const ShaderType& type,
                   const bool optimise)
    : TrackedObject(),
      GraphicsResource(context, GraphicsResource::Type::SHADER, getGUID()),
      glObject(glObjectType::TYPE_SHADER, context),
     _skipIncludes(false),
     _shader(std::numeric_limits<U32>::max()),
     _name(name),
     _type(type)
{
    _compiled.clear();

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
}

glShader::~glShader() {
    Console::d_printfn(Locale::get(_ID("SHADER_DELETE")), name().c_str());
    glDeleteShader(_shader);
}

bool glShader::load(const stringImpl& source, U32 lineOffset) {
    if (source.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_GLSL_NOT_FOUND")), name().c_str());
        return false;
    }

    _usedAtoms.clear();
    stringImpl parsedSource = _skipIncludes ? source
                                            : glShaderProgram::preprocessIncludes(name(), source, 0, _usedAtoms, true);

    Util::ReplaceStringInPlace(parsedSource, "//__LINE_OFFSET_",
                               Util::StringFormat("#line %d", lineOffset));

    const char* src = parsedSource.c_str();

    GLsizei sourceLength = (GLsizei)parsedSource.length();
    glShaderSource(_shader, 1, &src, &sourceLength);

    if (!_skipIncludes) {
       glShaderProgram::shaderFileWrite(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText, name(), src);
    }

    _compiled.clear();
    return true;
}

bool glShader::compile() {
    if (!_compiled.test_and_set()) {
        glCompileShader(_shader);
        if (Config::ENABLE_GPU_VALIDATION) {
            glObjectLabel(GL_SHADER, _shader, -1, name().c_str());
        }
        return validate();
    }

    return true;
}

bool glShader::validate() {
    if (Config::ENABLE_GPU_VALIDATION) {
        GLint length = 0, status = 0;

        glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
        glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1) {
            return true;
        }
        vector<char> shaderLog(length);
        glGetShaderInfoLog(_shader, length, NULL, &shaderLog[0]);
        shaderLog.push_back('\n');
        if (status == 0) {
            Console::errorfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _shader, _name.c_str(), &shaderLog[0], getGUID());
            return false;
        } else {
            Console::d_printfn(Locale::get(_ID("GLSL_VALIDATING_SHADER")), _shader, _name.c_str(), &shaderLog[0], getGUID());
            return true;
        }
    }

    return true;
}

// ============================ static data =========================== //
/// Remove a shader entity. The shader is deleted only if it isn't referenced by a program
void glShader::removeShader(glShader* s) {
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
                               const stringImpl& source,
                               const ShaderType& type,
                               const bool parseCode,
                               U32 lineOffset) {
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
    if (!shader->load(source, lineOffset)) {
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

    return shader;
}
};