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

#include <MemoryPool/C-11/MemoryPool.h>

struct ImDrawData;

namespace CEGUI {
    class GUIContext;
    class TextureTarget;
};

namespace Divide {
namespace GFX {


#define REGISTER_COMMAND(Type, PoolSize) \
    bool g_isRegistered_##Type = true; \
    /*MemoryPool<Type, PoolSize> ##Type_pool;*/ \
    //struct Type


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
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    SWITCH_WINDOW,
    COUNT
};

struct Command {
    explicit Command(CommandType type)
        : _type(type)
    {
    }
    virtual ~Command() = default;

    CommandType _type = CommandType::COUNT;

protected:
    friend class CommandBuffer;
    virtual void onAdd(CommandBuffer* buffer) { ACKNOWLEDGE_UNUSED(buffer); }
};

struct BindPipelineCommand : Command {
    BindPipelineCommand() : Command(CommandType::BIND_PIPELINE)
    {
    }

    const Pipeline* _pipeline = nullptr;
};
REGISTER_COMMAND(BindPipelineCommand, 1024);

struct SendPushConstantsCommand : Command {
    SendPushConstantsCommand() : Command(CommandType::SEND_PUSH_CONSTANTS)
    {
    }

    PushConstants _constants;
};
REGISTER_COMMAND(SendPushConstantsCommand, 1024);

struct DrawCommand : Command {
    DrawCommand() : Command(CommandType::DRAW_COMMANDS)
    {
    }

    vectorEASTL<GenericDrawCommand> _drawCommands;
};
REGISTER_COMMAND(DrawCommand, 16384);

struct SetViewportCommand : Command {
    SetViewportCommand() : Command(CommandType::SET_VIEWPORT)
    {
    }

    Rect<I32> _viewport;
};
REGISTER_COMMAND(SetViewportCommand, 256);

struct BeginRenderPassCommand : Command {
    BeginRenderPassCommand() : Command(CommandType::BEGIN_RENDER_PASS)
    {
    }

    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    stringImpl _name;
};
REGISTER_COMMAND(BeginRenderPassCommand, 256);

struct EndRenderPassCommand : Command {
    EndRenderPassCommand() : Command(CommandType::END_RENDER_PASS)
    {
    }
};
REGISTER_COMMAND(EndRenderPassCommand, 256);

struct BeginPixelBufferCommand : Command {
    BeginPixelBufferCommand() : Command(CommandType::BEGIN_PIXEL_BUFFER)
    {
    }

    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
};
REGISTER_COMMAND(BeginPixelBufferCommand, 128);

struct EndPixelBufferCommand : Command {
    EndPixelBufferCommand() : Command(CommandType::END_PIXEL_BUFFER)
    {
    }
};
REGISTER_COMMAND(EndPixelBufferCommand, 128);

struct BeginRenderSubPassCommand : Command {
    BeginRenderSubPassCommand() : Command(CommandType::BEGIN_RENDER_SUB_PASS)
    {
    }

    U16 _mipWriteLevel = 0u;
};
REGISTER_COMMAND(BeginRenderSubPassCommand, 512);

struct EndRenderSubPassCommand : Command {
    EndRenderSubPassCommand() : Command(CommandType::END_RENDER_SUB_PASS)
    {
    }
};
REGISTER_COMMAND(EndRenderSubPassCommand, 512);

struct BlitRenderTargetCommand : Command {
    BlitRenderTargetCommand() : Command(CommandType::BLIT_RT)
    {
    }

    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
};
REGISTER_COMMAND(BlitRenderTargetCommand, 128);

struct SetScissorCommand : Command {
    SetScissorCommand() : Command(CommandType::SET_SCISSOR)
    {
    }

    Rect<I32> _rect;
};
REGISTER_COMMAND(SetScissorCommand, 128);

struct SetBlendCommand : Command {
    SetBlendCommand() : Command(CommandType::SET_BLEND)
    {

    }

    bool _enabled = true;
    BlendingProperties _blendProperties;
};
REGISTER_COMMAND(SetBlendCommand, 256);

struct SetCameraCommand : Command {
    SetCameraCommand() : Command(CommandType::SET_CAMERA)
    {
    }

    Camera* _camera = nullptr;
};
REGISTER_COMMAND(SetCameraCommand, 512);

struct SetClipPlanesCommand : Command {
    SetClipPlanesCommand() : Command(CommandType::SET_CLIP_PLANES),
        _clippingPlanes(Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f))
    {
    }

    FrustumClipPlanes _clippingPlanes;
};
REGISTER_COMMAND(SetClipPlanesCommand, 512);

struct BindDescriptorSetsCommand : Command {
    BindDescriptorSetsCommand() : Command(CommandType::BIND_DESCRIPTOR_SETS)
    {
    }

    DescriptorSet _set;
};
REGISTER_COMMAND(BindDescriptorSetsCommand, 1024);

struct BeginDebugScopeCommand : Command {
    BeginDebugScopeCommand() : Command(CommandType::BEGIN_DEBUG_SCOPE)
    {
    }

    stringImpl _scopeName;
    I32 _scopeID = -1;
};
REGISTER_COMMAND(BeginDebugScopeCommand, 4096);

struct EndDebugScopeCommand : Command {
    EndDebugScopeCommand() : Command(CommandType::END_DEBUG_SCOPE)
    {
    }
};
REGISTER_COMMAND(EndDebugScopeCommand, 4096);

struct DrawTextCommand : Command {
    DrawTextCommand() : Command(CommandType::DRAW_TEXT)
    {
    }

    TextElementBatch _batch;
};
REGISTER_COMMAND(DrawTextCommand, 1024);

struct DrawIMGUICommand : Command {
    DrawIMGUICommand() : Command(CommandType::DRAW_IMGUI)
    {
    }

    ImDrawData* _data = nullptr;
};
REGISTER_COMMAND(DrawIMGUICommand, 64);

struct DispatchComputeCommand : Command {
    DispatchComputeCommand() : Command(CommandType::DISPATCH_COMPUTE)
    {
    }

    ComputeParams _params;
};
REGISTER_COMMAND(DispatchComputeCommand, 128);

struct SwitchWindowCommand : Command {
    SwitchWindowCommand() : Command(CommandType::SWITCH_WINDOW)
    {
    }

    I64 windowGUID = -1;
};
REGISTER_COMMAND(SwitchWindowCommand, 64);

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
