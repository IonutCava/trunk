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

#include <BetterEnums/include/enum.h>

struct ImDrawData;

namespace Divide {
namespace GFX {

constexpr size_t g_commandPoolSizeFactor = 512;

template<typename T>
struct CmdAllocator {
    static std::mutex s_PoolMutex;
    static MemoryPool<T, nextPOW2(sizeof(T) * g_commandPoolSizeFactor)> s_Pool;

    template <class... Args>
    static T* allocate(Args&&... args) {
#if 1
        T* ptr = nullptr;
        {
            UniqueLock w_lock(s_PoolMutex);
            ptr = s_Pool.allocate();
        }
        s_Pool.construct<T>(ptr, std::forward<Args>(args)...);
        return ptr;
#else
        UniqueLock w_lock(s_PoolMutex);
        return s_Pool.newElement(std::forward<Args>(args)...);
#endif
    }

    static void deallocate(T* ptr) {
        if (ptr != nullptr) {
            ptr->~T();
            UniqueLock w_lock(s_PoolMutex);
            s_Pool.deallocate(ptr);
        }
    }
};

#define DEFINE_POOL(Command) \
decltype(CmdAllocator<Command>::s_PoolMutex) CmdAllocator<Command>::s_PoolMutex; \
decltype(CmdAllocator<Command>::s_Pool) CmdAllocator<Command>::s_Pool; \

#define BEGIN_COMMAND(Name, Enum) struct Name final : Command<Name, Enum> { \
typedef Command<Name, Enum> Base;

#define END_COMMAND(Name) \
}

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
    RESOLVE_RT,
    COPY_TEXTURE,
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
    READ_BUFFER_DATA,
    CLEAR_BUFFER_DATA,
    BEGIN_DEBUG_SCOPE,
    END_DEBUG_SCOPE,
    SWITCH_WINDOW,
    EXTERNAL,
    COUNT
);

class CommandBuffer;

struct CommandBase
{
    virtual void addToBuffer(CommandBuffer& buffer) const = 0;
    virtual stringImpl toString(U16 indent) const = 0;
};

template<typename T, CommandType::_enumerated EnumVal>
struct Command : public CommandBase {
    inline void addToBuffer(CommandBuffer& buffer) const override {
        buffer.add(reinterpret_cast<const T&>(*this));
    }

    virtual stringImpl toString(U16 indent) const {
        ACKNOWLEDGE_UNUSED(indent);
        return stringImpl(CommandType::_from_integral(to_base(EType))._to_string());
    }

    static const CommandType::_enumerated EType = EnumVal;
};

BEGIN_COMMAND(BindPipelineCommand, CommandType::BIND_PIPELINE);
    const Pipeline* _pipeline = nullptr;

    stringImpl toString(U16 indent) const override;
END_COMMAND(BindPipelineCommand);


BEGIN_COMMAND(SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS);
    SendPushConstantsCommand(const PushConstants& constants)
        : _constants(constants)
    {
    }

    SendPushConstantsCommand() = default;
    ~SendPushConstantsCommand() = default;

    PushConstants _constants;

    stringImpl toString(U16 indent) const override;
END_COMMAND(SendPushConstantsCommand);


BEGIN_COMMAND(DrawCommand, CommandType::DRAW_COMMANDS);
    vectorEASTLFast<GenericDrawCommand> _drawCommands;

    stringImpl toString(U16 indent) const override;
END_COMMAND(DrawCommand);


BEGIN_COMMAND(SetViewportCommand, CommandType::SET_VIEWPORT);
    Rect<I32> _viewport;

    stringImpl toString(U16 indent) const override;
END_COMMAND(SetViewportCommand);

BEGIN_COMMAND(BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS);
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    eastl::fixed_string<char, 128 + 1, false> _name = "";

    stringImpl toString(U16 indent) const override;
 END_COMMAND(BeginRenderPassCommand);

BEGIN_COMMAND(EndRenderPassCommand, CommandType::END_RENDER_PASS);
    bool _autoResolveMSAAColour = false;
    bool _autoResolveMSAAExternalColour = false;
    bool _autoResolveMSAADepth = false;
END_COMMAND(EndRenderPassCommand);

BEGIN_COMMAND(BeginPixelBufferCommand, CommandType::BEGIN_PIXEL_BUFFER);
    PixelBuffer* _buffer = nullptr;
    DELEGATE_CBK<void, bufferPtr> _command;
END_COMMAND(BeginPixelBufferCommand);

BEGIN_COMMAND(EndPixelBufferCommand, CommandType::END_PIXEL_BUFFER);
END_COMMAND(EndPixelBufferCommand);

BEGIN_COMMAND(BeginRenderSubPassCommand, CommandType::BEGIN_RENDER_SUB_PASS);
    bool _validateWriteLevel = false;
    U16 _mipWriteLevel = std::numeric_limits<U16>::max();
    vectorEASTL<RenderTarget::DrawLayerParams> _writeLayers;
END_COMMAND(BeginRenderSubPassCommand);

BEGIN_COMMAND(EndRenderSubPassCommand, CommandType::END_RENDER_SUB_PASS);
END_COMMAND(EndRenderSubPassCommand);

BEGIN_COMMAND(BlitRenderTargetCommand, CommandType::BLIT_RT);
    // List of depth layers to blit
    vectorEASTL<DepthBlitEntry> _blitDepth;
    // List of colours + colour layer to blit
    vectorEASTL<ColourBlitEntry> _blitColours;
    RenderTargetID _source;
    RenderTargetID _destination;
END_COMMAND(BlitRenderTargetCommand);

BEGIN_COMMAND(ResetRenderTargetCommand, CommandType::RESET_RT);
    RenderTargetID _source;
    RTDrawDescriptor _descriptor;
END_COMMAND(ResetRenderTargetCommand);

BEGIN_COMMAND(ResolveRenderTargetCommand, CommandType::RESOLVE_RT);
    RenderTargetID _source;
    I8   _resolveColour = -1; //must be less than MAX_RT_COLOUR_ATTACHMENTS
    bool _resolveColours = false;
    bool _resolveDepth = false;
    bool _resolveExternalColours = false;
END_COMMAND(ResolveRenderTargetCommand);

BEGIN_COMMAND(CopyTextureCommand, CommandType::COPY_TEXTURE);
    Texture_ptr _source = nullptr;
    Texture_ptr _destination = nullptr;
    CopyTexParams _params;
END_COMMAND(CopyTextureCommand);

BEGIN_COMMAND(ComputeMipMapsCommand, CommandType::COMPUTE_MIPMAPS);
    Texture* _texture;
    vec2<U32> _layerRange = { 0, 1 };
END_COMMAND(ComputeMipMapsCommand);

BEGIN_COMMAND(SetScissorCommand, CommandType::SET_SCISSOR);
    Rect<I32> _rect;

    stringImpl toString(U16 indent) const override;
END_COMMAND(SetScissorCommand);

BEGIN_COMMAND(SetBlendCommand, CommandType::SET_BLEND);
    BlendingProperties _blendProperties;
END_COMMAND(SetBlendCommand);

BEGIN_COMMAND(SetCameraCommand, CommandType::SET_CAMERA);
    CameraSnapshot _cameraSnapshot;
    RenderStage _stage = RenderStage::COUNT;
END_COMMAND(SetCameraCommand);

BEGIN_COMMAND(SetClipPlanesCommand, CommandType::SET_CLIP_PLANES);
    FrustumClipPlanes _clippingPlanes;

    stringImpl toString(U16 indent) const override;
END_COMMAND(SetClipPlanesCommand);

BEGIN_COMMAND(BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS);
    DescriptorSet _set;

    stringImpl toString(U16 indent) const override;

END_COMMAND(BindDescriptorSetsCommand);

BEGIN_COMMAND(BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE);
    eastl::fixed_string<char, 128 + 1, true> _scopeName;
    I32 _scopeID = -1;

    stringImpl toString(U16 indent) const override;
END_COMMAND(BeginDebugScopeCommand);

BEGIN_COMMAND(EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE);
END_COMMAND(EndDebugScopeCommand);

BEGIN_COMMAND(DrawTextCommand, CommandType::DRAW_TEXT);
    TextElementBatch _batch;

    stringImpl toString(U16 indent) const override;

END_COMMAND(DrawTextCommand);

BEGIN_COMMAND(DrawIMGUICommand, CommandType::DRAW_IMGUI);
    ImDrawData* _data = nullptr;
    I64 _windowGUID = 0;
END_COMMAND(DrawIMGUICommand);

BEGIN_COMMAND(DispatchComputeCommand, CommandType::DISPATCH_COMPUTE);
    vec3<U32> _computeGroupSize;

    stringImpl toString(U16 indent) const override;
END_COMMAND(DispatchComputeCommand);

BEGIN_COMMAND(MemoryBarrierCommand, CommandType::MEMORY_BARRIER);
    U32 _barrierMask = 0;

    stringImpl toString(U16 indent) const override;

END_COMMAND(MemoryBarrierCommand);

BEGIN_COMMAND(ReadBufferDataCommand, CommandType::READ_BUFFER_DATA);
    ShaderBuffer* _buffer = nullptr;
    bufferPtr     _target = nullptr;
    size_t        _offsetElementCount = 0;
    size_t        _elementCount = 0;
END_COMMAND(ReadBufferDataCommand);


BEGIN_COMMAND(ClearBufferDataCommand, CommandType::CLEAR_BUFFER_DATA);
    ShaderBuffer* _buffer = nullptr;
    size_t        _offsetElementCount = 0;
    size_t        _elementCount = 0;
END_COMMAND(ClearBufferDataCommand);

BEGIN_COMMAND(ExternalCommand, CommandType::EXTERNAL);
    std::function<void()> _cbk;
END_COMMAND(ExternalCommand);

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
