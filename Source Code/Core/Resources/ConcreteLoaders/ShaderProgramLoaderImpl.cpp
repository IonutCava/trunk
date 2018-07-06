#include "stdafx.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace {
    const bool USE_THREADED_SHADER_LOAD = true;
};

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<ShaderProgram>::operator()() {
    if (_descriptor.getResourceName().empty()) {
        _descriptor.setResourceName(_descriptor.name());
    }

    if (_descriptor.getResourceLocation().empty()) {
        _descriptor.setResourceLocation(Paths::g_assetsLocation + Paths::g_shadersLocation);
    }

    ShaderProgram_ptr ptr(_context.gfx().newShaderProgram(_loadingDescriptorHash,
                                                          _descriptor.name(),
                                                          _descriptor.getResourceName(),
                                                          _descriptor.getResourceLocation(),
                                                          USE_THREADED_SHADER_LOAD ? _descriptor.getThreaded()
                                                                                   : false),
                          DeleteResource(_cache));



    // get all of the preprocessor defines
    if (!_descriptor.getPropertyListString().empty()) {
        vector<stringImpl> defines = Util::Split(_descriptor.getPropertyListString(), ',');
        for (U8 i = 0; i < defines.size(); i++) {
            if (!defines[i].empty()) {
                ptr->addShaderDefine(Util::Trim(defines[i]));
            }
        }
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
