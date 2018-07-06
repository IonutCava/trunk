#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace {
    const bool USE_THREADED_SHADER_LOAD = true;
};

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<ShaderProgram>::operator()() {
    const ParamHandler& par = ParamHandler::instance();

    stringImpl resourceLocation;
    if (_descriptor.getResourceLocation().compare("default") == 0) {
        resourceLocation = 
            par.getParam<stringImpl>(_ID("assetsLocation")) + "/" +
            par.getParam<stringImpl>(_ID("shaderLocation")) + "/";
    } else {
        resourceLocation = _descriptor.getResourceLocation();
    }

    ShaderProgram_ptr ptr(GFXDevice::instance().newShaderProgram(_descriptor.getName(),
                                                                 resourceLocation,
                                                                 USE_THREADED_SHADER_LOAD ? _descriptor.getThreaded()
                                                                                          : false),
                          DeleteResource());



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

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
