#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/CommandBufferPool.h"
#include "Headers/GFXDevice.h"

namespace Divide {

RenderPackage::RenderPackage() noexcept
{
    _lodIndexOffsets.fill({ 0u, 0u });
}

RenderPackage::~RenderPackage()
{
    if (_commands != nullptr) {
        GFX::deallocateCommandBuffer(_commands);
    }
}

void RenderPackage::clear() {
    if (!empty()) {
        _commands->clear();
        _drawCommandCount = 0;
    }
    textureDataDirty(true);
    _isInstanced = false;
    assert(_drawCommandCount == 0);
}

void RenderPackage::set(const RenderPackage& other) {
    clear();
    if (other._commands != nullptr) {
        commands()->add(*other._commands);
    }
    textureDataDirty(other.textureDataDirty());
    _isInstanced = other._isInstanced;
}

void RenderPackage::setLoDIndexOffset(const U8 lodIndex, size_t indexOffset, size_t indexCount) noexcept {
    if (lodIndex < _lodIndexOffsets.size()) {
        _lodIndexOffsets[lodIndex] = std::make_pair(indexOffset, indexCount);
    }
}

size_t RenderPackage::getSortKeyHash() const {
    if (_commands != nullptr && _commands->count<GFX::BindPipelineCommand>() > 0) {
        return _commands->get<GFX::BindPipelineCommand>(0)->_pipeline->getHash();
    }

    return 0;
}

const GFX::DrawCommand& RenderPackage::drawCommand(I32 index) const {
    return *_commands->get<GFX::DrawCommand>(index);
}

const GenericDrawCommand& RenderPackage::drawCommand(I32 index, I32 cmdIndex) const {
    assert(_commands != nullptr && index < drawCommandCount() && "RenderPackage::drawCommand error: Invalid draw command index!");
    
    const GFX::DrawCommand & cmd = drawCommand(index);
    assert(cmdIndex < to_I32(cmd._drawCommands.size()) && "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    return cmd._drawCommands[cmdIndex];
}

void RenderPackage::drawCommand(const I32 index, const I32 cmdIndex, const GenericDrawCommand& cmd) const {
    assert(_commands != nullptr && index < drawCommandCount() && "RenderPackage::drawCommand error: Invalid draw command index!");
    
    GFX::DrawCommand* drawCmd = _commands->get<GFX::DrawCommand>(index);
    assert(cmdIndex < to_I32(drawCmd->_drawCommands.size()) && "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    drawCmd->_drawCommands[cmdIndex] = cmd;
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    const bool wasInstanced = _isInstanced;

    GFX::DrawCommand newCmd = cmd;
    for (GenericDrawCommand& drawCmd : newCmd._drawCommands) {
        Divide::enableOptions(drawCmd, _drawCommandOptions);
        _isInstanced = drawCmd._cmd.primCount > 1 || _isInstanced;
    }
    ++_drawCommandCount;

    if (_isInstanced && !wasInstanced) {
        if (!_commands->exists<GFX::SendPushConstantsCommand>(0)) {
            GFX::SendPushConstantsCommand constantsCmd = {};
            constantsCmd._constants.set(_ID("DATA_IDX"), GFX::PushConstantType::UINT, 0u);
            add(constantsCmd);
        }
    }

    commands()->add(newCmd);
}

void RenderPackage::setDrawOption(const CmdRenderOptions option, const bool state) {
    if (BitCompare(_drawCommandOptions, option) == state) {
        return;
    }
    if (state) {
        SetBit(_drawCommandOptions, option);
    } else {
        ClearBit(_drawCommandOptions, option);
    }

    const auto& cmds = commands()->get<GFX::DrawCommand>();
    for (const auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            setOption(drawCmd, option, state);
        }
    }
}

void RenderPackage::enableOptions(const U16 optionMask) {
    if (AllCompare(_drawCommandOptions, optionMask)) {
        return;
    }
    SetBit(_drawCommandOptions, optionMask);

    const auto& cmds = commands()->get<GFX::DrawCommand>();
    for (const auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            Divide::enableOptions(drawCmd, optionMask);
        }
    }
}

void RenderPackage::disableOptions(const U16 optionMask) {
    if (!AllCompare(_drawCommandOptions, optionMask)) {
        return;
    }
    ClearBit(_drawCommandOptions, optionMask);

    const auto& cmds = commands()->get<GFX::DrawCommand>();
    for (const auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            Divide::disableOptions(drawCmd, optionMask);
        }
    }
}

const Pipeline* RenderPackage::pipeline(const I32 index) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::BindPipelineCommand>()) && "RenderPackage::pipeline error: Invalid pipeline index!");
    return _commands->get<GFX::BindPipelineCommand>(index)->_pipeline;
}

void RenderPackage::pipeline(const I32 index, const Pipeline& pipeline) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::BindPipelineCommand>()) && "RenderPackage::pipeline error: Invalid pipeline index!");
    _commands->get<GFX::BindPipelineCommand>(index)->_pipeline = &pipeline;
}

const FrustumClipPlanes& RenderPackage::clipPlanes(const I32 index) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::SetClipPlanesCommand>()) && "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    return _commands->get<GFX::SetClipPlanesCommand>(index)->_clippingPlanes;
}

void RenderPackage::clipPlanes(const I32 index, const FrustumClipPlanes& clipPlanes) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::SetClipPlanesCommand>()) && "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    _commands->get<GFX::SetClipPlanesCommand>(index)->_clippingPlanes = clipPlanes;
}

PushConstants& RenderPackage::pushConstants(const I32 index) {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()) && "RenderPackage::pushConstants error: Invalid push constants index!");
    return _commands->get<GFX::SendPushConstantsCommand>(index)->_constants;
}

const PushConstants& RenderPackage::pushConstants(const I32 index) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()) && "RenderPackage::pushConstants error: Invalid push constants index!");
    return _commands->get<GFX::SendPushConstantsCommand>(index)->_constants;
}

void RenderPackage::pushConstants(const I32 index, const PushConstants& constants) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()) && "RenderPackage::pushConstants error: Invalid push constants index!");
    _commands->get<GFX::SendPushConstantsCommand>(index)->_constants = constants;
}

DescriptorSet& RenderPackage::descriptorSet(const I32 index) {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()) && "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
}

const DescriptorSet& RenderPackage::descriptorSet(const I32 index) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()) && "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
}

void RenderPackage::descriptorSet(const I32 index, const DescriptorSet& descriptorSets) const {
    assert(_commands != nullptr && index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()) && "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    
    DescriptorSet & set = _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
    if (set != descriptorSets) {
        set = descriptorSets;
    }
}

void RenderPackage::addCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    commands()->add(commandBuffer);
    _drawCommandCount += to_I32(commandBuffer.count<GFX::DrawCommand>());
}

const ShaderBufferBinding& RenderPackage::getShaderBuffer(const I32 descriptorSetIndex, const I32 bufferIndex) const {
    const DescriptorSet& set = descriptorSet(descriptorSetIndex);
    assert(bufferIndex < to_I32(set._shaderBuffers.size()) && "RenderPackage::getShaderBuffer error: Invalid buffer sub-index!");

    return set._shaderBuffers[bufferIndex];
}

void RenderPackage::addShaderBuffer(const I32 descriptorSetIndex, const ShaderBufferBinding& buffer) const {
    assert(_commands != nullptr && descriptorSetIndex < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()) && "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    
    GFX::BindDescriptorSetsCommand* cmd = _commands->get<GFX::BindDescriptorSetsCommand>(descriptorSetIndex);
    cmd->_set.addShaderBuffer(buffer);
}

void RenderPackage::setTexture(const I32 descriptorSetIndex, const TextureData& data, size_t samplerHash, U8 binding) const {
    assert(_commands != nullptr && descriptorSetIndex < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()) && "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    GFX::BindDescriptorSetsCommand* cmd = _commands->get<GFX::BindDescriptorSetsCommand>(descriptorSetIndex);
    cmd->_set._textureData.setTexture(data, samplerHash, binding);
}

void RenderPackage::updateDrawCommands(const U32 dataIndex, U32 startOffset, U8 lodLevel) {
    OPTICK_EVENT();

    {
        lodLevel = std::min(lodLevel, to_U8(_lodIndexOffsets.size() - 1));
        const auto& [offset, count] = _lodIndexOffsets[lodLevel];

        const GFX::CommandBuffer::Container::EntryList& cmds = commands()->get<GFX::DrawCommand>();
        const bool autoIndex = (offset != 0u || count != 0u);

        for (GFX::CommandBase* cmd : cmds) {
            GFX::DrawCommand::CommandContainer& drawCommands = static_cast<GFX::DrawCommand&>(*cmd)._drawCommands;
            for (GenericDrawCommand& drawCmd : drawCommands) {
                drawCmd._commandOffset = startOffset++;

                drawCmd._cmd.baseInstance = (_isInstanced || drawCmd._cmd.primCount > 1u) ? 0u : dataIndex;
                if (autoIndex) {
                    drawCmd._cmd.firstIndex = to_U32(offset);
                    drawCmd._cmd.indexCount = to_U32(count);
                }
            }
        }
    }
    if (_isInstanced) {
        const I32 count = to_I32(_commands->count<GFX::SendPushConstantsCommand>());
        for (I32 i = 0; i < count; ++i) {
            pushConstants(i).set(_ID("DATA_IDX"), GFX::PushConstantType::UINT, dataIndex);
        }
    }

    lastDataIndex(dataIndex);
}

GFX::CommandBuffer* RenderPackage::commands() {
    if (_commands == nullptr) {
        _commands = GFX::allocateCommandBuffer();
    }

    return _commands;
}
}; //namespace Divide