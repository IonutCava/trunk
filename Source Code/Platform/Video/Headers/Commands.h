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

#include <MemoryPool/StackAlloc.h>
#include <MemoryPool/C-11/MemoryPool.h>

struct ImDrawData;

namespace Divide {
namespace GFX {

enum class CommandType : U8 {
    BEGIN_RENDER_PASS = 0,
    END_RENDER_PASS,
    BEGIN_PIXEL_BUFFER,
    END_PIXEL_BUFFER,
    BEGIN_RENDER_SUB_PASS,
    END_RENDER_SUB_PASS,
    SET_VIEWPORT,
    SET_SCISSOR,
    SET_BLEND,
    BLIT_RT,
    SET_CAMERA,
    SET_CLIP_PLANES,
    BIND_PIPELINE,
    BIND_DESCRIPTOR_SETS,
    SEND_PUSH_CONSTANTS,
    DRAW_COMMANDS,
    DRAW_TEXT,
    DRAW_IMGUI,
    DISPATCH_COMPUTE,
    READ_ATOMIC_COUNTER,
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    SWITCH_WINDOW,
    EXTERNAL,
    COUNT
};

class CommandBuffer;

struct CommandBase
{
    CommandBase() : CommandBase(CommandType::COUNT, "null") {}
    CommandBase(CommandType type, const char* typeName) : _type(type), _typeName(typeName) {}

    virtual ~CommandBase() = default;

    virtual void addToBuffer(CommandBuffer& buffer) const = 0;

    virtual const char* toString() const { return _typeName; }

    CommandType _type = CommandType::COUNT;
    const char* _typeName = nullptr;
};

template<typename T, CommandType enumVal>
struct Command : public CommandBase {
    Command() : CommandBase(enumVal, TO_STRING(enumVal)) { }
    
    virtual ~Command() = default;

    virtual void addToBuffer(CommandBuffer& buffer) const override {
        buffer.add(reinterpret_cast<const T*>(this));
    }
};

#define DECLARE_POOL(Command, Size) \
static SharedLock s_PoolMutex; \
static MemoryPool<Command, Size> s_Pool;

#define DEFINE_POOL(Command, Size) \
SharedLock Command::s_PoolMutex; \
MemoryPool<Command, Size> Command::s_Pool;


#define BEGIN_COMMAND(Name, Enum, PoolSize) \
struct Name final : Command<Name, Enum> {\
DECLARE_POOL(Name, PoolSize)

#define END_COMMAND() }

BEGIN_COMMAND(BindPipelineCommand, CommandType::BIND_PIPELINE, 4096);
    const Pipeline* _pipeline = nullptr;
END_COMMAND();


BEGIN_COMMAND(SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS, 4096);
    PushConstants _constants;
END_COMMAND();


BEGIN_COMMAND(DrawCommand, CommandType::DRAW_COMMANDS, 4096);
    vectorEASTL<GenericDrawCommand> _drawCommands;
END_COMMAND();


BEGIN_COMMAND(SetViewportCommand, CommandType::SET_VIEWPORT, 4096);
    Rect<I32> _viewport;

    const char* toString() const override {
        return (_typeName + Util::StringFormat("[%d, %d, %d, %d]", _viewport.x, _viewport.y, _viewport.z, _viewport.w)).c_str();
    }
END_COMMAND();

BEGIN_COMMAND(BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS, 4096);
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    eastl::fixed_string<char, 128 + 1, true> _name = "";

    const char* toString() const override {
        return (stringImpl(_typeName) + _name.c_str()).c_str();
    }
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
    U16 _mipWriteLevel = 0u;
END_COMMAND();

BEGIN_COMMAND(EndRenderSubPassCommand, CommandType::END_RENDER_SUB_PASS, 4096);
END_COMMAND();

BEGIN_COMMAND(BlitRenderTargetCommand, CommandType::BLIT_RT, 4096);
    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
END_COMMAND();

BEGIN_COMMAND(SetScissorCommand, CommandType::SET_SCISSOR, 4096);
    Rect<I32> _rect;
END_COMMAND();

BEGIN_COMMAND(SetBlendCommand, CommandType::SET_BLEND, 4096);
    BlendingProperties _blendProperties;
END_COMMAND();

BEGIN_COMMAND(SetCameraCommand, CommandType::SET_CAMERA, 4096);
    Camera* _camera = nullptr;
END_COMMAND();

BEGIN_COMMAND(SetClipPlanesCommand, CommandType::SET_CLIP_PLANES, 4096);
    FrustumClipPlanes _clippingPlanes;
END_COMMAND();

BEGIN_COMMAND(BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS, 4096);
    DescriptorSet_ptr _set = nullptr;
END_COMMAND();

BEGIN_COMMAND(BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE, 4096);
    eastl::fixed_string<char, 128 + 1, true> _scopeName;
    I32 _scopeID = -1;

    const char* toString() const override {
        return (stringImpl(_typeName) + _scopeName.c_str()).c_str();
    }
END_COMMAND();

BEGIN_COMMAND(EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE, 4096);
END_COMMAND();

BEGIN_COMMAND(DrawTextCommand, CommandType::DRAW_TEXT, 4096);
    TextElementBatch _batch;
END_COMMAND();

BEGIN_COMMAND(DrawIMGUICommand, CommandType::DRAW_IMGUI, 4096);
    ImDrawData* _data = nullptr;
END_COMMAND();

BEGIN_COMMAND(DispatchComputeCommand, CommandType::DISPATCH_COMPUTE, 4096);
    ComputeParams _params;
END_COMMAND();

BEGIN_COMMAND(ReadAtomicCounterCommand, CommandType::READ_ATOMIC_COUNTER, 4096);
    ShaderBuffer* _buffer = nullptr;
    U32* _target = nullptr;
    bool _resetCounter = false;
END_COMMAND();

BEGIN_COMMAND(SwitchWindowCommand, CommandType::SWITCH_WINDOW, 4096);
    I64 windowGUID = -1;
END_COMMAND();

BEGIN_COMMAND(ExternalCommand, CommandType::EXTERNAL, 4096);
    std::function<void()> _cbk;
END_COMMAND();

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
