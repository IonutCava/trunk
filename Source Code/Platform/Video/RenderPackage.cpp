#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

RenderPackage::RenderPackage(bool useSecondaryBuffers)
    : _qualityRequirement(MinQuality::FULL),
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
    _pipelines.clear();
    _clipPlanes.clear();
    _pushConstants.clear();
    _descriptorSets.clear();

    _dirtyFlags = to_base(CommandType::ALL);
}

void RenderPackage::set(const RenderPackage& other) {
    _commandOrdering = other._commandOrdering;

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
    GFX::CommandBuffer::CommandEntry entry;
    entry._typeIndex = static_cast<vec_size_eastl>(GFX::CommandType::DRAW_COMMANDS);
    entry._elementIndex = _drawCommands.size();
    _commandOrdering.push_back(entry);

    _drawCommands.push_back(cmd);
    SetBit(_dirtyFlags, CommandType::DRAW);
}

void RenderPackage::setDrawOption(CmdRenderOptions option, bool state) {
    for (GFX::DrawCommand& cmd : _drawCommands) {
        for (GenericDrawCommand& drawCmd : cmd._drawCommands) {
            setOption(drawCmd, option, state);
        }
    }
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
    GFX::CommandBuffer::CommandEntry entry;
    entry._typeIndex = static_cast<vec_size_eastl>(GFX::CommandType::BIND_PIPELINE);
    entry._elementIndex = _pipelines.size();
    _commandOrdering.push_back(entry);

    _pipelines.push_back(pipeline);
    SetBit(_dirtyFlags, CommandType::PIPELINE);
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
    GFX::CommandBuffer::CommandEntry entry;
    entry._typeIndex = static_cast<vec_size_eastl>(GFX::CommandType::SET_CLIP_PLANES);
    entry._elementIndex = _clipPlanes.size();
    _commandOrdering.push_back(entry);

    _clipPlanes.push_back(clipPlanes);
    SetBit(_dirtyFlags, CommandType::CLIP_PLANES);
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
    GFX::CommandBuffer::CommandEntry entry;
    entry._typeIndex = static_cast<vec_size_eastl>(GFX::CommandType::SEND_PUSH_CONSTANTS);
    entry._elementIndex = _pushConstants.size();
    _commandOrdering.push_back(entry);

    _pushConstants.push_back(pushConstants);

    SetBit(_dirtyFlags, CommandType::PUSH_CONSTANTS);
}

const DescriptorSet_ptr& RenderPackage::descriptorSet(I32 index) const {
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
    GFX::CommandBuffer::CommandEntry entry;
    entry._typeIndex = static_cast<vec_size_eastl>(GFX::CommandType::BIND_DESCRIPTOR_SETS);
    entry._elementIndex = _descriptorSets.size();
    _commandOrdering.push_back(entry);
    assert(descriptorSets._set != nullptr);
    _descriptorSets.push_back(descriptorSets);
    SetBit(_dirtyFlags, CommandType::DESCRIPTOR_SETS);
}

void RenderPackage::addCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
   const vectorEASTL<GFX::CommandBuffer::CommandEntry>& commands = commandBuffer();
    for (const GFX::CommandBuffer::CommandEntry& cmd : commands) {
        switch (cmd.type<GFX::CommandType::_enumerated>()) {
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

void RenderPackage::setDataIndex(U32 dataIndex) {
    bool dirty = false;
    for (GFX::DrawCommand& cmd : _drawCommands) {
        for (GenericDrawCommand& drawCmd : cmd._drawCommands) {
            if (drawCmd._cmd.baseInstance != dataIndex) {
                drawCmd._cmd.baseInstance = dataIndex;
                dirty = true;
            }
        }
    }
    if (dirty) {
        SetBit(_dirtyFlags, CommandType::DRAW);
    }
}

void RenderPackage::updateDrawCommands(U32 startOffset) {
    bool dirty = false;
    for (GFX::DrawCommand& cmd : _drawCommands) {
        for (GenericDrawCommand& drawCmd : cmd._drawCommands) {
            if (drawCmd._commandOffset != startOffset) {
                drawCmd._commandOffset = startOffset;
                dirty = true;
            }
            ++startOffset;
        }
    }

    if (dirty) {
        SetBit(_dirtyFlags, CommandType::DRAW);
    }
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
        for (const GFX::CommandBuffer::CommandEntry& cmd : _commandOrdering) {
            switch (cmd.type<GFX::CommandType::_enumerated>()) {
                case GFX::CommandType::DRAW_COMMANDS: {
                    GFX::EnqueueCommand(buffer, _drawCommands[cmd._elementIndex]);
                } break;
                case GFX::CommandType::BIND_PIPELINE: {
                    GFX::EnqueueCommand(buffer, _pipelines[cmd._elementIndex]);
                } break;
                case GFX::CommandType::SET_CLIP_PLANES: {
                    GFX::EnqueueCommand(buffer, _clipPlanes[cmd._elementIndex]);
                } break;
                case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                    GFX::EnqueueCommand(buffer, _pushConstants[cmd._elementIndex]);
                } break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    GFX::EnqueueCommand(buffer, _descriptorSets[cmd._elementIndex]);
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