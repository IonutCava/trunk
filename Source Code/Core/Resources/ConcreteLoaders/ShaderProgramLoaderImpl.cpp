#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace {
    const bool USE_THREADED_SHADER_LOAD = true;
    stringImpl s_defaultShaderProgramPath;
};

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<ShaderProgram>::operator()() {
    if (_descriptor.getResourceName().empty()) {
        _descriptor.setResourceName(_descriptor.getName());
    }

    if (s_defaultShaderProgramPath.empty()) {
        s_defaultShaderProgramPath = Util::StringFormat("%s/%s", Paths::g_assetsLocation, Paths::g_shadersLocation);
    }

    if (_descriptor.getResourceLocation().empty()) {
        _descriptor.setResourceLocation(s_defaultShaderProgramPath);
    }

    ShaderProgram_ptr ptr(_context.gfx().newShaderProgram(_descriptor.getName(),
                                                          _descriptor.getResourceName(),
                                                          _descriptor.getResourceLocation(),
                                                          USE_THREADED_SHADER_LOAD ? _descriptor.getThreaded()
                                                                                   : false),
                          DeleteResource(_cache));



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
