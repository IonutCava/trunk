#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

RenderPackage::RenderPackage(bool useSecondaryBuffers)
    : _isRenderable(false),
      _qualityRequirement(MinQuality::FULL),
      _isOcclusionCullable(true),
      _secondaryCommandPool(useSecondaryBuffers),
      _commands(nullptr),
      _dirtyFlags(to_base(CommandType::ALL))
{
}

RenderPackage::~RenderPackage()
{
    if (_commands != nullptr) {
        GFX::deallocateCommandBuffer(_commands, _secondaryCommandPool);
    }
}

void RenderPackage::clear() {
    if (_commands != nullptr) {
        _commands->clear();
    }
    _commandOrdering.clear();
    _drawCommands.clear();
    _pipelines.clear();
    _clipPlanes.clear();
    _pushConstants.clear();
    _descriptorSets.clear();

    _dirtyFlags = to_base(CommandType::ALL);
}

void RenderPackage::set(const RenderPackage& other) {
    _commandOrdering = other._commandOrdering;

    _drawCommands = other._drawCommands;
    _pipelines = other._pipelines;
    _clipPlanes = other._clipPlanes;
    _pushConstants = other._pushConstants;
    _descriptorSets = other._descriptorSets;

    _dirtyFlags = to_base(CommandType::ALL);
}

size_t RenderPackage::getSortKeyHash() const {
    if (!_pipelines.empty()) {
        return _pipelines.front()._pipeline->getHash();
    }

    return 0;
}

I32 RenderPackage::drawCommandCount() const {
    return to_I32(_drawCommands.size());
}

const GFX::DrawCommand& RenderPackage::drawCommand(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    return _drawCommands[index];
}

GFX::DrawCommand& RenderPackage::drawCommand(I32 cmdIdx) {
    DIVIDE_ASSERT(cmdIdx < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    return _drawCommands[cmdIdx];
}

const GenericDrawCommand& RenderPackage::drawCommand(I32 index, I32 cmdIndex) const {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    DIVIDE_ASSERT(cmdIndex < to_I32(_drawCommands[index]._drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    return _drawCommands[index]._drawCommands[cmdIndex];
}

void RenderPackage::drawCommand(I32 index, I32 cmdIndex, const GenericDrawCommand& cmd) {
    DIVIDE_ASSERT(index < to_I32(_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command index!");
    DIVIDE_ASSERT(cmdIndex < to_I32(_drawCommands[index]._drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    _drawCommands[index]._drawCommands[cmdIndex] = cmd;
    SetBit(_dirtyFlags, CommandType::DRAW);
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    CommandEntry entry;
    entry._type = GFX::CommandType::DRAW_COMMANDS;
    entry._index = _drawCommands.size();
    _commandOrdering.push_back(entry);

    _drawCommands.push_back(cmd);
}

const Pipeline* RenderPackage::pipeline(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    return _pipelines[index]._pipeline;
}

void RenderPackage::pipeline(I32 index, const Pipeline& pipeline) {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    _pipelines[index]._pipeline = &pipeline;
    SetBit(_dirtyFlags, CommandType::PIPELINE);
}

void RenderPackage::addPipelineCommand(const GFX::BindPipelineCommand& pipeline) {
    CommandEntry entry;
    entry._type = GFX::CommandType::BIND_PIPELINE;
    entry._index = _pipelines.size();
    _commandOrdering.push_back(entry);

    _pipelines.push_back(pipeline);
}

const FrustumClipPlanes& RenderPackage::clipPlanes(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    return _clipPlanes[index]._clippingPlanes;
}

void RenderPackage::clipPlanes(I32 index, const FrustumClipPlanes& clipPlanes) {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    _clipPlanes[index]._clippingPlanes = clipPlanes;
    SetBit(_dirtyFlags, CommandType::CLIP_PLANES);
}

void RenderPackage::addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes) {
    CommandEntry entry;
    entry._type = GFX::CommandType::SET_CLIP_PLANES;
    entry._index = _clipPlanes.size();
    _commandOrdering.push_back(entry);

    _clipPlanes.push_back(clipPlanes);
}

const PushConstants& RenderPackage::pushConstants(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_pushConstants.size()), "RenderPackage::pushConstants error: Invalid push constants index!");
    return _pushConstants[index]._constants;
}

void RenderPackage::pushConstants(I32 index, const PushConstants& constants) {
    DIVIDE_ASSERT(index < to_I32(_pushConstants.size()), "RenderPackage::pushConstants error: Invalid push constants index!");
    _pushConstants[index]._constants = constants;
    SetBit(_dirtyFlags, CommandType::PUSH_CONSTANTS);
}

void RenderPackage::addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants) {
    CommandEntry entry;
    entry._type = GFX::CommandType::SEND_PUSH_CONSTANTS;
    entry._index = _pushConstants.size();
    _commandOrdering.push_back(entry);

    _pushConstants.push_back(pushConstants);
}

const DescriptorSet_ptr& RenderPackage::descriptorSet(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    assert(_descriptorSets[index]._set != nullptr);
    return _descriptorSets[index]._set;
}

DescriptorSet_ptr& RenderPackage::descriptorSet(I32 index) {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    assert(_descriptorSets[index]._set != nullptr);
    return _descriptorSets[index]._set;
}

void RenderPackage::descriptorSet(I32 index, const DescriptorSet_ptr& descriptorSets) {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    assert(descriptorSets != nullptr);
    _descriptorSets[index]._set = descriptorSets;
    SetBit(_dirtyFlags, CommandType::DESCRIPTOR_SETS);
}

void RenderPackage::addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets) {
    CommandEntry entry;
    entry._type = GFX::CommandType::BIND_DESCRIPTOR_SETS;
    entry._index = _descriptorSets.size();
    _commandOrdering.push_back(entry);
    assert(descriptorSets._set != nullptr);
    _descriptorSets.push_back(descriptorSets);
}

void RenderPackage::addCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
   const vectorEASTL<GFX::CommandBuffer::CommandEntry>& commands = commandBuffer();
    for (const GFX::CommandBuffer::CommandEntry& cmd : commands) {
        switch (cmd.first) {
            case GFX::CommandType::DRAW_COMMANDS: {
                addDrawCommand(commandBuffer.getCommand<GFX::DrawCommand>(cmd));
            } break;
            case GFX::CommandType::BIND_PIPELINE: {
                addPipelineCommand(commandBuffer.getCommand<GFX::BindPipelineCommand>(cmd));
            } break;
            case GFX::CommandType::SET_CLIP_PLANES: {
                addClipPlanesCommand(commandBuffer.getCommand<GFX::SetClipPlanesCommand>(cmd));
            } break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                addPushConstantsCommand(commandBuffer.getCommand<GFX::SendPushConstantsCommand>(cmd));
            } break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                addDescriptorSetsCommand(commandBuffer.getCommand<GFX::BindDescriptorSetsCommand>(cmd));
            } break;
            default:
            case GFX::CommandType::COUNT: {
                DIVIDE_ASSERT(false,
                              "RenderPackage::addCommandBuffer error: Specified command buffer "
                              "contains commands that are not supported for this stage");
            } break;
        };
    }
}

const GFX::CommandBuffer& RenderPackage::commands() const {
    return *_commands;
}

GFX::CommandBuffer& RenderPackage::buildAndGetCommandBuffer(bool cacheMiss) {
    cacheMiss = false;
    //ToDo: Try to rebuild only the affected bits and pieces. That's why we have multiple dirty flags -Ionut
    if (_commands == nullptr || _dirtyFlags != 0) {
        cacheMiss = true;

        if (_commands == nullptr) {
            _commands = GFX::allocateCommandBuffer(_secondaryCommandPool);
        }
        GFX::CommandBuffer& buffer = *_commands;

        buffer.clear();
        for (const CommandEntry& cmd : _commandOrdering) {
            switch (cmd._type) {
                case GFX::CommandType::DRAW_COMMANDS: {
                    GFX::EnqueueCommand(buffer, _drawCommands[cmd._index]);
                } break;
                case GFX::CommandType::BIND_PIPELINE: {
                    GFX::EnqueueCommand(buffer, _pipelines[cmd._index]);
                } break;
                case GFX::CommandType::SET_CLIP_PLANES: {
                    GFX::EnqueueCommand(buffer, _clipPlanes[cmd._index]);
                } break;
                case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                    GFX::EnqueueCommand(buffer, _pushConstants[cmd._index]);
                } break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    GFX::EnqueueCommand(buffer, _descriptorSets[cmd._index]);
                } break;
                default:
                case GFX::CommandType::COUNT: {
                    DIVIDE_ASSERT(false,
                                  "RenderPackage::addCommandBuffer error: Specified command buffer "
                                  "contains commands that are not supported for this stage");
                } break;
            }
        }

        _dirtyFlags = to_base(CommandType::NONE);
    }

    return *_commands;
}

}; //namespace Divide