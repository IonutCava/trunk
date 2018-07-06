#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace {
    const bool USE_THREADED_SHADER_LOAD = false;
};

namespace Divide {

template<>
ShaderProgram* ImplResourceLoader<ShaderProgram>::operator()() {

    ParamHandler& par = ParamHandler::getInstance();
    ShaderProgram* ptr = GFX_DEVICE.newShaderProgram(USE_THREADED_SHADER_LOAD ? _descriptor.getThreaded() : false);

    if (_descriptor.getResourceLocation().compare("default") == 0) {
        ptr->setResourceLocation(
            par.getParam<stringImpl>(_ID("assetsLocation")) + "/" +
            par.getParam<stringImpl>(_ID("shaderLocation")) + "/");
    } else {
        ptr->setResourceLocation(_descriptor.getResourceLocation());
    }

    // get all of the preprocessor defines
    if (!_descriptor.getPropertyListString().empty()) {
        vectorImpl<stringImpl> defines =
            Util::Split(_descriptor.getPropertyListString(), ',');
        for (U8 i = 0; i < defines.size(); i++) {
            if (!defines[i].empty()) {
                ptr->addShaderDefine(Util::Trim(defines[i]));
            }
        }
    }

    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        ShaderManager::getInstance().registerShaderProgram(ptr->getName(), ptr);
    }

    return ptr;
}

};
