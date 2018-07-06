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


// beware of macros!
#define REGISTER_COMMAND(Type, PoolSize)
#define TYPE_HANDLER(T) const std::type_info& type() override { return typeid(T); }

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

    virtual const std::type_info& type() = 0;

    CommandType _type = CommandType::COUNT;
};

struct BindPipelineCommand final : Command {
    BindPipelineCommand() : Command(CommandType::BIND_PIPELINE)
    {
    }

    TYPE_HANDLER(BindPipelineCommand);

    const Pipeline* _pipeline = nullptr;
};
REGISTER_COMMAND(BindPipelineCommand, 1024);

struct SendPushConstantsCommand final : Command {
    SendPushConstantsCommand() : Command(CommandType::SEND_PUSH_CONSTANTS)
    {
    }

    TYPE_HANDLER(SendPushConstantsCommand);

    PushConstants _constants;
};
REGISTER_COMMAND(SendPushConstantsCommand, 1024);

struct DrawCommand final : Command {
    DrawCommand() : Command(CommandType::DRAW_COMMANDS)
    {
    }

    TYPE_HANDLER(DrawCommand);

    vectorEASTL<GenericDrawCommand> _drawCommands;
};
REGISTER_COMMAND(DrawCommand, 16384);

struct SetViewportCommand final : Command {
    SetViewportCommand() : Command(CommandType::SET_VIEWPORT)
    {
    }

    TYPE_HANDLER(SetViewportCommand);

    Rect<I32> _viewport;
};
REGISTER_COMMAND(SetViewportCommand, 256);

struct BeginRenderPassCommand final : Command {
    BeginRenderPassCommand() : Command(CommandType::BEGIN_RENDER_PASS)
    {
    }

    TYPE_HANDLER(BeginRenderPassCommand);

    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    stringImpl _name;
};
REGISTER_COMMAND(BeginRenderPassCommand, 256);

struct EndRenderPassCommand final : Command {
    EndRenderPassCommand() : Command(CommandType::END_RENDER_PASS)
    {
    }

    TYPE_HANDLER(BeginRenderPassCommand);

};
REGISTER_COMMAND(EndRenderPassCommand, 256);

struct BeginPixelBufferCommand final : Command {
    BeginPixelBufferCommand() : Command(CommandType::BEGIN_PIXEL_BUFFER)
    {
    }

    TYPE_HANDLER(EndRenderPassCommand);

    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
};
REGISTER_COMMAND(BeginPixelBufferCommand, 128);

struct EndPixelBufferCommand final : Command {
    EndPixelBufferCommand() : Command(CommandType::END_PIXEL_BUFFER)
    {
    }

    TYPE_HANDLER(BeginPixelBufferCommand);

};
REGISTER_COMMAND(EndPixelBufferCommand, 128);

struct BeginRenderSubPassCommand final : Command {
    BeginRenderSubPassCommand() : Command(CommandType::BEGIN_RENDER_SUB_PASS)
    {
    }

    TYPE_HANDLER(EndPixelBufferCommand);

    U16 _mipWriteLevel = 0u;
};
REGISTER_COMMAND(BeginRenderSubPassCommand, 512);

struct EndRenderSubPassCommand final : Command {
    EndRenderSubPassCommand() : Command(CommandType::END_RENDER_SUB_PASS)
    {
    }

    TYPE_HANDLER(BeginRenderSubPassCommand);

};
REGISTER_COMMAND(EndRenderSubPassCommand, 512);

struct BlitRenderTargetCommand final : Command {
    BlitRenderTargetCommand() : Command(CommandType::BLIT_RT)
    {
    }

    TYPE_HANDLER(EndRenderSubPassCommand);

    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
};
REGISTER_COMMAND(BlitRenderTargetCommand, 128);

struct SetScissorCommand final : Command {
    SetScissorCommand() : Command(CommandType::SET_SCISSOR)
    {
    }

    TYPE_HANDLER(BlitRenderTargetCommand);

    Rect<I32> _rect;
};
REGISTER_COMMAND(SetScissorCommand, 128);

struct SetBlendCommand final : Command {
    SetBlendCommand() : Command(CommandType::SET_BLEND)
    {
    }

    TYPE_HANDLER(SetScissorCommand);

    bool _enabled = true;
    BlendingProperties _blendProperties;
};
REGISTER_COMMAND(SetBlendCommand, 256);

struct SetCameraCommand final : Command {
    SetCameraCommand() : Command(CommandType::SET_CAMERA)
    {
    }

    TYPE_HANDLER(SetBlendCommand);

    Camera* _camera = nullptr;
};
REGISTER_COMMAND(SetCameraCommand, 512);

struct SetClipPlanesCommand final : Command {
    SetClipPlanesCommand() : Command(CommandType::SET_CLIP_PLANES),
        _clippingPlanes(Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f))
    {
    }

    TYPE_HANDLER(SetCameraCommand);

    FrustumClipPlanes _clippingPlanes;
};
REGISTER_COMMAND(SetClipPlanesCommand, 512);

struct BindDescriptorSetsCommand final : Command {
    BindDescriptorSetsCommand() : Command(CommandType::BIND_DESCRIPTOR_SETS)
    {
    }

    TYPE_HANDLER(BindDescriptorSetsCommand);

    DescriptorSet _set;
};
REGISTER_COMMAND(BindDescriptorSetsCommand, 1024);

struct BeginDebugScopeCommand final : Command {
    BeginDebugScopeCommand() : Command(CommandType::BEGIN_DEBUG_SCOPE)
    {
    }

    TYPE_HANDLER(BeginDebugScopeCommand);

    stringImpl _scopeName;
    I32 _scopeID = -1;
};
REGISTER_COMMAND(BeginDebugScopeCommand, 4096);

struct EndDebugScopeCommand final : Command {
    EndDebugScopeCommand() : Command(CommandType::END_DEBUG_SCOPE)
    {
    }

    TYPE_HANDLER(EndDebugScopeCommand);

};
REGISTER_COMMAND(EndDebugScopeCommand, 4096);

struct DrawTextCommand final : Command {
    DrawTextCommand() : Command(CommandType::DRAW_TEXT)
    {
    }

    TYPE_HANDLER(DrawTextCommand);

    TextElementBatch _batch;
};
REGISTER_COMMAND(DrawTextCommand, 1024);

struct DrawIMGUICommand final : Command {
    DrawIMGUICommand() : Command(CommandType::DRAW_IMGUI)
    {
    }

    TYPE_HANDLER(DrawIMGUICommand);

    ImDrawData* _data = nullptr;
};
REGISTER_COMMAND(DrawIMGUICommand, 64);

struct DispatchComputeCommand final : Command {
    DispatchComputeCommand() : Command(CommandType::DISPATCH_COMPUTE)
    {
    }

    TYPE_HANDLER(DispatchComputeCommand);

    ComputeParams _params;
};
REGISTER_COMMAND(DispatchComputeCommand, 128);

struct SwitchWindowCommand final : Command {
    SwitchWindowCommand() : Command(CommandType::SWITCH_WINDOW)
    {
    }

    TYPE_HANDLER(SwitchWindowCommand);

    I64 windowGUID = -1;
};
REGISTER_COMMAND(SwitchWindowCommand, 64);

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
