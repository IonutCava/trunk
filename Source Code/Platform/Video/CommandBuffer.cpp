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
void BeginRenderSubPass(CommandBuffer& buffer, const BeginRenderSubPassCommand& cmd) {
    buffer.add(cmd);
}
void EndRenderSubPass(CommandBuffer& buffer, const EndRenderSubPassCommand& cmd) {
    buffer.add(cmd);
}
void SetViewPort(CommandBuffer& buffer, const SetViewportCommand& cmd) {
    buffer.add(cmd);
}
void SetScissor(CommandBuffer& buffer, const SetScissorCommand& cmd) {
    buffer.add(cmd);
}
void BindPipeline(CommandBuffer& buffer, const BindPipelineCommand& cmd) {
    buffer.add(cmd);
}
void SendPushConstants(CommandBuffer& buffer, const SendPushConstantsCommand& cmd) {
    buffer.add(cmd);
}
void BindDescripotSets(CommandBuffer& buffer, const BindDescriptorSetsCommand& cmd) {
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


CommandBuffer::CommandBuffer()
{

}

void CommandBuffer::rebuildCaches() {
    _pipelineCache.resize(0);
    for (const std::shared_ptr<Command>& cmd : _data) {
        if (cmd->_type == CommandType::BIND_PIPELINE) {
            Pipeline& pipeline = std::dynamic_pointer_cast<BindPipelineCommand>(cmd)->_pipeline;
            _pipelineCache.push_back(&pipeline);
            if (pipeline.shaderProgram() == nullptr) {
                int al;
                al = 5;
            }
        }
    }

    _pushConstantsCache.resize(0);
    for (const std::shared_ptr<Command>& cmd : _data) {
        if (cmd->_type == CommandType::SEND_PUSH_CONSTANTS) {
            PushConstants& constants = std::dynamic_pointer_cast<SendPushConstantsCommand>(cmd)->_constants;
            _pushConstantsCache.push_back(&constants);
        }
    }

    _descriptorSetCache.resize(0);
    for (const std::shared_ptr<Command>& cmd : _data) {
        if (cmd->_type == CommandType::BIND_DESCRIPTOR_SETS) {
            DescriptorSet& set = std::dynamic_pointer_cast<BindDescriptorSetsCommand>(cmd)->_set;
            _descriptorSetCache.push_back(&set);
        }
    }

    _drawCommandsCache.resize(0);
    for (const std::shared_ptr<Command>& cmd : _data) {
        if (cmd->_type == CommandType::DRAW_COMMANDS) {
            vectorImpl<GenericDrawCommand>& drawCommands = std::dynamic_pointer_cast<DrawCommand>(cmd)->_drawCommands;
            for (GenericDrawCommand& drawCmd : drawCommands) {
                _drawCommandsCache.push_back(&drawCmd);
            }
        }
    }
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
            vectorImpl<GenericDrawCommand>& cmds = std::dynamic_pointer_cast<DrawCommand>(cmd)->_drawCommands;

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
            return std::dynamic_pointer_cast<DrawCommand>(cmd)->_drawCommands.empty();
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