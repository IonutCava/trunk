#include "stdafx.h"

#include "Headers/RenderPackage.h"

#include "Headers/CommandBufferPool.h"
#include "Headers/GFXDevice.h"

namespace Divide {

RenderPackage::~RenderPackage()
{
    deallocateCommandBuffer(_commands);
}

void RenderPackage::clear() {
    if (_commands != nullptr) {
        _commands->clear();
    }
    textureDataDirty(true);
    _isInstanced = false;
    _drawCommandCount = 0u;
}

void RenderPackage::setLoDIndexOffset(const U8 lodIndex, size_t indexOffset, size_t indexCount) noexcept {
    if (lodIndex < _lodIndexOffsets.size()) {
        _lodIndexOffsets[lodIndex] = std::make_pair(indexOffset, indexCount);
    }
}

void RenderPackage::addDrawCommand(const GFX::DrawCommand& cmd) {
    const bool wasInstanced = _isInstanced;

    GFX::DrawCommand newCmd = cmd;
    for (GenericDrawCommand& drawCmd : newCmd._drawCommands) {
        Divide::enableOptions(drawCmd, _drawCommandOptions);
        if (drawCmd._cmd.primCount > 1 && !_isInstanced) {
            _isInstanced = true;
            if (!wasInstanced && !_commands->exists<GFX::SendPushConstantsCommand>(0)) {
                GFX::SendPushConstantsCommand constantsCmd = {};
                constantsCmd._constants.set(_ID("DATA_INDICES"), GFX::PushConstantType::UINT, 0u);
                add(constantsCmd);
            }
        }
    }

    _drawCommandCount += to_U32(newCmd._drawCommands.size());
    _commands->add(newCmd);
}

void RenderPackage::setDrawOptions(const BaseType<CmdRenderOptions> optionMask, const bool state) {
    if (AllCompare(_drawCommandOptions, optionMask) == state) {
        return;
    }

    ToggleBit(_drawCommandOptions, optionMask, state);

    const auto& drawCommands = commands()->get<GFX::DrawCommand>();
    for (const auto& drawCommandEntry : drawCommands) {
        auto& drawCommand = static_cast<GFX::DrawCommand&>(*drawCommandEntry);
        for (GenericDrawCommand& drawCmd : drawCommand._drawCommands) {
            setOptions(drawCmd, optionMask, state);
        }
    }
}

void RenderPackage::setDrawOption(const CmdRenderOptions option, const bool state) {
    setDrawOptions(to_base(option), state);
}

void RenderPackage::enableOptions(const BaseType<CmdRenderOptions> optionMask) {
    setDrawOptions(optionMask, true);
}

void RenderPackage::disableOptions(const BaseType<CmdRenderOptions> optionMask) {
    setDrawOptions(optionMask, false);
}

void RenderPackage::appendCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    commands()->add(commandBuffer);
}

void RenderPackage::updateDrawCommands() {
    OPTICK_EVENT();

    U32 startOffset = _lastCommandOffset;
    const GFX::CommandBuffer::Container::EntryList& drawCommandEntries = commands()->get<GFX::DrawCommand>();
    for (GFX::CommandBase* cmd : drawCommandEntries) {
        GFX::DrawCommand::CommandContainer& drawCommands = static_cast<GFX::DrawCommand&>(*cmd)._drawCommands;
        for (GenericDrawCommand& drawCmd : drawCommands) {
            drawCmd._commandOffset = startOffset++;
        }
    }
}

U32 RenderPackage::updateAndRetrieveDrawCommands(const NodeDataIdx dataIndex, U32 startOffset, const U8 lodLevel, DrawCommandContainer& cmdsInOut) {
    OPTICK_EVENT();

    const U32 dataIndices = ((dataIndex._transformIDX << 16) | dataIndex._materialIDX);
    const auto& [offset, count] = _lodIndexOffsets[std::min(lodLevel, to_U8(_lodIndexOffsets.size() - 1))];
    const bool autoIndex = offset != 0u || count != 0u;

    const GFX::CommandBuffer::Container::EntryList& drawCommandEntries = commands()->get<GFX::DrawCommand>();

    U16 ret = 0u;
    _lastCommandOffset = startOffset;
    for (GFX::CommandBase* cmd : drawCommandEntries) {
        GFX::DrawCommand::CommandContainer& drawCommands = static_cast<GFX::DrawCommand&>(*cmd)._drawCommands;
        for (GenericDrawCommand& drawCmd : drawCommands) {

            drawCmd._commandOffset = startOffset++;
            drawCmd._cmd.baseInstance = _isInstanced ? 0u : dataIndices;
            if (autoIndex) {
                drawCmd._cmd.firstIndex = to_U32(offset);
                drawCmd._cmd.indexCount = to_U32(count);
            }
            cmdsInOut.push_back(drawCmd._cmd);
            ++ret;
        }
    }

    if (_isInstanced) {
        const U16 pushCount = to_U16(_commands->count<GFX::SendPushConstantsCommand>());
        for (U16 i = 0; i < pushCount; ++i) {
            _commands->get<GFX::SendPushConstantsCommand>(i)->_constants.set(_ID("DATA_INDICES"), GFX::PushConstantType::UINT, dataIndices);
        }
    }

    return ret;
}

GFX::CommandBuffer* RenderPackage::commands() {
    if (_commands == nullptr) {
        _commands = GFX::allocateCommandBuffer();
    }

    return _commands;
}
}; //namespace Divide