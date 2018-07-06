#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

namespace GenericDrawCommandResults {
    hashMapImpl<I64, QueryResult> g_queryResults;
};

IndirectDrawCommand::IndirectDrawCommand()
    : indexCount(0),
      primCount(1),
      firstIndex(0),
      baseVertex(0),
      baseInstance(0)
{
    static_assert(sizeof(IndirectDrawCommand) == 20, "Size of IndirectDrawCommand is incorrect!");
}

void IndirectDrawCommand::set(const IndirectDrawCommand& other) {
    indexCount = other.indexCount;
    primCount = other.primCount;
    firstIndex = other.firstIndex;
    baseVertex = other.baseVertex;
    baseInstance = other.baseInstance;
}

bool IndirectDrawCommand::operator==(const IndirectDrawCommand &other) const {
    return indexCount == other.indexCount &&
           primCount == other.primCount &&
           firstIndex == other.firstIndex &&
           baseVertex == other.baseVertex &&
           baseInstance == other.baseInstance;
}

GenericDrawCommand::GenericDrawCommand()
    : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
{
}

GenericDrawCommand::GenericDrawCommand(PrimitiveType type,
                                       U32 firstIndex,
                                       U32 indexCount,
                                       U32 primCount)
  : _lodIndex(0),
    _drawCount(1),
    _drawToBuffer(0),
    _type(type),
    _commandOffset(0),
    _patchVertexCount(4),
    _sourceBuffer(nullptr),
    _renderOptions(to_base(RenderOptions::RENDER_GEOMETRY))
{
    _cmd.indexCount = indexCount;
    _cmd.firstIndex = firstIndex;
    _cmd.primCount = primCount;

    static_assert(sizeof(GenericDrawCommand) == 56, "Size of GenericDrawCommand is incorrect!");
}

void GenericDrawCommand::set(const GenericDrawCommand& base) {
    _cmd.set(base._cmd);
    _lodIndex = base._lodIndex;
    _drawCount = base._drawCount;
    _drawToBuffer = base._drawToBuffer;
    _renderOptions = base._renderOptions;
    _type = base._type;
    _sourceBuffer = base._sourceBuffer;
    _commandOffset = base._commandOffset;
}

void GenericDrawCommand::reset() {
    set(GenericDrawCommand());
}

bool GenericDrawCommand::compatible(const GenericDrawCommand& other) const {
    return _lodIndex == other._lodIndex &&
           _drawToBuffer == other._drawToBuffer &&
           _renderOptions == other._renderOptions &&
           _type == other._type &&
           (_sourceBuffer != nullptr) == (other._sourceBuffer != nullptr);
}

bool GenericDrawCommand::isEnabledOption(RenderOptions option) const {
    return BitCompare(_renderOptions, to_U32(option));
}

void GenericDrawCommand::enableOption(RenderOptions option) {
    SetBit(_renderOptions, to_U32(option));
}

void GenericDrawCommand::disableOption(RenderOptions option) {
    ClearBit(_renderOptions, to_U32(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option) {
    toggleOption(option, !isEnabledOption(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option, const bool state) {
    if (state) {
        enableOption(option);
    } else {
        disableOption(option);
    }
}

GenericCommandBuffer::GenericCommandBuffer()
{

}

void GenericCommandBuffer::rebuildCaches() {
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

void GenericCommandBuffer::batch() {
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

void GenericCommandBuffer::clean() {
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
        if (_data[i-1]->_type == _data[i]->_type && _data[i]->_type == CommandType::BIND_PIPELINE) {
            redundantEntries.push_back(i - 1);
        }
    }

    erase_indices(_data, redundantEntries);
}

}; //namespace Divide
