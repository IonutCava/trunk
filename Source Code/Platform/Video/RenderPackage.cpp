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
      _commands(nullptr),
      _drawCommandDirty(true),
      _pipelineDirty(true),
      _clipPlanesDirty(true),
      _pushConstantsDirty(true),
      _descriptorSetsDirty(true)
{
}

RenderPackage::~RenderPackage()
{
    if (_commands != nullptr) {
        GFX::deallocateCommandBuffer(_commands, _secondaryCommandPool);
    }
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
        return _pipelines.front()._pipeline.getHash();
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
    _drawCommandDirty = true;
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    CommandEntry entry;
    entry._type = GFX::CommandType::DRAW_COMMANDS;
    entry._index = _drawCommands.size();
    _commandOrdering.push_back(entry);

    _drawCommands.push_back(cmd);
}

const Pipeline& RenderPackage::pipeline(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    return _pipelines[index]._pipeline;
}

void RenderPackage::pipeline(I32 index, const Pipeline& pipeline) {
    DIVIDE_ASSERT(index < to_I32(_pipelines.size()), "RenderPackage::pipeline error: Invalid pipeline index!");
    _pipelines[index]._pipeline.fromDescriptor(pipeline.toDescriptor());
    _pipelineDirty = true;
}

void RenderPackage::addPipelineCommand(const GFX::BindPipelineCommand& pipeline) {
    CommandEntry entry;
    entry._type = GFX::CommandType::BIND_PIPELINE;
    entry._index = _pipelines.size();
    _commandOrdering.push_back(entry);

    _pipelines.push_back(pipeline);
}

const ClipPlaneList& RenderPackage::clipPlanes(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    return _clipPlanes[index]._clippingPlanes;
}

void RenderPackage::clipPlanes(I32 index, const ClipPlaneList& clipPlanes) {
    DIVIDE_ASSERT(index < to_I32(_clipPlanes.size()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    _clipPlanes[index]._clippingPlanes = clipPlanes;
    _clipPlanesDirty = true;
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
    _pushConstantsDirty = true;
}

void RenderPackage::addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants) {
    CommandEntry entry;
    entry._type = GFX::CommandType::SEND_PUSH_CONSTANTS;
    entry._index = _pushConstants.size();
    _commandOrdering.push_back(entry);

    _pushConstants.push_back(pushConstants);
}

const DescriptorSet& RenderPackage::descriptorSet(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _descriptorSets[index]._set;
}

void RenderPackage::descriptorSet(I32 index, const DescriptorSet& descriptorSets) {
    DIVIDE_ASSERT(index < to_I32(_descriptorSets.size()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    _descriptorSets[index]._set = descriptorSets;
    _descriptorSetsDirty = true;
}

void RenderPackage::addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets) {
    CommandEntry entry;
    entry._type = GFX::CommandType::BIND_DESCRIPTOR_SETS;
    entry._index = _descriptorSets.size();
    _commandOrdering.push_back(entry);

    _descriptorSets.push_back(descriptorSets);
}

void RenderPackage::addCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    const vectorImpl<std::shared_ptr<GFX::Command>>& commands = commandBuffer();
    for (const std::shared_ptr<GFX::Command>& cmd : commands) {
        switch (cmd->_type) {
            case GFX::CommandType::DRAW_COMMANDS: {
                addDrawCommand(static_cast<GFX::DrawCommand&>(*cmd.get()));
            } break;
            case GFX::CommandType::BIND_PIPELINE: {
                addPipelineCommand(static_cast<GFX::BindPipelineCommand&>(*cmd.get()));
            } break;
            case GFX::CommandType::SET_CLIP_PLANES: {
                addClipPlanesCommand(static_cast<GFX::SetClipPlanesCommand&>(*cmd.get()));
            } break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                addPushConstantsCommand(static_cast<GFX::SendPushConstantsCommand&>(*cmd.get()));
            } break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                addDescriptorSetsCommand(static_cast<GFX::BindDescriptorSetsCommand&>(*cmd.get()));
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

bool RenderPackage::buildCommandBuffer() {
    //ToDo: Try to rebuild only the affected bits and pieces. That's why we have multiple dirty flags -Ionut
    if (_commands == nullptr || _drawCommandDirty || _pipelineDirty || _clipPlanesDirty || _pushConstantsDirty || _descriptorSetsDirty) {
        if (_commands == nullptr) {
            _commands = GFX::allocateCommandBuffer(_secondaryCommandPool);
        }

        for (const CommandEntry& cmd : _commandOrdering) {
            switch (cmd._type) {
                case GFX::CommandType::DRAW_COMMANDS: {
                    addDrawCommand(_drawCommands[cmd._index]);
                } break;
                case GFX::CommandType::BIND_PIPELINE: {
                    addPipelineCommand(_pipelines[cmd._index]);
                } break;
                case GFX::CommandType::SET_CLIP_PLANES: {
                    addClipPlanesCommand(_clipPlanes[cmd._index]);
                } break;
                case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                    addPushConstantsCommand(_pushConstants[cmd._index]);
                } break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    addDescriptorSetsCommand(_descriptorSets[cmd._index]);
                } break;
                default:
                case GFX::CommandType::COUNT: {
                    DIVIDE_ASSERT(false,
                                  "RenderPackage::addCommandBuffer error: Specified command buffer "
                                  "contains commands that are not supported for this stage");
                } break;
            }
        }

        _drawCommandDirty = _pipelineDirty = _clipPlanesDirty = _pushConstantsDirty = _descriptorSetsDirty = false;
        return true;
    }
    return false;
}

}; //namespace Divide