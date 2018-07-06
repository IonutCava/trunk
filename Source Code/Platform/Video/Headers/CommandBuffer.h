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

#include "Commands.h"

namespace Divide {

namespace GFX {

struct CommandEntry {
    U32 _idx = 0;
    CommandType _type = CommandType::COUNT;
};

class CommandBuffer {
    friend class CommandBufferPool;

  public:
    CommandBuffer(size_t index = 0);
    ~CommandBuffer();

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
    size_t _index = 0;

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
    vectorImplFast<BeginDebugScopeCommand> _beginDebugScopeCommands;
    vectorImplFast<EndDebugScopeCommand> _endDebugScopeCommands;
    vectorImplFast<DrawTextCommand> _drawTextCommands;
    vectorImplFast<DispatchComputeCommand> _dispatchComputeCommands;
};

void BeginRenderPass(CommandBuffer& buffer, const BeginRenderPassCommand& cmd);
void EndRenderPass(CommandBuffer& buffer, const EndRenderPassCommand& cmd);
void BeginPixelBuffer(CommandBuffer& buffer, const BeginPixelBufferCommand& cmd);
void EndPixelBuffer(CommandBuffer& buffer, const EndPixelBufferCommand& cmd);
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
void BeginDebugScope(CommandBuffer& buffer, const BeginDebugScopeCommand& cmd);
void EndDebugScope(CommandBuffer& buffer, const EndDebugScopeCommand& cmd);
void AddDrawCommands(CommandBuffer& buffer, const DrawCommand& cmd);
void AddDrawTextCommand(CommandBuffer& buffer, const DrawTextCommand& cmd);
void AddComputeCommand(CommandBuffer& buffer, const DispatchComputeCommand& cmd);

}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_

#include "CommandBuffer.inl"