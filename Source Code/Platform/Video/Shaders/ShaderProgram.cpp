#include "Headers/ShaderProgram.h"

#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

ShaderProgram::ShaderProgram(const bool optimise)
    : HardwareResource("temp_shader_program"),
      _optimise(optimise),
      _dirty(true),
      _outputCount(0),
      _elapsedTime(0ULL),
      _elapsedTimeMS(0.0f) {
    _bound = false;
    _linked = false;
    // Override in concrete implementations with appropriate invalid values
    _shaderProgramID = 0;
    // Start with clean refresh flags
    _refreshStage.fill(false);
    // Cache some frequently updated uniform locations
    _sceneDataDirty = true;
}

ShaderProgram::~ShaderProgram() {
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
    ParamHandler& par = ParamHandler::getInstance();
    LightManager& lightMgr = LightManager::getInstance();

    // Update internal timers
    _elapsedTime += deltaTime;
    _elapsedTimeMS =
        static_cast<F32>(Time::MicrosecondsToMilliseconds(_elapsedTime));
    // Skip programs that aren't fully loaded
    if (!isHWInitComplete()) {
        return false;
    }
    // Toggle fog on or off
    bool enableFog = par.getParam<bool>("rendering.enableFog");
#ifdef _DEBUG
    // Shadow splits are only visible in debug builds
    this->Uniform("dvd_showShadowSplits",
                  par.getParam<bool>("rendering.debug.showSplits"));
#endif

    // Time, fog, ambient light
    this->Uniform("dvd_time", _elapsedTimeMS);
    this->Uniform("dvd_enableFog", enableFog);
    this->Uniform("dvd_lightAmbient", lightMgr.getAmbientLight());
    // Scene specific data is updated only if it changed
    if (_sceneDataDirty) {
        // Check and update fog properties
        if (enableFog) {
            this->Uniform(
                "fogColor",
                vec3<F32>(
                    par.getParam<F32>("rendering.sceneState.fogColor.r"),
                    par.getParam<F32>("rendering.sceneState.fogColor.g"),
                    par.getParam<F32>("rendering.sceneState.fogColor.b")));
            this->Uniform("fogDensity",
                          par.getParam<F32>("rendering.sceneState.fogDensity"));
        }
        // Upload wind data
        const SceneState& activeSceneState = GET_ACTIVE_SCENE()->state();
        this->Uniform("windDirection",
                      vec2<F32>(activeSceneState.getWindDirX(),
                                activeSceneState.getWindDirZ()));
        this->Uniform("windSpeed", activeSceneState.getWindSpeed());
        _sceneDataDirty = false;
    }
    // The following values are updated only if a call to the ShaderManager's
    // refresh() function is made
    if (_dirty) {
        // Inverse screen resolution
        const vec2<U16>& screenRes =
            GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)
                ->getResolution();
        this->Uniform("dvd_invScreenDimension", vec2<F32>(1.0f / screenRes.width,
                                                          1.0f / screenRes.height));
        // Shadow mapping specific values
        this->Uniform("dvd_lightBleedBias", 0.0000002f);
        this->Uniform("dvd_minShadowVariance", 0.0002f);
        this->Uniform("dvd_shadowMaxDist", 250.0f);
        this->Uniform("dvd_shadowFadeDist", 150.0f);
        _dirty = false;
    }

    return true;
}

/// Rendering API specific loading (called after the API specific derived calls
/// processed the request)
bool ShaderProgram::generateHWResource(const stringImpl& name) {
    _name = name;

    if (!HardwareResource::generateHWResource(name)) {
        return false;
    }
    // Finish threaded loading
    HardwareResource::threadedLoad(name);
    // Validate loading state
    DIVIDE_ASSERT(isHWInitComplete(),
                  "ShaderProgram error: hardware initialization failed!");

    // Make sure the first call to update processes all of the needed data
    _dirty = _sceneDataDirty = true;

    return true;
}

/// Mark the shader as bound
bool ShaderProgram::bind() {
    _bound = true;
    return _shaderProgramID != 0;
}

/// Mark the shader as unbound
void ShaderProgram::unbind(bool resetActiveProgram) { _bound = false; }

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
    bool wasBound = _bound;
    if (wasBound) {
        unbind();
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
    generateHWResource(getName());
    // Restore bind state
    if (wasBound) {
        bind();
    }
}
};