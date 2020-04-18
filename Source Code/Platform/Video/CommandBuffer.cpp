#include "stdafx.h"

#include "Headers/CommandBuffer.h"
#include "Core/Headers/Console.h"
#include "Platform/Video/Textures/Headers/Texture.h"
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
DEFINE_POOL(ClearRenderTargetCommand);
DEFINE_POOL(ResetRenderTargetCommand);
DEFINE_POOL(ResetAndClearRenderTargetCommand);
DEFINE_POOL(CopyTextureCommand);
DEFINE_POOL(ComputeMipMapsCommand);
DEFINE_POOL(SetScissorCommand);
DEFINE_POOL(SetBlendCommand);
DEFINE_POOL(SetCameraCommand);
DEFINE_POOL(PushCameraCommand);
DEFINE_POOL(PopCameraCommand);
DEFINE_POOL(SetClipPlanesCommand);
DEFINE_POOL(BindDescriptorSetsCommand);
DEFINE_POOL(SetTextureMipLevelsCommand);
DEFINE_POOL(BeginDebugScopeCommand);
DEFINE_POOL(EndDebugScopeCommand);
DEFINE_POOL(DrawTextCommand);
DEFINE_POOL(DrawIMGUICommand);
DEFINE_POOL(DispatchComputeCommand);
DEFINE_POOL(MemoryBarrierCommand);
DEFINE_POOL(ReadBufferDataCommand);
DEFINE_POOL(ClearBufferDataCommand);
DEFINE_POOL(SetClippingStateCommand);
DEFINE_POOL(ExternalCommand);

namespace {
    inline bool ShouldSkipType(const U8 typeIndex) noexcept {
        switch (static_cast<CommandType>(typeIndex)) {
            case GFX::CommandType::BEGIN_DEBUG_SCOPE:
            case GFX::CommandType::END_DEBUG_SCOPE:
                return true;
        }
        return false;
    }

    inline bool IsCameraCommand(const U8 typeIndex) noexcept {
        switch (static_cast<CommandType>(typeIndex)) {
            case GFX::CommandType::PUSH_CAMERA:
            case GFX::CommandType::POP_CAMERA:
            case GFX::CommandType::SET_CAMERA:
                return true;
        }
        return false;
    }

    inline bool DoesNotAffectRT(const U8 typeIndex) noexcept {
        if (ShouldSkipType(typeIndex) || IsCameraCommand(typeIndex)) {
            return true;
        }
        switch (static_cast<CommandType>(typeIndex)) {
            case GFX::CommandType::SET_VIEWPORT:
            case GFX::CommandType::SET_SCISSOR:
            case GFX::CommandType::SET_CAMERA:
            case GFX::CommandType::SET_CLIP_PLANES:
            case GFX::CommandType::SEND_PUSH_CONSTANTS:
            case GFX::CommandType::SET_CLIPING_STATE:
                return true;
        }
        return false;
    }

    const auto RemoveEmptyDrawCommands = [](DrawCommand::CommandContainer& commands) {
        const size_t startSize = commands.size();
        eastl::erase_if(commands, [](const GenericDrawCommand& cmd) noexcept -> bool {
            return cmd._drawCount == 0u;
        });
        return startSize != commands.size();
    };
};

void CommandBuffer::add(const CommandBuffer& other) {
    OPTICK_EVENT();

    static_assert(sizeof(PolyContainerEntry) == 4, "PolyContainerEntry has the wrong size!");
    _commands.reserveAdditional(other._commands);

    for (const CommandEntry& cmd : other._commandOrder) {
        other.get<CommandBase>(cmd)->addToBuffer(this);
    }
    _batched = false;
}

void CommandBuffer::add(CommandBuffer** buffers, size_t count) {
    OPTICK_EVENT();

    static_assert(sizeof(PolyContainerEntry) == 4, "PolyContainerEntry has the wrong size!");
    for (size_t i = 0; i < count; ++i) {
        CommandBuffer* other = buffers[i];

        _commands.reserveAdditional(other->_commands);
        for (const CommandEntry& cmd : other->_commandOrder) {
            other->get<CommandBase>(cmd)->addToBuffer(this);
        }

        _batched = false;
    }
}

void CommandBuffer::batch() {
    OPTICK_EVENT();

    if (_batched) {
        return;
    }

    clean();

    CommandBase* prevCommand = nullptr;
    CommandType prevType = CommandType::COUNT;

    const auto  EraseEmptyCommands = [this]() {
        const size_t initialSize = _commandOrder.size();
        eastl::erase_if(_commandOrder, ([](const CommandEntry& entry) noexcept { return entry._data == PolyContainerEntry::INVALID_ENTRY_ID; }));
        return initialSize != _commandOrder.size();
    };

    do {
        OPTICK_EVENT("TRY_MERGE_LOOP");

        bool tryMerge = true;
        // Try and merge ONLY descriptor sets as these don't care about commands between them (they only set global state
        while (tryMerge) {
            OPTICK_EVENT("TRY_MERGE_LOOP_STEP_1");

            tryMerge = false;
            prevCommand = nullptr;
            for (CommandEntry& entry : _commandOrder) {
                if (entry._data != PolyContainerEntry::INVALID_ENTRY_ID && !ShouldSkipType(entry._typeIndex)) {
                    const GFX::CommandType cmdType = static_cast<GFX::CommandType>(entry._typeIndex);
                    if (cmdType == GFX::CommandType::BIND_DESCRIPTOR_SETS) {
                        CommandBase* crtCommand = get<CommandBase>(entry);

                        if (prevCommand != nullptr && tryMergeCommands(cmdType, prevCommand, crtCommand)) {
                            --_commandCount[entry._typeIndex];
                            entry._data = PolyContainerEntry::INVALID_ENTRY_ID;
                            tryMerge = true;
                        } else {
                            prevCommand = crtCommand;
                        }
                    }
                }
            }
        }

        tryMerge = true;
        while (tryMerge) {
            OPTICK_EVENT("TRY_MERGE_LOOP_STEP_2");

            tryMerge = false;
            prevCommand = nullptr;
            prevType = CommandType::COUNT;
            for (CommandEntry& entry : _commandOrder) {
                if (entry._data != PolyContainerEntry::INVALID_ENTRY_ID && !ShouldSkipType(entry._typeIndex)) {
                    const GFX::CommandType cmdType = static_cast<GFX::CommandType>(entry._typeIndex);
                    CommandBase* crtCommand = get<CommandBase>(entry);

                    if (prevCommand != nullptr && prevType == cmdType && tryMergeCommands(cmdType, prevCommand, crtCommand)) {
                        --_commandCount[entry._typeIndex];
                        entry._data = PolyContainerEntry::INVALID_ENTRY_ID;
                        tryMerge = true;
                    } else {
                        prevType = cmdType;
                        prevCommand = crtCommand;
                    }
                }
            }
        }
    } while (EraseEmptyCommands());

    // Now try and merge ONLY End/Begin render pass (don't unbind a render target if we immediatelly bind a new one
    prevCommand = nullptr;
    for (const CommandEntry& entry : _commandOrder) {
        if (entry._data != PolyContainerEntry::INVALID_ENTRY_ID && !DoesNotAffectRT(entry._typeIndex)) {
            CommandBase* crtCommand = get<CommandBase>(entry);
            if (prevCommand != nullptr && entry._typeIndex == to_base(GFX::CommandType::BEGIN_RENDER_PASS)) {
                static_cast<EndRenderPassCommand*>(prevCommand)->_setDefaultRTState = false;
                prevCommand = nullptr;
            } else if (entry._typeIndex == to_base(GFX::CommandType::END_RENDER_PASS)) {
                prevCommand = crtCommand;
            } else {
                prevCommand = nullptr;
            }
        }
    }

    // If we don't have any actual work to do, clear everything
    bool hasWork = false;
    for (const CommandEntry& cmd : _commandOrder) {
        if (hasWork) {
            break;
        }

        switch (static_cast<GFX::CommandType>(cmd._typeIndex)) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                // We may just wish to clear some state
                if (get<BeginRenderPassCommand>(cmd)->_descriptor.setViewport()) {
                    hasWork = true;
                    break;
                }
            } break;
            case GFX::CommandType::CLEAR_RT:
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
            case GFX::CommandType::PUSH_CAMERA:
            case GFX::CommandType::POP_CAMERA:
            case GFX::CommandType::SET_CLIP_PLANES:
            case GFX::CommandType::SET_SCISSOR:
            case GFX::CommandType::SET_BLEND:
            case GFX::CommandType::SET_VIEWPORT:
            case GFX::CommandType::SET_CLIPING_STATE:
            case GFX::CommandType::SWITCH_WINDOW: {
                hasWork = true;
                break;
            }break;
            case GFX::CommandType::SET_MIP_LEVELS: {
                const SetTextureMipLevelsCommand* crtCmd = get<SetTextureMipLevelsCommand>(cmd);
                hasWork = crtCmd->_texture != nullptr && (crtCmd->_baseLevel != crtCmd->_texture->getBaseMipLevel() || crtCmd->_maxLevel != crtCmd->_texture->getMaxMipLevel());
            } break;
            case GFX::CommandType::COPY_TEXTURE: {
                const CopyTextureCommand* crtCmd = get<CopyTextureCommand>(cmd);
                hasWork = crtCmd->_source._textureType != TextureType::COUNT && crtCmd->_destination._textureType != TextureType::COUNT;
            }break;
        };
    }

    if (!hasWork) {
        _commandOrder.resize(0);
        std::memset(_commandCount.data(), 0, sizeof(U24) * to_base(GFX::CommandType::COUNT));
    }

    _batched = true;
}

void CommandBuffer::clean() {
    OPTICK_EVENT();

    if (_commandOrder.empty()) {
        return;
    }

    const Pipeline* prevPipeline = nullptr;
    const Rect<I32>* prevScissorRect = nullptr;
    const Rect<I32>* prevViewportRect = nullptr;
    const DescriptorSet* prevDescriptorSet = nullptr;
    const BlendingProperties* prevBlendProperties = nullptr;

    bool erase = false;
    for (CommandEntry& cmd :_commandOrder) {
        erase = false;

        switch (static_cast<GFX::CommandType>(cmd._typeIndex)) {
            case CommandType::DRAW_COMMANDS :
            {
                OPTICK_EVENT("Clean Draw Commands");

                DrawCommand::CommandContainer& cmds = get<DrawCommand>(cmd)->_drawCommands;
                if (cmds.size() == 1) {
                    erase = cmds.begin()->_drawCount == 0u;
                } else {
                    RemoveEmptyDrawCommands(cmds);
                    erase = cmds.empty();
                }
            } break;
            case CommandType::BIND_PIPELINE : {
                OPTICK_EVENT("Clean Pipelines");

                const Pipeline* pipeline = get<BindPipelineCommand>(cmd)->_pipeline;
                // If the current pipeline is identical to the previous one, remove it
                if (prevPipeline == nullptr || prevPipeline->getHash() != pipeline->getHash()) {
                    prevPipeline = pipeline;
                } else {
                    erase = true;
                }
            }break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                OPTICK_EVENT("Clean Push Constants");

                erase = get<SendPushConstantsCommand>(cmd)->_constants.empty();
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                OPTICK_EVENT("Clean Descriptor Sets");

                const DescriptorSet& set = get<BindDescriptorSetsCommand>(cmd)->_set;
                if (prevDescriptorSet == nullptr || set.empty() || *prevDescriptorSet != set) {
                    prevDescriptorSet = &set;
                } else {
                    erase = true;
                }
            }break;
            case GFX::CommandType::DRAW_TEXT: {
                OPTICK_EVENT("Clean Draw Text");

                const TextElementBatch& textBatch = get<DrawTextCommand>(cmd)->_batch;

                erase = true;
                for (const TextElement& element : textBatch()) {
                    if (!element.text().empty()) {
                        erase = false;
                        break;
                    }
                }
            }break;
            case GFX::CommandType::SET_SCISSOR: {
                OPTICK_EVENT("Clean Scissor");

                const Rect<I32>& scissorRect = get<SetScissorCommand>(cmd)->_rect;
                if (prevScissorRect == nullptr || *prevScissorRect != scissorRect) {
                    prevScissorRect = &scissorRect;
                } else {
                    erase = true;
                }
            } break;
            case GFX::CommandType::SET_VIEWPORT: {
                OPTICK_EVENT("Clean Viewport");

                const Rect<I32>& viewportRect = get<SetViewportCommand>(cmd)->_viewport;

                if (prevViewportRect == nullptr || *prevViewportRect != viewportRect) {
                    prevViewportRect = &viewportRect;
                } else {
                    erase = true;
                }
            } break;
            case GFX::CommandType::SET_BLEND: {
                OPTICK_EVENT("Clean Viewport");

                const BlendingProperties& blendProperties = get<SetBlendCommand>(cmd)->_blendProperties;
                if (prevBlendProperties == nullptr || *prevBlendProperties != blendProperties) {
                    prevBlendProperties = &blendProperties;
                } else {
                    erase = true;
                }
            } break;
            default: break;
        };


        if (erase) {
            --_commandCount[cmd._typeIndex];
            cmd._data = PolyContainerEntry::INVALID_ENTRY_ID;
        } 
    }

    {
        OPTICK_EVENT("Remove redundant Pipelines");
        // Remove redundant pipeline changes
        auto entry = eastl::next(eastl::begin(_commandOrder));
        for (; entry != eastl::cend(_commandOrder); ++entry) {
            const U8 typeIndex = entry->_typeIndex;

            if (static_cast<GFX::CommandType>(typeIndex) == CommandType::BIND_PIPELINE &&
                eastl::prev(entry)->_typeIndex == typeIndex)
            {
                --_commandCount[typeIndex];
                entry->_data = PolyContainerEntry::INVALID_ENTRY_ID;
            }
        }
    }
    {
        OPTICK_EVENT("Remove invalid draw commands");
        eastl::erase_if(_commandOrder, [](const CommandEntry& entry) noexcept { return entry._data == PolyContainerEntry::INVALID_ENTRY_ID; });
    }
}

// New use cases that emerge from production work should be checked here.
bool CommandBuffer::validate() const {
    OPTICK_EVENT();

    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
        bool pushedPass = false, pushedSubPass = false, pushedPixelBuffer = false;
        bool hasPipeline = false, hasDescriptorSets = false;
        I32 pushedDebugScope = 0, pushedCamera = 0;

        for (const CommandEntry& cmd : _commandOrder) {
            switch (static_cast<GFX::CommandType>(cmd._typeIndex)) {
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
                case GFX::CommandType::PUSH_CAMERA: {
                    ++pushedCamera;
                }break;
                case GFX::CommandType::POP_CAMERA: {
                    --pushedCamera;
                }break;
                case GFX::CommandType::BIND_PIPELINE: {
                    hasPipeline = true;
                } break;
                case GFX::CommandType::DISPATCH_COMPUTE: 
                case GFX::CommandType::DRAW_TEXT:
                case GFX::CommandType::DRAW_IMGUI:
                case GFX::CommandType::DRAW_COMMANDS: {
                    if (!hasPipeline) {
                        return false;
                    }
                }break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    hasDescriptorSets = true;
                }break;
                case GFX::CommandType::BLIT_RT: {
                    if (!hasDescriptorSets) {
                        return false;
                    }
                }break;
                default: {
                    // no requirements yet
                }break;
            };
        }

        return !pushedPass && !pushedSubPass && !pushedPixelBuffer && pushedDebugScope == 0 && pushedCamera == 0;
    }

    return true;
}

void CommandBuffer::toString(const GFX::CommandBase& cmd, GFX::CommandType type, I32& crtIndent, stringImpl& out) const {
    const auto append = [](stringImpl& target, const stringImpl& text, I32 indent) {
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
        toString(*get<CommandBase>(cmd), static_cast<GFX::CommandType>(cmd._typeIndex), crtIndent, out);
        out.append("\n");
    }
    out.append("\n\n\n\n");

    assert(crtIndent == 0);

    return out;
}



bool BatchDrawCommands(bool byBaseInstance, GenericDrawCommand& previousIDC, GenericDrawCommand& currentIDC) noexcept {
    // Instancing is not compatible with MDI. Well, it might be, but I can't be bothered a.t.m. to implement it -Ionut
    if (previousIDC._cmd.primCount != currentIDC._cmd.primCount && (previousIDC._cmd.primCount > 1 || currentIDC._cmd.primCount > 1)) {
        return false;
    }

    // Batchable commands must share the same buffer and other various state
    if (compatible(previousIDC, currentIDC)) {
        const U32 diff = byBaseInstance 
                            ? (currentIDC._cmd.baseInstance - previousIDC._cmd.baseInstance) 
                            : u32Diff(currentIDC._commandOffset, previousIDC._commandOffset);

        if (diff == previousIDC._drawCount) {
            // If the rendering commands are batchable, increase the draw count for the previous one
            previousIDC._drawCount += currentIDC._drawCount;
            // And set the current command's draw count to zero so it gets removed from the list later on
            currentIDC._drawCount = 0;
            return true;
        }
    }

    return false;
}

bool Merge(DrawCommand* prevCommand, DrawCommand* crtCommand) {
    OPTICK_EVENT();

    const auto BatchCommands = [](DrawCommand::CommandContainer& commands, bool byBaseInstance) {
        const size_t commandCount = commands.size();
        for (size_t previousCommandIndex = 0, currentCommandIndex = 1;
             currentCommandIndex < commandCount;
             ++currentCommandIndex)
        {
            if (!BatchDrawCommands(byBaseInstance, commands[previousCommandIndex], commands[currentCommandIndex])) {
                previousCommandIndex = currentCommandIndex;
            }
        }
    };

    DrawCommand::CommandContainer& commands = prevCommand->_drawCommands;
    commands.insert(eastl::cend(commands),
                    eastl::make_move_iterator(eastl::begin(crtCommand->_drawCommands)),
                    eastl::make_move_iterator(eastl::end(crtCommand->_drawCommands)));
    crtCommand->_drawCommands.resize(0);

    {
        OPTICK_EVENT("Merge by instance");
        eastl::sort(eastl::begin(commands),
                    eastl::end(commands),
                    [](const GenericDrawCommand& a, const GenericDrawCommand& b) noexcept -> bool {
                        return a._cmd.baseInstance < b._cmd.baseInstance;
                    });
        do {
            BatchCommands(commands, true);
        } while (RemoveEmptyDrawCommands(commands));
    }

    {
        OPTICK_EVENT("Merge by offset");
        eastl::sort(eastl::begin(commands),
                    eastl::end(commands),
                    [](const GenericDrawCommand& a, const GenericDrawCommand& b) noexcept -> bool {
                        return a._commandOffset < b._commandOffset;
                    });

        do {
            BatchCommands(commands, false);
        } while (RemoveEmptyDrawCommands(commands));
    }

    return true;
}

}; //namespace GFX
}; //namespace Divide