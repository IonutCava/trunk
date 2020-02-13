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
DEFINE_POOL(ResolveRenderTargetCommand);
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
    FORCE_INLINE bool ShouldSkipType(const U8 typeIndex) noexcept {
        switch (static_cast<CommandType>(typeIndex)) {
            case GFX::CommandType::BEGIN_DEBUG_SCOPE:
            case GFX::CommandType::END_DEBUG_SCOPE:
                return true;
        }
        return false;
    }
};

void CommandBuffer::add(const CommandBuffer& other) {
    OPTICK_EVENT();

    static_assert(sizeof(PolyContainerEntry) == 4, "PolyContainerEntry has the wrong size!");
    _commands.reserve(other._commands);

    for (const CommandEntry& cmd : other._commandOrder) {
        other.getPtr<CommandBase>(cmd)->addToBuffer(*this);
    }
    _batched = false;
}

void CommandBuffer::add(CommandBuffer** buffers, size_t count) {
    OPTICK_EVENT();

    for (size_t i = 0; i < count; ++i) {
        add(*buffers[i]);
    }
}

void CommandBuffer::addDestructive(CommandBuffer& other) {
    OPTICK_EVENT();

    add(other);
    other.clear(false);
}

void CommandBuffer::addDestructive(CommandBuffer** buffers, size_t count) {
    OPTICK_EVENT();

    add(buffers, count);
    for (size_t i = 0; i < count; ++i) {
        buffers[i]->clear(false);
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

    bool tryMerge = true;

    while (tryMerge) {
        OPTICK_EVENT("TRY_MERGE_LOOP_1");

        tryMerge = false;
        prevCommand = nullptr;
        prevType = CommandType::COUNT;

        for (CommandEntry& entry : _commandOrder)  {
            if (entry._data == PolyContainerEntry::INVALID_ENTRY_ID) {
                continue;
            }
            if (ShouldSkipType(entry._typeIndex)) {
                continue;
            }

            const U8 typeIndex = entry._typeIndex;
            const GFX::CommandType cmdType = static_cast<GFX::CommandType>(typeIndex);

            CommandBase* crtCommand = getPtr<CommandBase>(entry);

            if (prevCommand != nullptr && 
                prevType == cmdType &&
                tryMergeCommands(cmdType, prevCommand, crtCommand))
            {
                --_commandCount[typeIndex];
                entry._data = PolyContainerEntry::INVALID_ENTRY_ID;
                tryMerge = true;
            } else {
                prevType = cmdType;
                prevCommand = crtCommand;
            }
        }
    }

    // Now try and merge ONLY descriptor sets
    tryMerge = true;
    while (tryMerge) {
        OPTICK_EVENT("TRY_MERGE_LOOP_2");

        tryMerge = false;

        prevCommand = nullptr;
        for (CommandEntry& entry : _commandOrder) {
            if (entry._data == PolyContainerEntry::INVALID_ENTRY_ID) {
                continue;
            }
            if (ShouldSkipType(entry._typeIndex)) {
                continue;
            }

            CommandBase* crtCommand = getPtr<CommandBase>(entry);
            if (prevCommand != nullptr && 
                entry._typeIndex == to_base(GFX::CommandType::BIND_DESCRIPTOR_SETS) &&
                tryMergeCommands(GFX::CommandType::BIND_DESCRIPTOR_SETS, prevCommand, crtCommand))
            {
                --_commandCount[entry._typeIndex];
                entry._data = PolyContainerEntry::INVALID_ENTRY_ID;
                tryMerge = true;
            }
            else if (entry._typeIndex == to_base(GFX::CommandType::BIND_DESCRIPTOR_SETS))
            {
                prevCommand = crtCommand;
            }
        }
    }


    eastl::erase_if(_commandOrder, ([](const CommandEntry& entry) noexcept { return entry._data == PolyContainerEntry::INVALID_ENTRY_ID; }));

    // If we don't have any actual work to do, clear everything
    bool hasWork = false;
    for (const CommandEntry& cmd : _commandOrder) {
        if (hasWork) {
            break;
        }

        switch (static_cast<GFX::CommandType>(cmd._typeIndex)) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                // We may just wish to clear some state
                if (getPtr<BeginRenderPassCommand>(cmd)->_descriptor.setViewport()) {
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
            case GFX::CommandType::RESOLVE_RT: {
                const ResolveRenderTargetCommand* crtCmd = getPtr<ResolveRenderTargetCommand>(cmd);
                hasWork = crtCmd->_resolveColours || crtCmd->_resolveDepth;
            } break;
            case GFX::CommandType::SET_MIP_LEVELS: {
                const SetTextureMipLevelsCommand* crtCmd = getPtr<SetTextureMipLevelsCommand>(cmd);
                hasWork = crtCmd->_texture != nullptr && (crtCmd->_baseLevel != crtCmd->_texture->getBaseMipLevel() || crtCmd->_maxLevel != crtCmd->_texture->getMaxMipLevel());
            } break;
            case GFX::CommandType::COPY_TEXTURE: {
                const CopyTextureCommand* crtCmd = getPtr<CopyTextureCommand>(cmd);
                hasWork = crtCmd->_source.type() != TextureType::COUNT && crtCmd->_destination.type() != TextureType::COUNT;
            }break;
        };
    }

    if (!hasWork) {
        _commandOrder.resize(0);
        std::memset(_commandCount.data(), 0, sizeof(I24) * to_base(GFX::CommandType::COUNT));
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
        const U8 typeIndex = cmd._typeIndex;

        switch (static_cast<GFX::CommandType>(typeIndex)) {
            case CommandType::DRAW_COMMANDS :
            {
                OPTICK_EVENT("Clean Draw Commands");

                vectorEASTLFast<GenericDrawCommand>& cmds = getPtr<DrawCommand>(cmd)->_drawCommands;
                eastl::erase_if(cmds,
                                [](const GenericDrawCommand& cmd) noexcept -> bool {
                                    return cmd._drawCount == 0u;
                                });

                erase = cmds.empty();
            } break;
            case CommandType::BIND_PIPELINE : {
                OPTICK_EVENT("Clean Pipelines");

                const Pipeline* pipeline = getPtr<BindPipelineCommand>(cmd)->_pipeline;
                // If the current pipeline is identical to the previous one, remove it
                erase = (prevPipeline != nullptr && *prevPipeline == *pipeline);

                if (!erase) {
                    prevPipeline = pipeline;
                }
            }break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                OPTICK_EVENT("Clean Push Constants");

                const PushConstants& constants = getPtr<SendPushConstantsCommand>(cmd)->_constants;
                erase = constants.empty();
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                OPTICK_EVENT("Clean Descriptor Sets");

                const DescriptorSet& set = getPtr<BindDescriptorSetsCommand>(cmd)->_set;
                erase = (set.empty() || (prevDescriptorSet != nullptr && *prevDescriptorSet == set));

                if (!erase) {
                    prevDescriptorSet = &set;
                }
            }break;
            case GFX::CommandType::DRAW_TEXT: {
                OPTICK_EVENT("Clean Draw Text");

                const TextElementBatch& textBatch = getPtr<DrawTextCommand>(cmd)->_batch;
                bool hasText = !textBatch.empty();
                if (hasText) {
                    hasText = false;
                    for (const TextElement& element : textBatch()) {
                        hasText = hasText || !element.text().empty();
                    }
                }

                erase = !hasText;
            }break;
            case GFX::CommandType::SET_SCISSOR: {
                OPTICK_EVENT("Clean Scissor");

                const Rect<I32>* scissorRect = &getPtr<SetScissorCommand>(cmd)->_rect;
                erase = (prevScissorRect != nullptr && *prevScissorRect == *scissorRect);

                if (!erase) {
                    prevScissorRect = scissorRect;
                }
            } break;
            case GFX::CommandType::SET_VIEWPORT: {
                OPTICK_EVENT("Clean Viewport");

                const Rect<I32>* viewportRect = &getPtr<SetViewportCommand>(cmd)->_viewport;
                erase = (prevViewportRect != nullptr && *prevViewportRect == *viewportRect);

                if (!erase) {
                    prevViewportRect = viewportRect;
                }
            } break;
            case GFX::CommandType::SET_BLEND: {
                OPTICK_EVENT("Clean Viewport");

                const BlendingProperties* blendProperties = &getPtr<SetBlendCommand>(cmd)->_blendProperties;
                erase = (prevBlendProperties != nullptr && *prevBlendProperties == *blendProperties);

                if (!erase) {
                    prevBlendProperties = blendProperties;
                }
            } break;
            default: break;
        };


        if (erase) {
            --_commandCount[typeIndex];
            cmd._data = PolyContainerEntry::INVALID_ENTRY_ID;
        } 
    }

    {
        OPTICK_EVENT("Remove redundant Pipelines");
        // Remove redundant pipeline changes
        auto entry = eastl::next(eastl::begin(_commandOrder));
        for (; entry != eastl::cend(_commandOrder); ++entry) {
            const U8 typeIndex = entry->_typeIndex;
            const GFX::CommandType type = static_cast<GFX::CommandType>(typeIndex);

            if (type == CommandType::BIND_PIPELINE && eastl::prev(entry)->_typeIndex == typeIndex) {
                --_commandCount[typeIndex];
                entry->_data = -1;
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

    if (Config::ENABLE_GPU_VALIDATION) {
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
        toString(get<CommandBase>(cmd), static_cast<GFX::CommandType>(cmd._typeIndex), crtIndent, out);
        out.append("\n");
    }
    out.append("\n\n\n\n");

    assert(crtIndent == 0);

    return out;
}



bool BatchDrawCommands(bool byBaseInstance, GenericDrawCommand& previousIDC, GenericDrawCommand& currentIDC) {
    // Instancing is not compatible with MDI. Well, it might be, but I can't be bothered a.t.m. to implement it -Ionut
    if ((previousIDC._cmd.primCount > 1 || currentIDC._cmd.primCount > 1) && previousIDC._cmd.primCount != currentIDC._cmd.primCount) {
        return false;
    }

    // Batchable commands must share the same buffer and other various state
    if (compatible(previousIDC, currentIDC)) {
        if (byBaseInstance) { // Base instance compatibility
            if (previousIDC._cmd.baseInstance + previousIDC._drawCount != currentIDC._cmd.baseInstance) {
                return false;
            }
        } else {// Command offset compatibility
            if (previousIDC._commandOffset + to_I32(previousIDC._drawCount) != currentIDC._commandOffset) {
                return false;
            }
        }

        // If the rendering commands are batchable, increase the draw count for the previous one
        previousIDC._drawCount += currentIDC._drawCount;
        // And set the current command's draw count to zero so it gets removed from the list later on
        currentIDC._drawCount = 0;
        return true;
    }

    return false;
}

bool Merge(DrawCommand* prevCommand, DrawCommand* crtCommand) {
    OPTICK_EVENT();

    const auto BatchCommands = [](vectorEASTLFast<GenericDrawCommand>& commands, bool byBaseInstance) {
        vec_size previousCommandIndex = 0;
        vec_size currentCommandIndex = 1;
        const vec_size commandCount = commands.size();
        for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
            GenericDrawCommand& previousCommand = commands[previousCommandIndex];
            GenericDrawCommand& currentCommand = commands[currentCommandIndex];
            if (!BatchDrawCommands(byBaseInstance, previousCommand, currentCommand)) {
                previousCommandIndex = currentCommandIndex;
            }
        }
    };

    const auto RemoveEmptyDrawCommands = [](vectorEASTLFast<GenericDrawCommand>& commands) {
        const size_t startSize = commands.size();
        eastl::erase_if(commands,
                        [](const GenericDrawCommand& cmd) noexcept -> bool {
                            return cmd._drawCount == 0;
                        });
        return startSize - commands.size() > 0;
    };

    vectorEASTLFast<GenericDrawCommand>& commands = prevCommand->_drawCommands;
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