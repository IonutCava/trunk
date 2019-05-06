#include "stdafx.h"

#include "Headers/CommandBuffer.h"
#include "Core/Headers/Console.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

namespace Divide {

namespace GFX {

DEFINE_POOL(BindPipelineCommand);
DEFINE_POOL(SendPushConstantsCommand);
DEFINE_POOL(DrawCommand);
DEFINE_POOL(SetViewportCommand);
DEFINE_POOL(BeginRenderPassCommand);
DEFINE_POOL(EndRenderPassCommand);
DEFINE_POOL(BeginPixelBufferCommand);
DEFINE_POOL(EndPixelBufferCommand);
DEFINE_POOL(BeginRenderSubPassCommand);
DEFINE_POOL(EndRenderSubPassCommand);
DEFINE_POOL(BlitRenderTargetCommand);
DEFINE_POOL(ResetRenderTargetCommand);
DEFINE_POOL(ResolveRenderTargetCommand);
DEFINE_POOL(CopyTextureCommand);
DEFINE_POOL(ComputeMipMapsCommand);
DEFINE_POOL(SetScissorCommand);
DEFINE_POOL(SetBlendCommand);
DEFINE_POOL(SetCameraCommand);
DEFINE_POOL(SetClipPlanesCommand);
DEFINE_POOL(BindDescriptorSetsCommand);
DEFINE_POOL(BeginDebugScopeCommand);
DEFINE_POOL(EndDebugScopeCommand);
DEFINE_POOL(DrawTextCommand);
DEFINE_POOL(DrawIMGUICommand);
DEFINE_POOL(DispatchComputeCommand);
DEFINE_POOL(MemoryBarrierCommand);
DEFINE_POOL(ReadBufferDataCommand);
DEFINE_POOL(ClearBufferDataCommand);
DEFINE_POOL(ExternalCommand);

void CommandBuffer::add(const CommandBuffer& other) {
    for (const CommandEntry& cmd : other._commandOrder) {
        other.get<CommandBase>(cmd).addToBuffer(*this);
    }
}

void CommandBuffer::addDestructive(CommandBuffer& other) {
    add(other);
    other.clear(false);
}

void CommandBuffer::batch() {
    clean();

    std::array<CommandBase*, to_base(GFX::CommandType::COUNT)> prevCommands;

    bool tryMerge = true;

    bool partial = false;
    while (tryMerge) {
        prevCommands.fill(nullptr);
        tryMerge = false;
        bool skip = false;
        for (auto it = std::begin(_commandOrder); it != std::cend(_commandOrder);) {
            skip = false;

            const CommandEntry& cmd = *it;

            auto type = static_cast<GFX::CommandType::_enumerated>(cmd._typeIndex);
            CommandBase& crtCommand = get<CommandBase>(cmd);
            CommandBase*& prevCommand = prevCommands[to_U16(type)];

            if (prevCommand != nullptr && tryMergeCommands(type, prevCommand, &crtCommand, partial)) {
                it = _commandOrder.erase(it);
                --_commandCount[to_base(type)];
                skip = true;
                tryMerge = true;
            } else {
                prevCommands.fill(nullptr);
            }
            

            if (!skip) {
                prevCommand = &crtCommand;
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

        switch (static_cast<GFX::CommandType::_enumerated>(cmd._typeIndex)) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                // We may just wish to clear the RT
                const BeginRenderPassCommand& crtCmd = get<BeginRenderPassCommand>(cmd);
                if (crtCmd._descriptor.stateMask() != 0) {
                    hasWork = true;
                    break;
                }
            } break;
            case GFX::CommandType::READ_BUFFER_DATA:
            case GFX::CommandType::CLEAR_BUFFER_DATA:
            case GFX::CommandType::DISPATCH_COMPUTE:
            case GFX::CommandType::MEMORY_BARRIER:
            case GFX::CommandType::DRAW_TEXT:
            case GFX::CommandType::DRAW_COMMANDS:
            case GFX::CommandType::DRAW_IMGUI:
            //case GFX::CommandType::BIND_DESCRIPTOR_SETS:
            //case GFX::CommandType::BIND_PIPELINE:
            case GFX::CommandType::BLIT_RT:
            case GFX::CommandType::RESET_RT:
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
            case GFX::CommandType::RESOLVE_RT: {
                const ResolveRenderTargetCommand& crtCmd = get<ResolveRenderTargetCommand>(cmd);
                hasWork = crtCmd._resolveColours || crtCmd._resolveDepth;
            } break;
            case GFX::CommandType::COPY_TEXTURE: {
                const CopyTextureCommand& crtCmd = get<CopyTextureCommand>(cmd);
                hasWork = crtCmd._source != nullptr && crtCmd._destination != nullptr;
            }break;
        };
    }

    if (!hasWork) {
        _commandOrder.resize(0);
        _commandCount.fill(0);
        return;
    }
}

void CommandBuffer::clean() {
    if (_commandOrder.empty()) {
        return;
    }

    bool skip = false;
    const Pipeline* prevPipeline = nullptr;
    const DescriptorSet* prevDescriptorSet = nullptr;

    for (auto it = eastl::begin(_commandOrder); it != eastl::cend(_commandOrder);) {
        skip = false;
        CommandEntry cmd = *it;

        switch (static_cast<GFX::CommandType::_enumerated>(cmd._typeIndex)) {
            case CommandType::DRAW_COMMANDS :
            {
                vectorEASTLFast<GenericDrawCommand>& cmds = get<DrawCommand>(cmd)._drawCommands;

                auto beginIt = eastl::begin(cmds);
                auto endIt = eastl::end(cmds);
                cmds.erase(eastl::remove_if(beginIt, endIt, [](const GenericDrawCommand& cmd) -> bool { return cmd._drawCount == 0; }), endIt);
                if (cmds.empty()) {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                }
            } break;
            case CommandType::BIND_PIPELINE : {
                const Pipeline* pipeline = get<BindPipelineCommand>(cmd)._pipeline;
                // If the current pipeline is identical to the previous one, remove it
                if (prevPipeline != nullptr && *prevPipeline == *pipeline) {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                }
                prevPipeline = pipeline;
            }break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                PushConstants& constants = get<SendPushConstantsCommand>(cmd)._constants;
                if (constants.data().empty()) {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                }
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                const DescriptorSet& set = get<BindDescriptorSetsCommand>(cmd)._set;
                if (prevDescriptorSet != nullptr && *prevDescriptorSet == set) {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                }
                if (!skip && 
                    set._shaderBuffers.empty() &&
                    set._textureData.textures().empty() &&
                    set._textureViews.empty() &&
                    set._images.empty())
                {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                } else {
                    prevDescriptorSet = &set;
                }
            }break;
            case GFX::CommandType::DRAW_TEXT: {
                const TextElementBatch& batch = get<DrawTextCommand>(cmd)._batch;
                bool hasText = !batch._data.empty();
                if (hasText) {
                    hasText = false;
                    for (const TextElement& element : batch()) {
                        hasText = hasText || !element.text().empty();
                    }
                }
                if (!hasText) {
                    it = _commandOrder.erase(it);
                    --_commandCount[cmd._typeIndex];
                    skip = true;
                }
            }break;
            default: break;
        };


        if (!skip) {
            ++it;
        }
    }

    // Remove redundant pipeline changes

    auto entry = eastl::begin(_commandOrder); ++entry;
    for (; entry != eastl::cend(_commandOrder);) {
        auto prev = eastl::prev(entry);
        GFX::CommandType type = static_cast<GFX::CommandType::_enumerated>(entry->_typeIndex);

        if (prev->_typeIndex == type && type._value == CommandType::BIND_PIPELINE) {
            entry = _commandOrder.erase(entry);
            --_commandCount[entry->_typeIndex];
        } else {
            ++entry;
        }
    }
}

// New use cases that emerge from production work should be checked here.
bool CommandBuffer::validate() const {
    bool valid = true;

    if (Config::ENABLE_GPU_VALIDATION) {
        bool pushedPass = false, pushedSubPass = false, pushedPixelBuffer = false;
        bool hasPipeline = false, needsPipeline = false;
        bool hasDescriptorSets = false, needsDescriptorSets = false;
        U32 pushedDebugScope = 0;

        for (const CommandEntry& cmd : _commandOrder) {
            switch (static_cast<GFX::CommandType::_enumerated>(cmd._typeIndex)) {
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
                default: {
                    // no requirements yet
                }break;
            };
        }

        valid = !pushedPass && !pushedSubPass && !pushedPixelBuffer && pushedDebugScope == 0;

        if (needsDescriptorSets) {
            valid = hasDescriptorSets && valid;
        }

        if (needsPipeline && !hasPipeline) {
            valid = false;
        }
    }

    return valid;
}


bool CommandBuffer::mergeDrawCommands(vectorEASTLFast<GenericDrawCommand>& commands, bool byBaseInstance) const {
    const size_t startSize = commands.size();

    std::sort(std::begin(commands),
                std::end(commands),
                [byBaseInstance](const GenericDrawCommand & a, const GenericDrawCommand & b) -> bool
                {
                    return (byBaseInstance ? a._cmd.baseInstance < b._cmd.baseInstance
                                           : a._commandOffset < b._commandOffset);
                });

    auto batch = [byBaseInstance](GenericDrawCommand& previousIDC, GenericDrawCommand& currentIDC)  -> bool {
        bool instanced = false;
        // Instancing is not compatible with MDI. Well, it might be, but I can't be bothered a.t.m. to implement it -Ionut
        if (previousIDC._cmd.primCount > 1 || currentIDC._cmd.primCount > 1) {
            instanced = previousIDC._cmd.primCount != currentIDC._cmd.primCount;
        }

        // Batchable commands must share the same buffer and other various state
        if (!instanced && compatible(previousIDC, currentIDC)) {
            bool merge = false;
            if (byBaseInstance) { // Base instance compatibility
                merge = previousIDC._cmd.baseInstance + previousIDC._drawCount == currentIDC._cmd.baseInstance;
            } else {// Command offset compatibility
                merge = previousIDC._commandOffset + previousIDC._drawCount == currentIDC._commandOffset;
            }

            if (merge) {
                // If the rendering commands are batchable, increase the draw count for the previous one
                previousIDC._drawCount += currentIDC._drawCount;
                // And set the current command's draw count to zero so it gets removed from the list later on
                currentIDC._drawCount = 0;
                return true;
            }
        }

        return false;
    };

    vec_size previousCommandIndex = 0;
    vec_size currentCommandIndex = 1;
    const vec_size commandCount = commands.size();
    for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
        GenericDrawCommand& previousCommand = commands[previousCommandIndex];
        GenericDrawCommand& currentCommand = commands[currentCommandIndex];
        if (!batch(previousCommand, currentCommand)) {
            previousCommandIndex = currentCommandIndex;
        }
    }

    commands.erase(eastl::remove_if(eastl::begin(commands),
                                eastl::end(commands),
                                [](const GenericDrawCommand& cmd) -> bool {
                                    return cmd._drawCount == 0;
                                }),
                eastl::end(commands));

    return startSize - commands.size() > 0;
}

void CommandBuffer::toString(const GFX::CommandBase& cmd, GFX::CommandType::_enumerated type, I32& crtIndent, stringImpl& out) const {
    auto append = [](stringImpl& target, const stringImpl& text, I32 indent) {
        for (I32 i = 0; i < indent; ++i) {
            target.append("    ");
        }
        target.append(text);
    };

    switch (type) {
        case GFX::CommandType::BEGIN_RENDER_PASS:
        case GFX::CommandType::BEGIN_PIXEL_BUFFER:
        case GFX::CommandType::BEGIN_RENDER_SUB_PASS: 
        case GFX::CommandType::BEGIN_DEBUG_SCOPE : {
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
            ++crtIndent;
        }break;
        case GFX::CommandType::END_RENDER_PASS:
        case GFX::CommandType::END_PIXEL_BUFFER:
        case GFX::CommandType::END_RENDER_SUB_PASS:
        case GFX::CommandType::END_DEBUG_SCOPE: {
            --crtIndent;
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
        }break;
        default: {
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
        }break;
    }
}

stringImpl CommandBuffer::toString() const {
    I32 crtIndent = 0;
    stringImpl out = "\n\n\n\n";
    for (const CommandEntry& cmd : _commandOrder) {
        toString(get<CommandBase>(cmd), static_cast<GFX::CommandType::_enumerated>(cmd._typeIndex), crtIndent, out);
        out.append("\n");
    }
    out.append("\n\n\n\n");

    assert(crtIndent == 0);

    return out;
}

}; //namespace GFX
}; //namespace Divide