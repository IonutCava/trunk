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

#include "Platform/Headers/PlatformDataTypes.h"
#include "Core/TemplateLibraries/Headers/Vector.h"

namespace Divide {

namespace GFX {

enum class CommandType : U8 {
    BEGIN_RENDER_PASS = 0,
    END_RENDER_PASS,
    BEGIN_RENDER_SUB_PASS,
    END_RENDER_SUB_PASS,
    SET_VIEWPORT,
    SET_SCISSOR,
    BIND_PIPELINE,
    BIND_DESCRIPTOR_SETS
};


struct Command {
    explicit Command(CommandType type)
        : _type(type)
    {
    }

    virtual void execute() = 0;

    CommandType _type;
};

struct BeginRenderPassCommand : Command {
    BeginRenderPassCommand()
        : Command(CommandType::BEGIN_RENDER_PASS)
    {
    }

    void execute() {}
};

struct EndRenderPassCommand : Command {
    EndRenderPassCommand()
        : Command(CommandType::END_RENDER_PASS)
    {
    }

    void execute() {}
};

struct BeginRenderSubPassCommand : Command {
    BeginRenderSubPassCommand()
        : Command(CommandType::BEGIN_RENDER_SUB_PASS)
    {
    }

    void execute() {}
};

struct EndRenderSubPassCommand : Command {
    EndRenderSubPassCommand()
        : Command(CommandType::END_RENDER_SUB_PASS)
    {
    }

    void execute() {}
};

struct SetViewportCommand : Command {
    SetViewportCommand()
        : Command(CommandType::SET_VIEWPORT)
    {
    }

    void execute() {}
};

struct SetScissorCommand : Command {
    SetScissorCommand()
        : Command(CommandType::SET_SCISSOR)
    {
    }

    void execute() {}
};

struct BindPipelineCommand : Command {
    BindPipelineCommand()
        : Command(CommandType::BIND_PIPELINE)
    {
    }

    void execute() {}
};

struct BindDescriptorSetsCommand : Command {
    BindDescriptorSetsCommand()
        : Command(CommandType::BIND_DESCRIPTOR_SETS)
    {
    }

    void execute() {}
};


struct CommandBuffer {
    vectorImpl<Command> _commands;
};

void BeginRenderPass(CommandBuffer& buffer, const BeginRenderPassCommand& cmd);
void EndRenderPass(CommandBuffer& buffer, const EndRenderPassCommand& cmd);
void BeginRenderSubPass(CommandBuffer& buffer, const BeginRenderSubPassCommand& cmd);
void EndRenderSubPass(CommandBuffer& buffer, const EndRenderSubPassCommand& cmd);
void SetViewPort(CommandBuffer& buffer, const SetViewportCommand& cmd);
void SetScissor(CommandBuffer& buffer, const SetScissorCommand& cmd);
void BindPipeline(CommandBuffer& buffer, const BindPipelineCommand& cmd);
void BindDescripotSets(CommandBuffer& buffer, const BindDescriptorSetsCommand& cmd);

}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_H_