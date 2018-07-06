#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

RenderPackage::RenderPackage(GFXDevice& context, bool useSecondaryBuffers)
    : _context(context),
      _isRenderable(false),
      _isOcclusionCullable(true),
      _secondaryCommandPool(useSecondaryBuffers),
      _commands(_context.allocateCommandBuffer(useSecondaryBuffers))
{
}

RenderPackage::~RenderPackage()
{
    _context.deallocateCommandBuffer(_commands, _secondaryCommandPool);
}

void RenderPackage::clear() {
    _commands.clear();
}

void RenderPackage::set(const RenderPackage& other) {
    _commands = other._commands;
}

size_t RenderPackage::getSortKeyHash() const {
    const vectorImpl<Pipeline*>& pipelines = _commands.getPipelines();
    if (!pipelines.empty()) {
        return pipelines.front()->getHash();
    }

    return 0;
}


}; //namespace Divide