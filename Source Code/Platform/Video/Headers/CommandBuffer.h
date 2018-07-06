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

#ifndef _COMMAND_BUFFER_H_
#define _COMMAND_BUFFER_H_

#include "GenericDrawCommand.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

namespace GFX {

enum class CommandType : U8 {
    BEGIN_RENDER_PASS = 0,
    END_RENDER_PASS,
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
    COUNT
};

struct Command {
    explicit Command(CommandType type)
        : _type(type)
    {
    }

    virtual void execute() = 0;

    CommandType _type = CommandType::COUNT;
};

struct BindPipelineCommand : Command {
    BindPipelineCommand() : Command(CommandType::BIND_PIPELINE)
    {
    }

    void execute() override {};

    Pipeline _pipeline;
};

struct SendPushConstantsCommand : Command {
    SendPushConstantsCommand() : Command(CommandType::SEND_PUSH_CONSTANTS)
    {
    }

    void execute() override {};

    PushConstants _constants;
};

struct DrawCommand : Command {
    DrawCommand() : Command(CommandType::DRAW_COMMANDS)
    {
    }

    void execute() override {};

    vectorImpl<GenericDrawCommand> _drawCommands;
};

struct SetViewportCommand : Command {
    SetViewportCommand() : Command(CommandType::SET_VIEWPORT)
    {
    }

    void execute() override {};

    vec4<I32> _viewport;
};

struct BeginRenderPassCommand : Command {
    BeginRenderPassCommand() : Command(CommandType::BEGIN_RENDER_PASS)
    {
    }

    void execute() override {};

    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
};

struct EndRenderPassCommand : Command {
    EndRenderPassCommand() : Command(CommandType::END_RENDER_PASS)
    {
    }

    void execute() override {};
};


struct BeginRenderSubPassCommand : Command {
    BeginRenderSubPassCommand() : Command(CommandType::BEGIN_RENDER_SUB_PASS)
    {
    }

    void execute() override {};

    U16 _mipWriteLevel = 0u;
};

struct EndRenderSubPassCommand : Command {
    EndRenderSubPassCommand() : Command(CommandType::END_RENDER_SUB_PASS)
    {
    }

    void execute() override {};
};

struct BlitRenderTargetCommand : Command {
    BlitRenderTargetCommand() : Command(CommandType::BLIT_RT)
    {
    }

    void execute() override {};

    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
};

struct SetScissorCommand : Command {
    SetScissorCommand() : Command(CommandType::SET_SCISSOR)
    {
    }

    void execute() override {};

    vec4<I32> _rect;
};

struct SetCameraCommand : Command {
    SetCameraCommand() : Command(CommandType::SET_CAMERA)
    {
    }

    void execute() override {};

    Camera* _camera = nullptr;
};

struct SetClipPlanesCommand : Command {
    SetClipPlanesCommand()
        : Command(CommandType::SET_CLIP_PLANES),
          _clippingPlanes(to_base(ClipPlaneIndex::COUNT), Plane<F32>(0.0f, 0.0f, 0.0f, 0.0f))
    {
    }

    void execute() override {};

    ClipPlaneList _clippingPlanes;
};


struct BindDescriptorSetsCommand : Command {
    BindDescriptorSetsCommand() : Command(CommandType::BIND_DESCRIPTOR_SETS)
    {
    }

    void execute() override {};

    DescriptorSet _set;
};

struct DrawTextCommand : Command {
    DrawTextCommand() : Command(CommandType::DRAW_TEXT)
    {
    }

    void execute() override {};

    TextElementBatch _batch;
};

struct DispatchComputeCommand : Command {
    DispatchComputeCommand() : Command(CommandType::DISPATCH_COMPUTE)
    {
    }

    void execute() override {};

    ComputeParams _params;
};

struct CommandEntry {
    U32 _idx = 0;
    CommandType _type = CommandType::COUNT;
};

class CommandBuffer {
   public:
    CommandBuffer();

    template<typename T>
    inline void add(const T& command);

    inline void add(const CommandBuffer& other);

    void clean();

    inline void batch();

    inline vectorImpl<std::shared_ptr<Command>>& operator()();
    inline const vectorImpl<std::shared_ptr<Command>>& operator()() const;

    inline const vectorImpl<Pipeline*>& getPipelines() const;
    inline const vectorImpl<ClipPlaneList*>& getClipPlanes() const;
    inline const vectorImpl<PushConstants*>& getPushConstants() const;
    inline const vectorImpl<DescriptorSet*>& getDescriptorSets() const;
    inline const vectorImpl<GenericDrawCommand*>& getDrawCommands() const;

    inline vectorAlg::vecSize size() const { return _data.size(); }
    inline void clear();
    inline bool empty() const;

    protected:
    void rebuildCaches();

    protected:
    vectorImpl<std::shared_ptr<Command>> _data;

    vectorImpl<Pipeline*> _pipelineCache;
    vectorImpl<ClipPlaneList*> _clipPlanesCache;
    vectorImpl<PushConstants*> _pushConstantsCache;
    vectorImpl<DescriptorSet*> _descriptorSetCache;
    vectorImpl<GenericDrawCommand*> _drawCommandsCache;

    vectorImplFast<CommandEntry> _buffer;
    vectorImplFast<BindPipelineCommand> _bindPipelineCommands;
    vectorImplFast<SendPushConstantsCommand> _sendPushConstantsCommands;
    vectorImplFast<DrawCommand> _drawCommands;
    vectorImplFast<SetViewportCommand> _setViewportCommands;
    vectorImplFast<SetCameraCommand> _setCameraCommands;
    vectorImplFast<SetClipPlanesCommand> _setClipPlanesCommand;
    vectorImplFast<BeginRenderPassCommand> _beginRenderPassCommands;
    vectorImplFast<EndRenderPassCommand> _endRenderPassCommands;
    vectorImplFast<BeginRenderSubPassCommand> _beginRenderSubPassCommands;
    vectorImplFast<EndRenderSubPassCommand> _endRenderSubPassCommands;
    vectorImplFast<BlitRenderTargetCommand> _blitRenderTargetCommands;
    vectorImplFast<SetScissorCommand> _setScissorCommands;
    vectorImplFast<BindDescriptorSetsCommand> _bindDescriptorSetsCommands;
    vectorImplFast<DrawTextCommand> _drawTextCommands;
    vectorImplFast<DispatchComputeCommand> _dispatchComputeCommands;
};

void BeginRenderPass(CommandBuffer& buffer, const BeginRenderPassCommand& cmd);
void EndRenderPass(CommandBuffer& buffer, const EndRenderPassCommand& cmd);
void BeginRenderSubPass(CommandBuffer& buffer, const BeginRenderSubPassCommand& cmd);
void EndRenderSubPass(CommandBuffer& buffer, const EndRenderSubPassCommand& cmd);
void BlitRenderTarget(CommandBuffer& buffer, const BlitRenderTargetCommand& cmd);
void SetViewPort(CommandBuffer& buffer, const SetViewportCommand& cmd);
void SetScissor(CommandBuffer& buffer, const SetScissorCommand& cmd);
void SetCamera(CommandBuffer& buffer, const SetCameraCommand& cmd);
void SetClipPlanes(CommandBuffer& buffer, const SetClipPlanesCommand& cmd);
void BindPipeline(CommandBuffer& buffer, const BindPipelineCommand& cmd);
void SendPushConstants(CommandBuffer& buffer, const SendPushConstantsCommand& cmd);
void BindDescriptorSets(CommandBuffer& buffer, const BindDescriptorSetsCommand& cmd);
void AddDrawCommands(CommandBuffer& buffer, const DrawCommand& cmd);
void AddDrawTextCommand(CommandBuffer& buffer, const DrawTextCommand& cmd);
void AddComputeCommand(CommandBuffer& buffer, const DispatchComputeCommand& cmd);
}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_

#include "CommandBuffer.inl"