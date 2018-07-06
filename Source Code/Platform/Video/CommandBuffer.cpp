#include "stdafx.h"

#include "Headers/CommandBuffer.h"

namespace Divide {
namespace GFX {

void BeginRenderPass(CommandBuffer& buffer, const BeginRenderPassCommand& cmd) {
    buffer.add(cmd);
}
void EndRenderPass(CommandBuffer& buffer, const EndRenderPassCommand& cmd) {
    buffer.add(cmd);
}
void BeginPixelBuffer(CommandBuffer& buffer, const BeginPixelBufferCommand& cmd) {
    buffer.add(cmd);
}
void EndPixelBuffer(CommandBuffer& buffer, const EndPixelBufferCommand& cmd) {
    buffer.add(cmd);
}
void BeginRenderSubPass(CommandBuffer& buffer, const BeginRenderSubPassCommand& cmd) {
    buffer.add(cmd);
}
void EndRenderSubPass(CommandBuffer& buffer, const EndRenderSubPassCommand& cmd) {
    buffer.add(cmd);
}
void BlitRenderTarget(CommandBuffer& buffer, const BlitRenderTargetCommand& cmd) {
    buffer.add(cmd);
}
void SetViewPort(CommandBuffer& buffer, const SetViewportCommand& cmd) {
    buffer.add(cmd);
}
void SetScissor(CommandBuffer& buffer, const SetScissorCommand& cmd) {
    buffer.add(cmd);
}
void SetCamera(CommandBuffer& buffer, const SetCameraCommand& cmd) {
    buffer.add(cmd);
}
void SetClipPlanes(CommandBuffer& buffer, const SetClipPlanesCommand& cmd) {
    buffer.add(cmd);
}
void BindPipeline(CommandBuffer& buffer, const BindPipelineCommand& cmd) {
    buffer.add(cmd);
}
void SendPushConstants(CommandBuffer& buffer, const SendPushConstantsCommand& cmd) {
    buffer.add(cmd);
}
void BindDescriptorSets(CommandBuffer& buffer, const BindDescriptorSetsCommand& cmd) {
    buffer.add(cmd);
}
void BeginDebugScope(CommandBuffer& buffer, const BeginDebugScopeCommand& cmd) {
    buffer.add(cmd);
}
void EndDebugScope(CommandBuffer& buffer, const EndDebugScopeCommand& cmd) {
    buffer.add(cmd);
}
void AddDrawCommands(CommandBuffer& buffer, const DrawCommand& cmd) {
    buffer.add(cmd);
}
void AddDrawTextCommand(CommandBuffer& buffer, const DrawTextCommand& cmd) {
    buffer.add(cmd);
}
void AddComputeCommand(CommandBuffer& buffer, const DispatchComputeCommand& cmd) {
    buffer.add(cmd);
}


CommandBuffer::CommandBuffer(size_t index)
    : _index(index)
{
}

CommandBuffer::~CommandBuffer()
{
}


void CommandBuffer::batch() {
    /*auto batch = [](GenericDrawCommand& previousIDC,
    GenericDrawCommand& currentIDC)  -> bool {
    if (previousIDC.compatible(currentIDC) &&
    // Batchable commands must share the same buffer
    previousIDC.sourceBuffer()->getGUID() == currentIDC.sourceBuffer()->getGUID())
    {
    U32 prevCount = previousIDC.drawCount();
    if (previousIDC.cmd().baseInstance + prevCount != currentIDC.cmd().baseInstance) {
    return false;
    }
    // If the rendering commands are batchable, increase the draw count for the previous one
    previousIDC.drawCount(to_U16(prevCount + currentIDC.drawCount()));
    // And set the current command's draw count to zero so it gets removed from the list later on
    currentIDC.drawCount(0);

    return true;
    }

    return false;
    };


    vectorAlg::vecSize previousCommandIndex = 0;
    vectorAlg::vecSize currentCommandIndex = 1;
    const vectorAlg::vecSize commandCount = commands.size();
    for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
    GenericDrawCommand& previousCommand = commands[previousCommandIndex];
    GenericDrawCommand& currentCommand = commands[currentCommandIndex];
    if (!batch(previousCommand, currentCommand))
    {
    previousCommandIndex = currentCommandIndex;
    }
    }

    commands.erase(std::remove_if(std::begin(commands),
    std::end(commands),
    [](const GenericDrawCommand& cmd) -> bool {
    return cmd.drawCount() == 0;
    }),
    std::end(commands));
    */
}

void CommandBuffer::clean() {
    if (_data.empty()) {
        return;
    }

    for (const std::shared_ptr<Command>& cmd : _data) {
        if (cmd->_type == CommandType::DRAW_COMMANDS) {
            vectorImpl<GenericDrawCommand>& cmds = static_cast<DrawCommand*>(cmd.get())->_drawCommands;

            cmds.erase(std::remove_if(std::begin(cmds),
                                      std::end(cmds),
                                      [](const GenericDrawCommand& cmd) -> bool {
                return cmd.drawCount() == 0;
            }),
                       std::end(cmds));
        }
    }

    _data.erase(std::remove_if(std::begin(_data),
                               std::end(_data),
                               [](const std::shared_ptr<Command>& cmd) -> bool {
                                  if (cmd->_type == CommandType::DRAW_COMMANDS) {
                                      return static_cast<DrawCommand*>(cmd.get())->_drawCommands.empty();
                                  }
                                  return false;
                              }),
                std::end(_data));


    // Remove redundant pipeline changes
    vectorAlg::vecSize size = _data.size();
    vectorImpl<vectorAlg::vecSize> redundantEntries;
    for (vectorAlg::vecSize i = 1; i < size; ++i) {
        if (_data[i - 1]->_type == _data[i]->_type && _data[i]->_type == CommandType::BIND_PIPELINE) {
            redundantEntries.push_back(i - 1);
        }
    }

    erase_indices(_data, redundantEntries);
}
}; //namespace GFX
}; //namespace Divide