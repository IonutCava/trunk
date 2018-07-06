#include "Headers/ShaderProgram.h"

#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

ShaderProgram::ShaderProgram()
    : Resource("temp_shader_program")
{
    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
    // Start with clean refresh flags
    _refreshStage.fill(false);
}

ShaderProgram::~ShaderProgram()
{
    Console::d_printfn(Locale::get("SHADER_PROGRAM_REMOVE"), getName().c_str());
    // Remove every shader attached to this program
    for (ShaderIDMap::value_type& it : _shaderIDMap) {
        ShaderManager::getInstance().removeShader(it.second);
    }
    // Unregister the program from the manager
    ShaderManager::getInstance().unregisterShaderProgram(getName());
    _shaderIDMap.clear();
}

/// Called once per frame. Update common values used across programs
bool ShaderProgram::update(const U64 deltaTime) {
    return true;
}

/// Add a define to the shader. The defined must not have been added previously
void ShaderProgram::addShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    vectorImpl<stringImpl>::iterator it =
        std::find(std::begin(_definesList), std::end(_definesList), define);
    // If we can't find it, we add it
    if (it == std::end(_definesList)) {
        _definesList.push_back(define);
    } else {
        // If we did find it, we'll show an error message in debug builds about
        // double add
        Console::d_errorfn(Locale::get("ERROR_INVALID_DEFINE_ADD"),
                           define.c_str(), getName().c_str());
    }
}

/// Remove a define from the shader. The defined must have been added previously
void ShaderProgram::removeShaderDefine(const stringImpl& define) {
    // Find the string in the list of program defines
    vectorImpl<stringImpl>::iterator it =
        std::find(std::begin(_definesList), std::end(_definesList), define);
    // If we find it, we remove it
    if (it != std::end(_definesList)) {
        _definesList.erase(it);
    } else {
        // If we did not find it, we'll show an error message in debug builds
        Console::d_errorfn(Locale::get("ERROR_INVALID_DEFINE_DELETE"),
                           define.c_str(), getName().c_str());
    }
}

/// Rebuild the specified shader stages from source code
void ShaderProgram::recompile(const bool vertex, const bool fragment,
                              const bool geometry, const bool tessellation,
                              const bool compute) {
    _linked = false;
    // Remember bind state
    bool wasBound = isBound();
    if (wasBound) {
        ShaderManager::getInstance().unbind();
    }
    // Update refresh flags
    _refreshStage[to_uint(ShaderType::VERTEX)] = vertex;
    _refreshStage[to_uint(ShaderType::FRAGMENT)] = fragment;
    _refreshStage[to_uint(ShaderType::GEOMETRY)] = geometry;
    _refreshStage[to_uint(ShaderType::TESSELATION_CTRL)] =
        tessellation;
    _refreshStage[to_uint(ShaderType::TESSELATION_EVAL)] =
        tessellation;
    _refreshStage[to_uint(ShaderType::COMPUTE)] = compute;
    // Recreate all of the needed shaders
    load();
    // Restore bind state
    if (wasBound) {
        bind();
    }
}
};
