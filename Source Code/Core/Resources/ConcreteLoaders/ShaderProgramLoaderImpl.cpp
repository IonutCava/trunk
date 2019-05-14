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
    if (_descriptor.assetName().empty()) {
        _descriptor.assetName(_descriptor.resourceName());
    }

    if (_descriptor.assetLocation().empty()) {
        _descriptor.assetLocation(Paths::g_assetsLocation + Paths::g_shadersLocation);
    }

    const std::shared_ptr<ShaderProgramDescriptor>& shaderDescriptor = _descriptor.getPropertyDescriptor<ShaderProgramDescriptor>();
    assert(shaderDescriptor != nullptr);

    ShaderProgram_ptr ptr(_context.gfx().newShaderProgram(_loadingDescriptorHash,
                                                          _descriptor.resourceName(),
                                                          _descriptor.assetName(),
                                                          _descriptor.assetLocation(),
                                                          *shaderDescriptor,
                                                          USE_THREADED_SHADER_LOAD ? _descriptor.getThreaded()
                                                                                   : false),
                          DeleteResource(_cache));


    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
