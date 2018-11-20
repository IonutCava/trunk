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


    const std::shared_ptr<ShaderProgramDescriptor>& shaderDescriptor = _descriptor.getPropertyDescriptor<ShaderProgramDescriptor>();
    if (shaderDescriptor != nullptr) {
        // get all of the preprocessor defines
        for (auto it : shaderDescriptor->_defines) {
            ptr->addShaderDefine(it.first, it.second);
        }
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
