/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _GFX_COMMAND_H_
#define _GFX_COMMAND_H_

#include "GenericDrawCommand.h"
#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Rendering/Camera/Headers/CameraSnapshot.h"

#include <MemoryPool/StackAlloc.h>
#include <MemoryPool/C-11/MemoryPool.h>
#include <BetterEnums/include/enum.h>

struct ImDrawData;

namespace Divide {
namespace GFX {

BETTER_ENUM(CommandType, U8,
    BEGIN_RENDER_PASS,
    END_RENDER_PASS,
    BEGIN_PIXEL_BUFFER,
    END_PIXEL_BUFFER,
    BEGIN_RENDER_SUB_PASS,
    END_RENDER_SUB_PASS,
    SET_VIEWPORT,
    SET_SCISSOR,
    SET_BLEND,
    BLIT_RT,
    RESET_RT,
    COMPUTE_MIPMAPS,
    SET_CAMERA,
    SET_CLIP_PLANES,
    BIND_PIPELINE,
    BIND_DESCRIPTOR_SETS,
    SEND_PUSH_CONSTANTS,
    DRAW_COMMANDS,
    DRAW_TEXT,
    DRAW_IMGUI,
    DISPATCH_COMPUTE,
    MEMORY_BARRIER,
    READ_ATOMIC_COUNTER,
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    SWITCH_WINDOW,
    EXTERNAL,
    COUNT
);

class CommandBuffer;

struct CommandBase
{
    CommandBase() noexcept 
        : CommandBase(CommandType::COUNT)
    {
    }

    CommandBase(CommandType type) noexcept
        : _type(type)
    {
    }

    virtual ~CommandBase() = default;

    virtual void addToBuffer(CommandBuffer& buffer) const = 0;

    virtual stringImpl toString(U16 indent) const {
        ACKNOWLEDGE_UNUSED(indent);

        return stringImpl(_type._to_string());
    }

    CommandType _type = CommandType::COUNT;
};

template<typename T, CommandType::_enumerated EnumVal>
struct Command : public CommandBase {
    Command()
        : CommandBase(EnumVal)
    {
    }
    
    void addToBuffer(CommandBuffer& buffer) const override {
        buffer.add(reinterpret_cast<const T&>(*this));
    }

    virtual ~Command() = default;
};

#define DECLARE_POOL(Command, Size) \
static std::mutex s_PoolMutex; \
static MemoryPool<Command, Size> s_Pool;

#define DEFINE_POOL(Command, Size) \
std::mutex Command::s_PoolMutex; \
MemoryPool<Command, Size> Command::s_Pool;


#define BEGIN_COMMAND(Name, Enum, PoolSize) \
struct Name final : Command<Name, Enum> {\
DECLARE_POOL(Name, PoolSize)

#define END_COMMAND() }

BEGIN_COMMAND(BindPipelineCommand, CommandType::BIND_PIPELINE, 4096);
    const Pipeline* _pipeline = nullptr;

    stringImpl toString(U16 indent) const override;
END_COMMAND();


BEGIN_COMMAND(SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS, 4096);
    PushConstants _constants;

    stringImpl toString(U16 indent) const override;
END_COMMAND();


BEGIN_COMMAND(DrawCommand, CommandType::DRAW_COMMANDS, 4096);
    vectorEASTL<GenericDrawCommand> _drawCommands;

    stringImpl toString(U16 indent) const override;
END_COMMAND();


BEGIN_COMMAND(SetViewportCommand, CommandType::SET_VIEWPORT, 4096);
    Rect<I32> _viewport;

    stringImpl toString(U16 indent) const override;
END_COMMAND();

BEGIN_COMMAND(BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS, 4096);
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    eastl::fixed_string<char, 128 + 1, true> _name = "";

    stringImpl toString(U16 indent) const override;
 END_COMMAND();

BEGIN_COMMAND(EndRenderPassCommand, CommandType::END_RENDER_PASS, 4096);
END_COMMAND();

BEGIN_COMMAND(BeginPixelBufferCommand, CommandType::BEGIN_PIXEL_BUFFER, 4096);
    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
END_COMMAND();

BEGIN_COMMAND(EndPixelBufferCommand, CommandType::END_PIXEL_BUFFER, 4096);
END_COMMAND();

BEGIN_COMMAND(BeginRenderSubPassCommand, CommandType::BEGIN_RENDER_SUB_PASS, 4096);
    bool _validateWriteLevel = false;
    U16 _mipWriteLevel = std::numeric_limits<U16>::max();
    vectorEASTL<RenderTarget::DrawLayerParams> _writeLayers;
END_COMMAND();

BEGIN_COMMAND(EndRenderSubPassCommand, CommandType::END_RENDER_SUB_PASS, 4096);
END_COMMAND();

BEGIN_COMMAND(BlitRenderTargetCommand, CommandType::BLIT_RT, 4096);
    // List of depth layers to blit
    vectorEASTL<DepthBlitEntry> _blitDepth;
    // List of colours + colour layer to blit
    vectorEASTL<ColourBlitEntry> _blitColours;
    RenderTargetID _source;
    RenderTargetID _destination;
END_COMMAND();

BEGIN_COMMAND(ResetRenderTargetCommand, CommandType::RESET_RT, 4096);
    RenderTargetID _source;
    RTDrawDescriptor _descriptor;
END_COMMAND();

BEGIN_COMMAND(ComputeMipMapsCommand, CommandType::COMPUTE_MIPMAPS, 2048);
    Texture* _texture;
    vec2<U32> _layerRange = { 0, 1 };
END_COMMAND();

BEGIN_COMMAND(SetScissorCommand, CommandType::SET_SCISSOR, 4096);
    Rect<I32> _rect;

    stringImpl toString(U16 indent) const override;
END_COMMAND();

BEGIN_COMMAND(SetBlendCommand, CommandType::SET_BLEND, 4096);
    BlendingProperties _blendProperties;
END_COMMAND();

BEGIN_COMMAND(SetCameraCommand, CommandType::SET_CAMERA, 4096);
    CameraSnapshot _cameraSnapshot;
END_COMMAND();

BEGIN_COMMAND(SetClipPlanesCommand, CommandType::SET_CLIP_PLANES, 4096);
    FrustumClipPlanes _clippingPlanes;

    stringImpl toString(U16 indent) const override;
END_COMMAND();

BEGIN_COMMAND(BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS, 4096);
    DescriptorSet _set;

    stringImpl toString(U16 indent) const override;

END_COMMAND();

BEGIN_COMMAND(BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE, 4096);
    eastl::fixed_string<char, 128 + 1, true> _scopeName;
    I32 _scopeID = -1;

    stringImpl toString(U16 indent) const override;
END_COMMAND();

BEGIN_COMMAND(EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE, 4096);
END_COMMAND();

BEGIN_COMMAND(DrawTextCommand, CommandType::DRAW_TEXT, 4096);
    TextElementBatch _batch;

    stringImpl toString(U16 indent) const override;

END_COMMAND();

BEGIN_COMMAND(DrawIMGUICommand, CommandType::DRAW_IMGUI, 4096);
    ImDrawData* _data = nullptr;
    I64 _windowGUID = 0;
END_COMMAND();

BEGIN_COMMAND(DispatchComputeCommand, CommandType::DISPATCH_COMPUTE, 1024);
    vec3<U32> _computeGroupSize;

    stringImpl toString(U16 indent) const override;
END_COMMAND();

BEGIN_COMMAND(MemoryBarrierCommand, CommandType::MEMORY_BARRIER, 1024);
    U32 _barrierMask = 0;

    stringImpl toString(U16 indent) const override;

END_COMMAND();

BEGIN_COMMAND(ReadAtomicCounterCommand, CommandType::READ_ATOMIC_COUNTER, 1024);
    ShaderBuffer* _buffer = nullptr;
    U32* _target = nullptr;
    U8   _offset = 0;
    bool _resetCounter = false;
END_COMMAND();

BEGIN_COMMAND(ExternalCommand, CommandType::EXTERNAL, 4096);
    std::function<void()> _cbk;
END_COMMAND();

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
