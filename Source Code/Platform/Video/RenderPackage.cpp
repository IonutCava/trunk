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
      _commands(GFX::allocateCommandBuffer(useSecondaryBuffers)),
      _drawCommandDirty(true),
      _pipelineDirty(true),
      _clipPlanesDirty(true),
      _pushConstantsDirty(true),
      _descriptorSetsDirty(true)
{
}

RenderPackage::~RenderPackage()
{
    GFX::deallocateCommandBuffer(_commands, _secondaryCommandPool);
}

void RenderPackage::clear() {
    _commands->clear();
    _drawCommands.clear();
    _pipelines.clear();
    _clipPlanes.clear();
    _pushConstants.clear();

    _drawCommandDirty = _pipelineDirty = _clipPlanesDirty = _pushConstantsDirty = _descriptorSetsDirty = true;
}

void RenderPackage::set(const RenderPackage& other) {
    *_commands = *other._commands;
}

size_t RenderPackage::getSortKeyHash() const {
    if (!_pipelines.empty()) {
        return _pipelines.front().getHash();
    }

    return 0;
}

size_t RenderPackage::drawCommandCount() const {
    return _drawCommands.size();
}

GenericDrawCommand& RenderPackage::drawCommand(I32 index) {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    return _drawCommands[index];
}

const GenericDrawCommand& RenderPackage::drawCommand(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    return _drawCommands[index];
}

void RenderPackage::drawCommand(I32 index, const GenericDrawCommand cmd) {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    _drawCommands[index].set(cmd);
    _drawCommandDirty = true;
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    GFX::AddDrawCommands(*_commands, cmd);
    _drawCommands.insert(std::cend(_drawCommands),
                         std::cbegin(cmd._drawCommands),
                         std::cend(cmd._drawCommands));
}

const Pipeline& RenderPackage::pipeline(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    return _pipelines[index];
}

void RenderPackage::pipeline(I32 index, const Pipeline& pipeline) {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    _pipelines[index].fromDescriptor(pipeline.toDescriptor());
    _pipelineDirty = true;
}

void RenderPackage::addPipelineCommand(const GFX::BindPipelineCommand& pipeline) {
    GFX::BindPipeline(*_commands, pipeline);
    _pipelines.push_back(pipeline._pipeline);
}

const ClipPlaneList& RenderPackage::clipPlanes(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    return _clipPlanes[index];
}

void RenderPackage::clipPlanes(I32 index, const ClipPlaneList& clipPlanes) {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    _clipPlanes[index] = clipPlanes;
    _clipPlanesDirty = true;
}

void RenderPackage::addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes) {
    GFX::SetClipPlanes(*_commands, clipPlanes);
    _clipPlanes.push_back(clipPlanes._clippingPlanes);
}

const PushConstants& RenderPackage::pushConstants(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_pushConstants.size()), "RenderPackage::pushConstants error: Invalid push constants index!");
    return _pushConstants[index];
}

void RenderPackage::pushConstants(I32 index, const PushConstants& constants) {
    DIVIDE_ASSERT(index < to_I32(_pushConstants.size()), "RenderPackage::pushConstants error: Invalid push constants index!");
    _pushConstants[index] = constants;
    _pushConstantsDirty = true;
}

void RenderPackage::addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants) {
    GFX::SendPushConstants(*_commands, pushConstants);
    _pushConstants.push_back(pushConstants._constants);
}

const DescriptorSet& RenderPackage::descriptorSet(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _descriptorSets[index];
}

void RenderPackage::descriptorSet(I32 index, const DescriptorSet& descriptorSets) {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    _descriptorSets[index] = descriptorSets;
    _descriptorSetsDirty = true;
}

void RenderPackage::addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets) {
    GFX::BindDescriptorSets(*_commands, descriptorSets);
    _descriptorSets.push_back(descriptorSets._set);
}

GFX::CommandBuffer& RenderPackage::commands() {
    return *_commands;
}

const GFX::CommandBuffer& RenderPackage::commands() const {
    return *_commands;
}
}; //namespace Divide