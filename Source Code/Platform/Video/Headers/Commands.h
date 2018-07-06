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
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    SWITCH_WINDOW,
    COUNT
};

class CommandBuffer;

struct CommandBase {
    CommandBase() : CommandBase(CommandType::COUNT, "null") {}
    CommandBase(CommandType type, const char* typeName) : _type(type), _typeName(typeName) {}

    virtual ~CommandBase() = default;

    virtual const std::type_info& type() const = 0;
    virtual bool onBufferAdd(CommandBuffer& buffer) const = 0;

    virtual const char* toString() const { return _typeName; }

    CommandType _type;
    const char* _typeName = nullptr;;
};

template<typename T, CommandType enumVal>
struct Command : public CommandBase {
    Command() : CommandBase(enumVal, TO_STRING(enumVal)) { }
    
    virtual ~Command() = default;

    bool onBufferAdd(CommandBuffer& buffer) const override {
        return buffer.registerType<T>();
    }

    const std::type_info& type() const override { return typeid (T); };
};

struct BindPipelineCommand final : Command<BindPipelineCommand, CommandType::BIND_PIPELINE> {
    const Pipeline* _pipeline = nullptr;
};

struct SendPushConstantsCommand final : Command<SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS> {
    PushConstants _constants;
};

struct DrawCommand final : Command<DrawCommand, CommandType::DRAW_COMMANDS> {
    vectorEASTL<GenericDrawCommand> _drawCommands;
};

struct SetViewportCommand final : Command<SetViewportCommand, CommandType::SET_VIEWPORT> {
    Rect<I32> _viewport;

    const char* toString() const override {
        return (_typeName + Util::StringFormat("[%d, %d, %d, %d]", _viewport.x, _viewport.y, _viewport.z, _viewport.w)).c_str();
    }
};

struct BeginRenderPassCommand final : Command<BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS> {
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    eastl::fixed_string<char, 128 + 1, true> _name;

    const char* toString() const override {
        return (stringImpl(_typeName) + _name.c_str()).c_str();
    }
};

struct EndRenderPassCommand final : Command<EndRenderPassCommand, CommandType::END_RENDER_PASS> {
};

struct BeginPixelBufferCommand final : Command<BeginPixelBufferCommand, CommandType::BEGIN_PIXEL_BUFFER> {
    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
};

struct EndPixelBufferCommand final : Command<EndPixelBufferCommand, CommandType::END_PIXEL_BUFFER> {
};

struct BeginRenderSubPassCommand final : Command<BeginRenderSubPassCommand, CommandType::BEGIN_RENDER_SUB_PASS> {
    U16 _mipWriteLevel = 0u;
};

struct EndRenderSubPassCommand final : Command<EndRenderSubPassCommand, CommandType::END_RENDER_SUB_PASS> {
};

struct BlitRenderTargetCommand final : Command<BlitRenderTargetCommand, CommandType::BLIT_RT> {
    bool _blitColour = true;
    bool _blitDepth = false;
    RenderTargetID _source;
    RenderTargetID _destination;
};

struct SetScissorCommand final : Command<SetScissorCommand, CommandType::SET_SCISSOR> {
    Rect<I32> _rect;
};

struct SetBlendCommand final : Command<SetBlendCommand, CommandType::SET_BLEND> {
    bool _enabled = true;
    BlendingProperties _blendProperties;
};

struct SetCameraCommand final : Command<SetCameraCommand, CommandType::SET_CAMERA> {
    Camera* _camera = nullptr;
};

struct SetClipPlanesCommand final : Command<SetClipPlanesCommand, CommandType::SET_CLIP_PLANES> {
    FrustumClipPlanes _clippingPlanes;
};

struct BindDescriptorSetsCommand final : Command<BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS> {
    DescriptorSet_ptr _set = nullptr;
};

struct BeginDebugScopeCommand final : Command<BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE> {
    eastl::fixed_string<char, 128 + 1, true> _scopeName;
    I32 _scopeID = -1;

    const char* toString() const override { 
        return (stringImpl(_typeName) + _scopeName.c_str()).c_str();
    }
};

struct EndDebugScopeCommand final : Command<EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE> {
};

struct DrawTextCommand final : Command<DrawTextCommand, CommandType::DRAW_TEXT> {
    TextElementBatch _batch;
};

struct DrawIMGUICommand final : Command<DrawIMGUICommand, CommandType::DRAW_IMGUI> {
    ImDrawData* _data = nullptr;
};

struct DispatchComputeCommand final : Command<DispatchComputeCommand, CommandType::DISPATCH_COMPUTE> {
    ComputeParams _params;
};

struct SwitchWindowCommand final : Command<SwitchWindowCommand, CommandType::SWITCH_WINDOW> {
    I64 windowGUID = -1;
};

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
