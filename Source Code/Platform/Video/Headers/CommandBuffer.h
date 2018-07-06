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
#ifndef _COMMAND_BUFFER_H_
#define _COMMAND_BUFFER_H_

#include "Commands.h"
#include <boost/poly_collection/base_collection.hpp>

namespace Divide {

namespace GFX {

class CommandBuffer : protected NonCopyable {
    friend class CommandBufferPool;

  public:
    typedef std::pair<GFX::CommandType, vec_size> CommandEntry;

  public:
    CommandBuffer();
    ~CommandBuffer();

    template<typename T>
    inline typename std::enable_if<std::is_base_of<Command, T>::value, void>::type
    add(const T& command);

    bool validate() const;

    void add(const CommandBuffer& other);

    void clean();

    void batch();

    // Return true if merge is successful
    bool tryMergeCommands(GFX::Command* prevCommand, GFX::Command* crtCommand) const;

    const GFX::Command& getCommand(const CommandEntry& commandEntry) const;

    inline vectorEASTL<CommandEntry>& operator()();
    inline const vectorEASTL<CommandEntry>& operator()() const;

    inline vec_size size() const { return _commandOrder.size(); }
    inline void clear();
    inline bool empty() const;

    // Multi-line. indented list of all commands (and params for some of them)
    stringImpl toString() const;

  protected:
    void toString(const GFX::Command& cmd, I32& crtIndent, stringImpl& out) const;
    bool resetMerge(GFX::CommandType type) const;

    const std::type_info& getType(GFX::CommandType type) const;
    GFX::Command* getCommandInternal(const CommandEntry& commandEntry);
    const GFX::Command* getCommandInternal(const CommandEntry& commandEntry) const;
  protected:
      vectorEASTL<CommandEntry> _commandOrder;
      boost::base_collection<GFX::Command> _commands;
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
void SetBlend(CommandBuffer& buffer, const SetBlendCommand& cmd);
void SetCamera(CommandBuffer& buffer, const SetCameraCommand& cmd);
void SetClipPlanes(CommandBuffer& buffer, const SetClipPlanesCommand& cmd);
void BindPipeline(CommandBuffer& buffer, const BindPipelineCommand& cmd);
void SendPushConstants(CommandBuffer& buffer, const SendPushConstantsCommand& cmd);
void BindDescriptorSets(CommandBuffer& buffer, const BindDescriptorSetsCommand& cmd);
void BeginDebugScope(CommandBuffer& buffer, const BeginDebugScopeCommand& cmd);
void EndDebugScope(CommandBuffer& buffer, const EndDebugScopeCommand& cmd);
void AddDrawCommands(CommandBuffer& buffer, const DrawCommand& cmd);
void AddDrawTextCommand(CommandBuffer& buffer, const DrawTextCommand& cmd);
void AddDrawIMGUICommand(CommandBuffer& buffer, const DrawIMGUICommand& cmd);
void AddComputeCommand(CommandBuffer& buffer, const DispatchComputeCommand& cmd);
void AddSwitchWindow(CommandBuffer& buffer, const SwitchWindowCommand& cmd);
}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_

#include "CommandBuffer.inl"