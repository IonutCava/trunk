#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

RenderPackage::RenderPackage() noexcept
    : _commands(GFX::allocateCommandBuffer(true)),
      _drawCommandOptions(to_base(CmdRenderOptions::RENDER_GEOMETRY)),
      _drawCommandCount(0),
      _qualityRequirement(MinQuality::FULL),
      _isInstanced(false)
{
    _lodIndexOffsets.fill({ 0u, 0u });
}

RenderPackage::~RenderPackage()
{
    GFX::deallocateCommandBuffer(_commands, true);
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
    _commands->clear();
    _commands->add(*other._commands);
    textureDataDirty(other.textureDataDirty());
    _isInstanced = other._isInstanced;
}

void RenderPackage::setLoDIndexOffset(U8 lodIndex, size_t indexOffset, size_t indexCount) noexcept {
    if (lodIndex < _lodIndexOffsets.size()) {
        _lodIndexOffsets[lodIndex] = std::make_pair(indexOffset, indexCount);
    }
}

size_t RenderPackage::getSortKeyHash() const {
    if (_commands->count<GFX::BindPipelineCommand>() > 0) {
        return _commands->get<GFX::BindPipelineCommand>(0)->_pipeline->getHash();
    }

    return 0;
}

const GFX::DrawCommand& RenderPackage::drawCommand(I32 index) const {
    DIVIDE_ASSERT(index < drawCommandCount(), "RenderPackage::drawCommand error: Invalid draw command index!");
    return *_commands->get<GFX::DrawCommand>(index);
}

const GenericDrawCommand& RenderPackage::drawCommand(I32 index, I32 cmdIndex) const {
    DIVIDE_ASSERT(index < drawCommandCount(), "RenderPackage::drawCommand error: Invalid draw command index!");
    
    const GFX::DrawCommand & cmd = drawCommand(index);
    DIVIDE_ASSERT(cmdIndex < to_I32(cmd._drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    return cmd._drawCommands[cmdIndex];
}

void RenderPackage::drawCommand(I32 index, I32 cmdIndex, const GenericDrawCommand& cmd) {
    DIVIDE_ASSERT(index < drawCommandCount(), "RenderPackage::drawCommand error: Invalid draw command index!");
    
    GFX::DrawCommand* drawCmd = _commands->get<GFX::DrawCommand>(index);
    DIVIDE_ASSERT(cmdIndex < to_I32(drawCmd->_drawCommands.size()), "RenderPackage::drawCommand error: Invalid draw command sub-index!");
    drawCmd->_drawCommands[cmdIndex] = cmd;
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    const bool wasInstanced = _isInstanced;

    GFX::DrawCommand* newCmd = _commands->add(cmd);
    for (GenericDrawCommand& drawCmd : newCmd->_drawCommands) {
        Divide::enableOptions(drawCmd, _drawCommandOptions);
        _isInstanced = drawCmd._cmd.primCount > 1 || _isInstanced;
    }
    ++_drawCommandCount;

    if (_isInstanced && !wasInstanced) {
        PushConstants constants = {};
        constants.set(_ID("DATA_IDX"), GFX::PushConstantType::UINT, 0u);
        addPushConstantsCommand(GFX::SendPushConstantsCommand{ constants });
    }
}

void RenderPackage::setDrawOption(CmdRenderOptions option, bool state) {
    if (BitCompare(_drawCommandOptions, option) == state) {
        return;
    }
    if (state) {
        SetBit(_drawCommandOptions, option);
    } else {
        ClearBit(_drawCommandOptions, option);
    }

    const auto& cmds = _commands->get<GFX::DrawCommand>();
    for (auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            setOption(drawCmd, option, state);
        }
    }
}

void RenderPackage::enableOptions(U16 optionMask) {
    if (AllCompare(_drawCommandOptions, optionMask)) {
        return;
    }
    SetBit(_drawCommandOptions, optionMask);

    auto& cmds = _commands->get<GFX::DrawCommand>();
    for (auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            Divide::enableOptions(drawCmd, optionMask);
        }
    }
}

void RenderPackage::disableOptions(U16 optionMask) {
    if (!AllCompare(_drawCommandOptions, optionMask)) {
        return;
    }
    ClearBit(_drawCommandOptions, optionMask);

    auto& cmds = _commands->get<GFX::DrawCommand>();
    for (auto& cmd : cmds) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*cmd);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            Divide::disableOptions(drawCmd, optionMask);
        }
    }
}

const Pipeline* RenderPackage::pipeline(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::BindPipelineCommand>()), "RenderPackage::pipeline error: Invalid pipeline index!");
    return _commands->get<GFX::BindPipelineCommand>(index)->_pipeline;
}

void RenderPackage::pipeline(I32 index, const Pipeline& pipeline) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::BindPipelineCommand>()), "RenderPackage::pipeline error: Invalid pipeline index!");
    _commands->get<GFX::BindPipelineCommand>(index)->_pipeline = &pipeline;
}

void RenderPackage::addPipelineCommand(const GFX::BindPipelineCommand& pipeline) {
    _commands->add(pipeline);
}

const FrustumClipPlanes& RenderPackage::clipPlanes(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::SetClipPlanesCommand>()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    return _commands->get<GFX::SetClipPlanesCommand>(index)->_clippingPlanes;
}

void RenderPackage::clipPlanes(I32 index, const FrustumClipPlanes& clipPlanes) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::SetClipPlanesCommand>()), "RenderPackage::clipPlanes error: Invalid clip plane list index!");
    _commands->get<GFX::SetClipPlanesCommand>(index)->_clippingPlanes = clipPlanes;
}

void RenderPackage::addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes) {
    _commands->add(clipPlanes);
}

PushConstants& RenderPackage::pushConstants(I32 index) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()), "RenderPackage::pushConstants error: Invalid push constants index!");
    return _commands->get<GFX::SendPushConstantsCommand>(index)->_constants;
}

const PushConstants& RenderPackage::pushConstants(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()), "RenderPackage::pushConstants error: Invalid push constants index!");
    return _commands->get<GFX::SendPushConstantsCommand>(index)->_constants;
}

void RenderPackage::pushConstants(I32 index, const PushConstants& constants) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::SendPushConstantsCommand>()), "RenderPackage::pushConstants error: Invalid push constants index!");
    _commands->get<GFX::SendPushConstantsCommand>(index)->_constants = constants;
}

void RenderPackage::addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants) {
    _commands->add(pushConstants);
}

DescriptorSet& RenderPackage::descriptorSet(I32 index) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
}

const DescriptorSet& RenderPackage::descriptorSet(I32 index) const {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    return _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
}

void RenderPackage::descriptorSet(I32 index, const DescriptorSet& descriptorSets) {
    DIVIDE_ASSERT(index < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    
    DescriptorSet & set = _commands->get<GFX::BindDescriptorSetsCommand>(index)->_set;
    if (set != descriptorSets) {
        set = descriptorSets;
    }
}

void RenderPackage::addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets) {
    _commands->add(descriptorSets);
}

void RenderPackage::addCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    _commands->add(commandBuffer);
    _drawCommandCount += to_I32(commandBuffer.count<GFX::DrawCommand>());
}

void RenderPackage::addShaderBuffer(I32 descriptorSetIndex, const ShaderBufferBinding& buffer) {
    DIVIDE_ASSERT(descriptorSetIndex < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    
    GFX::BindDescriptorSetsCommand* cmd = _commands->get<GFX::BindDescriptorSetsCommand>(descriptorSetIndex);
    cmd->_set.addShaderBuffer(buffer);
}

void RenderPackage::setTexture(I32 descriptorSetIndex, const TextureData& data, U8 binding) {
    DIVIDE_ASSERT(descriptorSetIndex < to_I32(_commands->count<GFX::BindDescriptorSetsCommand>()), "RenderPackage::descriptorSet error: Invalid descriptor set index!");
    GFX::BindDescriptorSetsCommand* cmd = _commands->get<GFX::BindDescriptorSetsCommand>(descriptorSetIndex);
    cmd->_set._textureData.setTexture(data, binding);
}

void RenderPackage::updateDrawCommands(U32 dataIndex, U32 startOffset, U8 lodLevel) {
    OPTICK_EVENT();

    lodLevel = std::min(lodLevel, to_U8(_lodIndexOffsets.size() - 1));
    const auto& [offset, count] = _lodIndexOffsets[lodLevel];

    const GFX::CommandBuffer::Container::EntryList& cmds = _commands->get<GFX::DrawCommand>();
    const bool autoIndex = autoIndexBuffer() && (offset != 0u || count != 0u);

    for (GFX::CommandBase* cmd : cmds) {
        GFX::DrawCommand::CommandContainer& drawCommands = static_cast<GFX::DrawCommand&>(*cmd)._drawCommands;
        for (GenericDrawCommand& drawCmd : drawCommands) {
            drawCmd._commandOffset = startOffset++;

            drawCmd._cmd.baseInstance = (_isInstanced || drawCmd._cmd.primCount > 1u) ? 0u : dataIndex;
            drawCmd._cmd.firstIndex = autoIndex ? to_U32(offset) : drawCmd._cmd.firstIndex;
            drawCmd._cmd.indexCount = autoIndex ? to_U32(count) : drawCmd._cmd.indexCount;
        }
    }

    if (_isInstanced) {
        assert(_commands->exists<GFX::SendPushConstantsCommand>(0));
        pushConstants(0).set(_ID("DATA_IDX"), GFX::PushConstantType::UINT, dataIndex);
    }
}

}; //namespace Divide