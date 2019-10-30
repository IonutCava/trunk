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

struct ImDrawData;

namespace Divide {
namespace GFX {

constexpr size_t g_commandPoolSizeFactor = 256;

template<typename T>
struct CmdAllocator {
    static std::mutex s_PoolMutex;
    static MemoryPool<T, prevPOW2(sizeof(T) * g_commandPoolSizeFactor)> s_Pool;

    template <class... Args>
    static T* allocate(Args&&... args) {
        return s_Pool.newElement(s_PoolMutex, std::forward<Args>(args)...);
    }

    static void deallocate(T*& ptr) {
        s_Pool.deleteElement(s_PoolMutex, ptr);
    }
};

#define TO_STR(arg) #arg

#define DEFINE_POOL(Command) \
decltype(CmdAllocator<Command>::s_PoolMutex) CmdAllocator<Command>::s_PoolMutex; \
decltype(CmdAllocator<Command>::s_Pool) CmdAllocator<Command>::s_Pool; \

#define BEGIN_COMMAND(Name, Enum) struct Name final : Command<Name, Enum> { \
typedef Command<Name, Enum> Base; \
inline const char* commandName() const final { return TO_STR(Enum); }

#define END_COMMAND(Name) \
}

enum class CommandType : U8 {
    BEGIN_RENDER_PASS,
    END_RENDER_PASS,
    BEGIN_PIXEL_BUFFER,
    END_PIXEL_BUFFER,
    BEGIN_RENDER_SUB_PASS,
    END_RENDER_SUB_PASS,
    SET_VIEWPORT,
    SET_SCISSOR,
    SET_BLEND,
    CLEAR_RT,
    RESET_RT,
    RESET_AND_CLEAR_RT,
    BLIT_RT,
    RESOLVE_RT,
    COPY_TEXTURE,
    COMPUTE_MIPMAPS,
    SET_CAMERA,
    PUSH_CAMERA,
    POP_CAMERA,
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
};

class CommandBuffer;

struct CommandBase
{
    virtual void addToBuffer(CommandBuffer& buffer) const = 0;
    virtual stringImpl toString(U16 indent) const = 0;
    virtual const char* commandName() const = 0;
};

template<typename T, CommandType EnumVal>
struct Command : public CommandBase {
    inline void addToBuffer(CommandBuffer& buffer) const final {
        buffer.add(reinterpret_cast<const T&>(*this));
    }

    virtual stringImpl toString(U16 indent) const override {
        ACKNOWLEDGE_UNUSED(indent);
        return stringImpl(commandName());
    }

    static constexpr CommandType EType = EnumVal;
};

BEGIN_COMMAND(BindPipelineCommand, CommandType::BIND_PIPELINE);
    BindPipelineCommand(const Pipeline* pipeline)
        : _pipeline(pipeline)
    {
    }

    BindPipelineCommand() = default;
    ~BindPipelineCommand() = default;

    const Pipeline* _pipeline = nullptr;

    stringImpl toString(U16 indent) const final;
END_COMMAND(BindPipelineCommand);


BEGIN_COMMAND(SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS);
    SendPushConstantsCommand(const PushConstants& constants)
        : _constants(constants)
    {
    }

    SendPushConstantsCommand() = default;
    ~SendPushConstantsCommand() = default;

    PushConstants _constants;

    stringImpl toString(U16 indent) const final;
END_COMMAND(SendPushConstantsCommand);


BEGIN_COMMAND(DrawCommand, CommandType::DRAW_COMMANDS);
    static_assert(sizeof(GenericDrawCommand) == 32, "Wrong command size! May cause performance issues. Disable assert to continue anyway.");

    DrawCommand() {}
    DrawCommand(size_t reserveSize) { _drawCommands.reserve(reserveSize); }
    DrawCommand(const GenericDrawCommand& cmd) : _drawCommands({ cmd }) {}
    DrawCommand(const vectorEASTLFast<GenericDrawCommand>& cmds) : _drawCommands(cmds) {}

    vectorEASTLFast<GenericDrawCommand> _drawCommands;

    stringImpl toString(U16 indent) const final;
END_COMMAND(DrawCommand);


BEGIN_COMMAND(SetViewportCommand, CommandType::SET_VIEWPORT);
    Rect<I32> _viewport;

    stringImpl toString(U16 indent) const final;
END_COMMAND(SetViewportCommand);

BEGIN_COMMAND(BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS);
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    Str64 _name = "";

    stringImpl toString(U16 indent) const final;
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

BEGIN_COMMAND(ClearRenderTargetCommand, CommandType::CLEAR_RT);
    RenderTargetID _target;
    RTClearDescriptor _descriptor;
END_COMMAND(ClearRenderTargetCommand);

BEGIN_COMMAND(ResetRenderTargetCommand, CommandType::RESET_RT);
    RenderTargetID _source;
    RTDrawDescriptor _descriptor;
END_COMMAND(ResetRenderTargetCommand);

BEGIN_COMMAND(ResetAndClearRenderTargetCommand, CommandType::RESET_AND_CLEAR_RT);
    RenderTargetID _source;
    RTDrawDescriptor _drawDescriptor;
    RTClearDescriptor _clearDescriptor;
END_COMMAND(ResetAndClearRenderTargetCommand);

BEGIN_COMMAND(ResolveRenderTargetCommand, CommandType::RESOLVE_RT);
    RenderTargetID _source;
    I8   _resolveColour = -1; //must be less than MAX_RT_COLOUR_ATTACHMENTS
    bool _resolveColours = false;
    bool _resolveDepth = false;
    bool _resolveExternalColours = false;
END_COMMAND(ResolveRenderTargetCommand);

BEGIN_COMMAND(CopyTextureCommand, CommandType::COPY_TEXTURE);
    TextureData _source;
    TextureData _destination;
    CopyTexParams _params;
END_COMMAND(CopyTextureCommand);

BEGIN_COMMAND(ComputeMipMapsCommand, CommandType::COMPUTE_MIPMAPS);
    Texture* _texture = nullptr;
    vec2<U32> _layerRange = { 0, 1 };
END_COMMAND(ComputeMipMapsCommand);

BEGIN_COMMAND(SetScissorCommand, CommandType::SET_SCISSOR);
    Rect<I32> _rect;

    stringImpl toString(U16 indent) const final;
END_COMMAND(SetScissorCommand);

BEGIN_COMMAND(SetBlendCommand, CommandType::SET_BLEND);
    BlendingProperties _blendProperties;
END_COMMAND(SetBlendCommand);

BEGIN_COMMAND(SetCameraCommand, CommandType::SET_CAMERA);
    SetCameraCommand() = default;
    SetCameraCommand(const CameraSnapshot& snapshot) : _cameraSnapshot(snapshot) {}

    CameraSnapshot _cameraSnapshot;

    stringImpl toString(U16 indent) const final;
END_COMMAND(PushCameraCommand);

BEGIN_COMMAND(PushCameraCommand, CommandType::PUSH_CAMERA);
    CameraSnapshot _cameraSnapshot;
END_COMMAND(PushCameraCommand);

BEGIN_COMMAND(PopCameraCommand, CommandType::POP_CAMERA);
END_COMMAND(PopCameraCommand);

BEGIN_COMMAND(SetClipPlanesCommand, CommandType::SET_CLIP_PLANES);
    FrustumClipPlanes _clippingPlanes;

    stringImpl toString(U16 indent) const final;
END_COMMAND(SetClipPlanesCommand);

BEGIN_COMMAND(BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS);
    DescriptorSet _set;

    stringImpl toString(U16 indent) const final;

END_COMMAND(BindDescriptorSetsCommand);

BEGIN_COMMAND(BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE);
    Str64 _scopeName;
    I32 _scopeID = -1;

    stringImpl toString(U16 indent) const final;
END_COMMAND(BeginDebugScopeCommand);

BEGIN_COMMAND(EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE);
END_COMMAND(EndDebugScopeCommand);

BEGIN_COMMAND(DrawTextCommand, CommandType::DRAW_TEXT);
    TextElementBatch _batch;

    stringImpl toString(U16 indent) const final;
END_COMMAND(DrawTextCommand);

BEGIN_COMMAND(DrawIMGUICommand, CommandType::DRAW_IMGUI);
    ImDrawData* _data = nullptr;
    I64 _windowGUID = 0;
END_COMMAND(DrawIMGUICommand);

BEGIN_COMMAND(DispatchComputeCommand, CommandType::DISPATCH_COMPUTE);
    vec3<U32> _computeGroupSize;

    stringImpl toString(U16 indent) const final;
END_COMMAND(DispatchComputeCommand);

BEGIN_COMMAND(MemoryBarrierCommand, CommandType::MEMORY_BARRIER);
    U8 _barrierMask = 0;

    stringImpl toString(U16 indent) const final;
END_COMMAND(MemoryBarrierCommand);

BEGIN_COMMAND(ReadBufferDataCommand, CommandType::READ_BUFFER_DATA);
    ShaderBuffer* _buffer = nullptr;
    bufferPtr     _target = nullptr;
    U32           _offsetElementCount = 0;
    U32           _elementCount = 0;
END_COMMAND(ReadBufferDataCommand);


BEGIN_COMMAND(ClearBufferDataCommand, CommandType::CLEAR_BUFFER_DATA);
    ShaderBuffer* _buffer = nullptr;
    U32           _offsetElementCount = 0;
    U32           _elementCount = 0;
END_COMMAND(ClearBufferDataCommand);

BEGIN_COMMAND(ExternalCommand, CommandType::EXTERNAL);
    std::function<void()> _cbk;
END_COMMAND(ExternalCommand);

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
