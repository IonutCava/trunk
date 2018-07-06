#include "stdafx.h"

#include "Headers/CommandBuffer.h"
#include "Core/Headers/Console.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

namespace Divide {
namespace GFX {

CommandBuffer::CommandBuffer()
{
    _commands.register_types<BeginRenderPassCommand,
                             EndRenderPassCommand,
                             BeginPixelBufferCommand,
                             EndPixelBufferCommand,
                             BeginRenderSubPassCommand,
                             EndRenderSubPassCommand,
                             BeginDebugScopeCommand,
                             EndDebugScopeCommand,
                             SetViewportCommand,
                             SetScissorCommand,
                             SetBlendCommand,
                             SetCameraCommand,
                             SetClipPlanesCommand,
                             BlitRenderTargetCommand,
                             BindPipelineCommand,
                             BindDescriptorSetsCommand,
                             SendPushConstantsCommand,
                             SwitchWindowCommand,
                             DrawCommand,
                             DrawTextCommand,
                             DrawIMGUICommand,
                             DispatchComputeCommand>();
    _commands.reserve(25);
}

CommandBuffer::~CommandBuffer()
{
}


const std::type_info& CommandBuffer::getType(GFX::CommandType type) const {
    switch (type) {
    case GFX::CommandType::BEGIN_RENDER_PASS:     return typeid(BeginRenderPassCommand);
    case GFX::CommandType::END_RENDER_PASS:       return typeid(EndRenderPassCommand);
    case GFX::CommandType::BEGIN_PIXEL_BUFFER:    return typeid(BeginPixelBufferCommand);
    case GFX::CommandType::END_PIXEL_BUFFER:      return typeid(EndPixelBufferCommand);
    case GFX::CommandType::BEGIN_RENDER_SUB_PASS: return typeid(BeginRenderSubPassCommand);
    case GFX::CommandType::END_RENDER_SUB_PASS:   return typeid(EndRenderSubPassCommand);
    case GFX::CommandType::BLIT_RT:               return typeid(BlitRenderTargetCommand);
    case GFX::CommandType::BIND_DESCRIPTOR_SETS:  return typeid(BindDescriptorSetsCommand);
    case GFX::CommandType::BIND_PIPELINE:         return typeid(BindPipelineCommand);
    case GFX::CommandType::SEND_PUSH_CONSTANTS:   return typeid(SendPushConstantsCommand);
    case GFX::CommandType::SET_SCISSOR:           return typeid(SetScissorCommand);
    case GFX::CommandType::SET_BLEND:             return typeid(SetBlendCommand);
    case GFX::CommandType::SET_VIEWPORT:          return typeid(SetViewportCommand);
    case GFX::CommandType::SET_CAMERA:            return typeid(SetCameraCommand);
    case GFX::CommandType::SET_CLIP_PLANES:       return typeid(SetClipPlanesCommand);
    case GFX::CommandType::BEGIN_DEBUG_SCOPE:     return typeid(BeginDebugScopeCommand);
    case GFX::CommandType::END_DEBUG_SCOPE:       return typeid(EndDebugScopeCommand);
    case GFX::CommandType::DRAW_TEXT:             return typeid(DrawTextCommand);
    case GFX::CommandType::DRAW_IMGUI:            return typeid(DrawIMGUICommand);
    case GFX::CommandType::DISPATCH_COMPUTE:      return typeid(DispatchComputeCommand);
    case GFX::CommandType::SWITCH_WINDOW:         return typeid(SwitchWindowCommand);
    };

    return typeid(DrawCommand);
}

const GFX::Command& CommandBuffer::getCommand(const CommandBuffer::CommandEntry& commandEntry) const {
    return *(_commands.begin(getType(commandEntry.first)) + commandEntry.second);
}

GFX::Command* CommandBuffer::getCommandInternal(const CommandBuffer::CommandEntry& commandEntry) {
    return &(*(_commands.begin(getType(commandEntry.first)) + commandEntry.second));
}

const GFX::Command* CommandBuffer::getCommandInternal(const CommandBuffer::CommandEntry& commandEntry) const {
    return &(*(_commands.begin(getType(commandEntry.first)) + commandEntry.second));
}

void CommandBuffer::add(const CommandBuffer& other) {
    if (!other.empty()) {
        for (const CommandEntry& cmd : other._commandOrder) {
            _commands.insert(*other.getCommandInternal(cmd));
            _commandOrder.emplace_back(cmd.first, _commands.size(getType(cmd.first)) - 1);
        }
    }
}

void CommandBuffer::batch() {
    clean();

    std::array<Command*, to_base(GFX::CommandType::COUNT)> prevCommands;
    vectorEASTL<CommandEntry>::iterator it;

    bool tryMerge = true;

    while (tryMerge) {
        prevCommands.fill(nullptr);
        tryMerge = false;
        bool skip = false;
        for (it = std::begin(_commandOrder); it != std::cend(_commandOrder);) {
            skip = false;

            const CommandEntry& cmd = *it;
            if (resetMerge(cmd.first)) {
                prevCommands.fill(nullptr);
            }

            Command* crtCommand = getCommandInternal(cmd);
            Command* prevCommand = prevCommands[to_U16(cmd.first)];
            
            if (prevCommand != nullptr && prevCommand->_type == cmd.first) {
                if (tryMergeCommands(prevCommand, crtCommand)) {
                    it = _commandOrder.erase(it);
                    skip = true;
                    tryMerge = true;
                }
            }
            if (!skip) {
                prevCommand = crtCommand;
                ++it;
            }
        }
    }
    
    // If we don't have any actual work to do, clear everything
    bool hasWork = false;
    for (const CommandEntry& cmd : _commandOrder) {
        if (hasWork) {
            break;
        }

        switch (cmd.first) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                // We may just wish to clear the RT
                const GFX::BeginRenderPassCommand& crtCmd = static_cast<const GFX::BeginRenderPassCommand&>(getCommand(cmd));
                if (crtCmd._descriptor.stateMask() != 0) {
                    hasWork = true;
                    break;
                }
            } break;
            case GFX::CommandType::DISPATCH_COMPUTE:
            case GFX::CommandType::DRAW_TEXT:
            case GFX::CommandType::DRAW_COMMANDS:
            case GFX::CommandType::DRAW_IMGUI:
            case GFX::CommandType::BIND_DESCRIPTOR_SETS:
            case GFX::CommandType::BIND_PIPELINE:
            case GFX::CommandType::BLIT_RT:
            case GFX::CommandType::SEND_PUSH_CONSTANTS:
            case GFX::CommandType::BEGIN_PIXEL_BUFFER:
            case GFX::CommandType::SET_CAMERA:
            case GFX::CommandType::SET_CLIP_PLANES:
            case GFX::CommandType::SET_SCISSOR:
            case GFX::CommandType::SET_BLEND:
            case GFX::CommandType::SET_VIEWPORT:
            case GFX::CommandType::SWITCH_WINDOW: {
                hasWork = true;
                break;
            }break;
                
        };
    }

    if (!hasWork) {
        _commandOrder.clear();
        return;
    }
}

void CommandBuffer::clean() {
    if (_commandOrder.empty()) {
        return;
    }

    bool skip = false;
    const Pipeline* prevPipeline = nullptr;
    DescriptorSet prevDescriptorSet;

    vectorEASTL<CommandEntry>::iterator it;
    for (it = std::begin(_commandOrder); it != std::cend(_commandOrder);) {
        skip = false;
        const CommandEntry& cmd = *it;

        switch (cmd.first) {
            case CommandType::DRAW_COMMANDS :
            {
                vectorEASTL<GenericDrawCommand>& cmds = static_cast<DrawCommand*>(getCommandInternal(cmd))->_drawCommands;

                cmds.erase(eastl::remove_if(eastl::begin(cmds),
                                          eastl::end(cmds),
                                          [](const GenericDrawCommand& cmd) -> bool {
                                              return cmd.drawCount() == 0;
                                          }),
                           eastl::end(cmds));

                if (cmds.empty()) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            } break;
            case CommandType::BIND_PIPELINE : {
                const Pipeline* pipeline = static_cast<BindPipelineCommand*>(getCommandInternal(cmd))->_pipeline;
                // If the current pipeline is identical to the previous one, remove it
                if (prevPipeline != nullptr && *prevPipeline == *pipeline) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
                prevPipeline = pipeline;
            }break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                PushConstants& constants = static_cast<SendPushConstantsCommand*>(getCommandInternal(cmd))->_constants;
                if (constants.data().empty()) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                const DescriptorSet& set = static_cast<BindDescriptorSetsCommand*>(getCommandInternal(cmd))->_set;
                if (prevDescriptorSet == set) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
                prevDescriptorSet = set;
            }break;
            case GFX::CommandType::DRAW_TEXT: {
                const TextElementBatch& batch = static_cast<DrawTextCommand*>(getCommandInternal(cmd))->_batch;
                bool hasText = !batch._data.empty();
                if (hasText) {
                    hasText = false;
                    for (const TextElement& element : batch()) {
                        hasText = hasText || !element.text().empty();
                    }
                }
                if (!hasText) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            }break;
            case GFX::CommandType::BEGIN_RENDER_PASS:
            case GFX::CommandType::END_RENDER_PASS:
            case GFX::CommandType::BEGIN_RENDER_SUB_PASS:
            case GFX::CommandType::END_RENDER_SUB_PASS:
            case GFX::CommandType::BEGIN_DEBUG_SCOPE:
            case GFX::CommandType::END_DEBUG_SCOPE:
            case GFX::CommandType::BEGIN_PIXEL_BUFFER:
            case GFX::CommandType::END_PIXEL_BUFFER:
            case GFX::CommandType::DISPATCH_COMPUTE:
            case GFX::CommandType::DRAW_IMGUI:
            case GFX::CommandType::BLIT_RT:
            case GFX::CommandType::SWITCH_WINDOW: {
            }break;
        };


        if (!skip) {
            ++it;
        }
    }

    // Remove redundant pipeline changes
    vec_size size = _commandOrder.size();

    vector<vec_size> redundantEntries;
    for (vec_size i = 1; i < size; ++i) {
        if (_commandOrder[i - 1].first== _commandOrder[i].first && _commandOrder[i].first == CommandType::BIND_PIPELINE) {
            redundantEntries.push_back(i - 1);
        }
    }

    erase_indices(_commandOrder, redundantEntries);
}

//ToDo: This needs to grow to handle every possible scenario - Ionut
bool CommandBuffer::validate() const {
    bool valid = true;

    if (Config::ENABLE_GPU_VALIDATION) {
        bool pushedPass = false, pushedSubPass = false, pushedPixelBuffer = false;
        bool hasPipeline = false, needsPipeline = false;
        bool hasDescriptorSets = false, needsDescriptorSets = false;
        U32 pushedDebugScope = 0;

        for (const CommandEntry& cmd : _commandOrder) {
            switch (cmd.first) {
                case GFX::CommandType::BEGIN_RENDER_PASS: {
                    if (pushedPass) {
                        return false;
                    }
                    pushedPass = true;
                } break;
                case GFX::CommandType::END_RENDER_PASS: {
                    if (!pushedPass) {
                        return false;
                    }
                    pushedPass = false;
                } break;
                case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
                    if (pushedSubPass) {
                        return false;
                    }
                    pushedSubPass = true;
                } break;
                case GFX::CommandType::END_RENDER_SUB_PASS: {
                    if (!pushedSubPass) {
                        return false;
                    }
                    pushedSubPass = false;
                } break;
                case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
                    ++pushedDebugScope;
                } break;
                case GFX::CommandType::END_DEBUG_SCOPE: {
                    if (pushedDebugScope == 0) {
                        return false;
                    }
                    --pushedDebugScope;
                } break;
                case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
                    if (pushedPixelBuffer) {
                        return false;
                    }
                    pushedPixelBuffer = true;
                }break;
                case GFX::CommandType::END_PIXEL_BUFFER: {
                    if (!pushedPixelBuffer) {
                        return false;
                    }
                    pushedPixelBuffer = false;
                }break;
                case GFX::CommandType::BIND_PIPELINE: {
                    hasPipeline = true;
                } break;
                case GFX::CommandType::DISPATCH_COMPUTE: 
                case GFX::CommandType::DRAW_TEXT:
                case GFX::CommandType::DRAW_IMGUI:
                case GFX::CommandType::DRAW_COMMANDS: {
                    needsPipeline = true;
                }break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    hasDescriptorSets = true;
                }break;
                case GFX::CommandType::BLIT_RT: {
                    needsDescriptorSets = true;
                }break;
                case GFX::CommandType::SWITCH_WINDOW: {
                    // no requirements yet
                }break;
            };
        }

        valid = !pushedPass && !pushedSubPass && !pushedPixelBuffer &&
                pushedDebugScope == 0 &&
                (hasPipeline == needsPipeline);
        if (needsDescriptorSets) {
            valid = hasDescriptorSets && valid;
        }
    }

    return valid;
}

bool CommandBuffer::tryMergeCommands(GFX::Command* prevCommand, GFX::Command* crtCommand) const {
    assert(prevCommand != nullptr && crtCommand != nullptr && prevCommand->_type == crtCommand->_type);
    switch (crtCommand->_type) {
        case GFX::CommandType::DRAW_COMMANDS : {
            DrawCommand* crtDrawCommand = static_cast<DrawCommand*>(crtCommand);
            DrawCommand* prevDrawCommand = static_cast<DrawCommand*>(prevCommand);

            prevDrawCommand->_drawCommands.insert(eastl::cend(prevDrawCommand->_drawCommands),
                                                  eastl::cbegin(crtDrawCommand->_drawCommands),
                                                  eastl::cend(crtDrawCommand->_drawCommands));

            auto batch = [](GenericDrawCommand& previousIDC, GenericDrawCommand& currentIDC)  -> bool {
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

            
            vectorEASTL<GenericDrawCommand>& commands = prevDrawCommand->_drawCommands;
            vec_size previousCommandIndex = 0;
            vec_size currentCommandIndex = 1;
            const vec_size commandCount = commands.size();
            for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
                GenericDrawCommand& previousCommand = commands[previousCommandIndex];
                GenericDrawCommand& currentCommand = commands[currentCommandIndex];
                if (!batch(previousCommand, currentCommand))
                {
                    previousCommandIndex = currentCommandIndex;
                }
            }

            commands.erase(eastl::remove_if(eastl::begin(commands),
                                          eastl::end(commands),
                                          [](const GenericDrawCommand& cmd) -> bool {
                                              return cmd.drawCount() == 0;
                                          }),
                          eastl::end(commands));
            return true;
        }break;

        case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
            BindDescriptorSetsCommand* crtBindCommand = static_cast<BindDescriptorSetsCommand*>(crtCommand);
            BindDescriptorSetsCommand* prevBindCommand = static_cast<BindDescriptorSetsCommand*>(prevCommand);
            return prevBindCommand->_set.merge(crtBindCommand->_set);

        } break;

        case GFX::CommandType::SEND_PUSH_CONSTANTS: {
            SendPushConstantsCommand* crtPushConstants = static_cast<SendPushConstantsCommand*>(crtCommand);
            SendPushConstantsCommand* prevPushConstants = static_cast<SendPushConstantsCommand*>(prevCommand);
            return prevPushConstants->_constants.merge(crtPushConstants->_constants);
        } break;
        
    };

    return false;
}

bool CommandBuffer::resetMerge(GFX::CommandType type) const {
    return type != GFX::CommandType::DRAW_COMMANDS &&
           type != GFX::CommandType::BIND_DESCRIPTOR_SETS &&
           type != GFX::CommandType::SEND_PUSH_CONSTANTS;
}

void CommandBuffer::toString(const GFX::Command& cmd, I32& crtIndent, stringImpl& out) const {
    auto append = [](stringImpl& target, const stringImpl& text, I32 indent) {
        for (I32 i = 0; i < indent; ++i) {
            target.append("    ");
        }
        target.append(text);
    };

    switch (cmd._type) {
        case GFX::CommandType::BEGIN_RENDER_PASS: {
            const GFX::BeginRenderPassCommand& crtCmd = static_cast<const GFX::BeginRenderPassCommand&>(cmd);
            append(out, "BEGIN_RENDER_PASS: " + crtCmd._name, crtIndent);
            ++crtIndent;
        }break;
        case GFX::CommandType::END_RENDER_PASS: {
            --crtIndent;
            append(out, "END_RENDER_PASS", crtIndent);
        }break;
        case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
            append(out, "BEGIN_PIXEL_BUFFER", crtIndent);
            ++crtIndent;
        }break;
        case GFX::CommandType::END_PIXEL_BUFFER: {
            --crtIndent;
            append(out, "END_PIXEL_BUFFER", crtIndent);
        }break;
        case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
            append(out, "BEGIN_RENDER_SUB_PASS", crtIndent);
            ++crtIndent;
        }break;
        case GFX::CommandType::END_RENDER_SUB_PASS: {
            --crtIndent;
            append(out, "END_RENDER_SUB_PASS", crtIndent);
        }break;
        case GFX::CommandType::BLIT_RT: {
            append(out, "BLIT_RT", crtIndent);
        }break;
        case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
            append(out, "BIND_DESCRIPTOR_SETS", crtIndent);
        }break;
        case GFX::CommandType::BIND_PIPELINE: {
            append(out, "BIND_PIPELINE", crtIndent);
        } break;
        case GFX::CommandType::SEND_PUSH_CONSTANTS: {
            append(out, "SEND_PUSH_CONSTANTS", crtIndent);
        } break;
        case GFX::CommandType::SET_SCISSOR: {
            append(out, "SET_SCISSOR", crtIndent);
        }break;
        case GFX::CommandType::SET_BLEND: {
            append(out, "SET_BLEND", crtIndent);
        }break;
        case GFX::CommandType::SET_VIEWPORT: {
            Rect<I32> viewport = static_cast<const GFX::SetViewportCommand&>(cmd)._viewport;
            append(out, "SET_VIEWPORT: " + Util::StringFormat("[%d, %d, %d, %d]", viewport.x, viewport.y, viewport.z, viewport.w), crtIndent);
        }break;
        case GFX::CommandType::SET_CAMERA: {
            append(out, "SET_CAMERA", crtIndent);
        }break;
        case GFX::CommandType::SET_CLIP_PLANES: {
            append(out, "SET_CLIP_PLANES", crtIndent);
        }break;
        case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
            const GFX::BeginDebugScopeCommand& crtCmd = static_cast<const GFX::BeginDebugScopeCommand&>(cmd);
            append(out, "BEGIN_DEBUG_SCOPE: " + crtCmd._scopeName, crtIndent);
            ++crtIndent;
        } break;
        case GFX::CommandType::END_DEBUG_SCOPE: {
            --crtIndent;
            append(out, "END_DEBUG_SCOPE", crtIndent);
        } break;
        case GFX::CommandType::DRAW_TEXT: {
            append(out, "DRAW_TEXT", crtIndent);
        }break;
        case GFX::CommandType::DRAW_COMMANDS: {
            append(out, "DRAW_COMMANDS", crtIndent);
        }break;
        case GFX::CommandType::DRAW_IMGUI: {
            append(out, "DRAW_IMGUI", crtIndent);
        }break;
        case GFX::CommandType::DISPATCH_COMPUTE: {
            append(out, "DISPATCH_COMPUTE", crtIndent);
        }break;
        case GFX::CommandType::SWITCH_WINDOW: {
            append(out, "SWITCH_WINDOW", crtIndent);
        }break;
    }
}

stringImpl CommandBuffer::toString() const {
    I32 crtIndent = 0;
    stringImpl out = "\n\n\n\n";
    for (const CommandEntry& cmd : _commandOrder) {
        toString(getCommand(cmd), crtIndent, out);
        out.append("\n");
    }
    out.append("\n\n\n\n");

    assert(crtIndent == 0);

    return out;
}

}; //namespace GFX
}; //namespace Divide