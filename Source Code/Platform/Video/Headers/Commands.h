/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _GFX_COMMAND_H_
#define _GFX_COMMAND_H_

#include "GenericDrawCommand.h"
#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

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
    BLIT_RT,
    SET_CAMERA,
    SET_CLIP_PLANES,
    BIND_PIPELINE,
    BIND_DESCRIPTOR_SETS,
    SEND_PUSH_CONSTANTS,
    DRAW_COMMANDS,
    DRAW_TEXT,
    DISPATCH_COMPUTE,
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    COUNT
};

struct Command {
    explicit Command(CommandType type)
        : _type(type)
    {
    }

    CommandType _type = CommandType::COUNT;
};

struct BindPipelineCommand : Command {
    BindPipelineCommand() : Command(CommandType::BIND_PIPELINE)
    {
    }

    Pipeline _pipeline;
};

struct SendPushConstantsCommand : Command {
    SendPushConstantsCommand() : Command(CommandType::SEND_PUSH_CONSTANTS)
    {
    }

    PushConstants _constants;
};

struct DrawCommand : Command {
    DrawCommand() : Command(CommandType::DRAW_COMMANDS)
    {
    }

    vectorImpl<GenericDrawCommand> _drawCommands;
};

struct SetViewportCommand : Command {
    SetViewportCommand() : Command(CommandType::SET_VIEWPORT)
    {
    }

    vec4<I32> _viewport;
};

struct BeginRenderPassCommand : Command {
    BeginRenderPassCommand() : Command(CommandType::BEGIN_RENDER_PASS)
    {
    }

    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    stringImpl _name;
};

struct EndRenderPassCommand : Command {
    EndRenderPassCommand() : Command(CommandType::END_RENDER_PASS)
    {
    }
};

struct BeginPixelBufferCommand : Command {
    BeginPixelBufferCommand() : Command(CommandType::BEGIN_PIXEL_BUFFER)
    {
    }

    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
};

struct EndPixelBufferCommand : Command {
    EndPixelBufferCommand() : Command(CommandType::END_PIXEL_BUFFER)
    {
    }
};

struct BeginRenderSubPassCommand : Command {
    BeginRenderSubPassCommand() : Command(CommandType::BEGIN_RENDER_SUB_PASS)
    {
    }

    U16 _mipWriteLevel = 0u;
};

struct EndRenderSubPassCommand : Command {
    EndRenderSubPassCommand() : Command(CommandType::END_RENDER_SUB_PASS)
    {
    }
};

struct BlitRenderTargetCommand : Command {
    BlitRenderTargetCommand() : Command(CommandType::BLIT_RT)
    {
    }

    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
};

struct SetScissorCommand : Command {
    SetScissorCommand() : Command(CommandType::SET_SCISSOR)
    {
    }

    vec4<I32> _rect;
};

struct SetCameraCommand : Command {
    SetCameraCommand() : Command(CommandType::SET_CAMERA)
    {
    }

    Camera* _camera = nullptr;
};

struct SetClipPlanesCommand : Command {
    SetClipPlanesCommand()
        : Command(CommandType::SET_CLIP_PLANES),
        _clippingPlanes(to_base(ClipPlaneIndex::COUNT), Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f))
    {
    }

    ClipPlaneList _clippingPlanes;
};


struct BindDescriptorSetsCommand : Command {
    BindDescriptorSetsCommand() : Command(CommandType::BIND_DESCRIPTOR_SETS)
    {
    }

    DescriptorSet _set;
};

struct BeginDebugScopeCommand : Command {
    BeginDebugScopeCommand() : Command(CommandType::BEGIN_DEBUG_SCOPE)
    {
    }

    stringImpl _scopeName;
    I32 _scopeID = -1;
};


struct EndDebugScopeCommand : Command {
    EndDebugScopeCommand() : Command(CommandType::END_DEBUG_SCOPE)
    {
    }
};

struct DrawTextCommand : Command {
    DrawTextCommand() : Command(CommandType::DRAW_TEXT)
    {
    }

    TextElementBatch _batch;
};

struct DispatchComputeCommand : Command {
    DispatchComputeCommand() : Command(CommandType::DISPATCH_COMPUTE)
    {
    }

    ComputeParams _params;
};

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
