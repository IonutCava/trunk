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


#ifndef TO_STR
#define TO_STR(arg) #arg
#endif

#include "ClipPlanes.h"
#include "DescriptorSets.h"
#include "GenericDrawCommand.h"
#include "PushConstants.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Rendering/Camera/Headers/CameraSnapshot.h"
#include "Utility/Headers/TextLabel.h"

struct ImDrawData;

namespace Divide {
    class Pipeline;
    class ShaderBuffer;
    class PixelBuffer;

    namespace GFX {

constexpr size_t g_commandPoolSizeFactor = 256;

template<typename T>
struct CmdAllocator {
    static Mutex s_PoolMutex;
    static MemoryPool<T, prevPOW2(sizeof(T) * g_commandPoolSizeFactor)> s_Pool;

    template <class... Args>
    static T* allocate(Args&&... args) {
        return s_Pool.newElement(s_PoolMutex, std::forward<Args>(args)...);
    }

    static void deallocate(T*& ptr) {
        s_Pool.deleteElement(s_PoolMutex, ptr);
    }
};

enum class CommandType : U8 {
    BEGIN_RENDER_PASS,
    END_RENDER_PASS,
    BEGIN_PIXEL_BUFFER,
    END_PIXEL_BUFFER,
    BEGIN_RENDER_SUB_PASS,
    END_RENDER_SUB_PASS,
    SET_BLEND_STATE,
    SET_VIEWPORT,
    PUSH_VIEWPORT,
    POP_VIEWPORT,
    SET_SCISSOR,
    SET_BLEND,
    CLEAR_RT,
    RESET_RT,
    RESET_AND_CLEAR_RT,
    BLIT_RT,
    COPY_TEXTURE,
    SET_MIP_LEVELS,
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
    SET_CLIPING_STATE,
    EXTERNAL,
    COUNT
};

namespace Names {
    static const char* commandType[] = {
        "BEGIN_RENDER_PASS", "END_RENDER_PASS", "BEGIN_PIXEL_BUFFER", "END_PIXEL_BUFFER", "BEGIN_RENDER_SUB_PASS",
        "END_RENDER_SUB_PASS", "SET_BLEND_STATE", "SET_VIEWPORT", "PUSH_VIEWPORT","POP_VIEWPORT", "SET_SCISSOR", "SET_BLEND", "CLEAR_RT",
        "RESET_RT", "RESET_AND_CLEAR_RT", "BLIT_RT", "COPY_TEXTURE", "SET_MIP_LEVELS", "COMPUTE_MIPMAPS", "SET_CAMERA",
        "PUSH_CAMERA", "POP_CAMERA", "SET_CLIP_PLANES", "BIND_PIPELINE", "BIND_DESCRIPTOR_SETS", "SEND_PUSH_CONSTANTS",
        "DRAW_COMMANDS", "DRAW_TEXT", "DRAW_IMGUI", "DISPATCH_COMPUTE", "MEMORY_BARRIER", "READ_BUFFER_DATA", "CLEAR_BUFFER_DATA",
        "BEGIN_DEBUG_SCOPE","END_DEBUG_SCOPE", "SWITCH_WINDOW", "SET_CLIPING_STATE", "EXTERNAL", "UNKNOWN"
    };
};

class CommandBuffer;
struct CommandBase;
struct Deleter {
    virtual ~Deleter() = default;
    virtual void del(CommandBase*& cmd) const { ACKNOWLEDGE_UNUSED(cmd); }
};

template<typename T>
struct DeleterImpl final : Deleter {
    void del(CommandBase*& cmd) const final {
        CmdAllocator<T>::deallocate((T*&)(cmd));
        cmd = nullptr;
    }
};

struct CommandBase
{
    explicit CommandBase(const CommandType type) noexcept : EType(type) {}
    virtual ~CommandBase() = default;

    virtual void addToBuffer(CommandBuffer* buffer) const = 0;
    [[nodiscard]] virtual Deleter& getDeleter() const noexcept = 0;

    [[nodiscard]] const char* Name() const noexcept { return Names::commandType[to_base(EType)]; }
    [[nodiscard]] CommandType Type() const noexcept { return EType; }

protected:
    CommandType EType = CommandType::COUNT;
};

stringImpl ToString(const CommandBase& cmd, U16 indent);

template<typename T, CommandType EnumVal>
struct Command : CommandBase {
    static constexpr CommandType EType = EnumVal;

    Command() noexcept : CommandBase(EnumVal) {}

    void addToBuffer(CommandBuffer* buffer) const final;

protected:
    void DELETE_CMD(GFX::CommandBase*& cmd) noexcept;

    [[nodiscard]] Deleter& getDeleter() const noexcept final {
        static DeleterImpl<T> s_deleter;
        return s_deleter; 
    }
};

#define IMPLEMENT_COMMAND(Command) \
decltype(CmdAllocator<Command>::s_PoolMutex) CmdAllocator<Command>::s_PoolMutex; \
decltype(CmdAllocator<Command>::s_Pool) CmdAllocator<Command>::s_Pool;

#define BEGIN_COMMAND(Name, Enum) struct Name final : public Command<Name, Enum> { \
using Base = Command<Name, Enum>; \
Name() = default; \

#define END_COMMAND(Name) }

BEGIN_COMMAND(BindPipelineCommand, CommandType::BIND_PIPELINE);
    BindPipelineCommand(const Pipeline* pipeline) noexcept : _pipeline(pipeline) {}

    const Pipeline* _pipeline = nullptr;
END_COMMAND(BindPipelineCommand);


BEGIN_COMMAND(SendPushConstantsCommand, CommandType::SEND_PUSH_CONSTANTS);
    SendPushConstantsCommand(const PushConstants& constants) noexcept : _constants(constants) {}

    PushConstants _constants;
END_COMMAND(SendPushConstantsCommand);


BEGIN_COMMAND(DrawCommand, CommandType::DRAW_COMMANDS);
    using CommandContainer = eastl::fixed_vector<GenericDrawCommand, 4, true, eastl::dvd_eastl_allocator>;

    static_assert(sizeof(GenericDrawCommand) == 32, "Wrong command size! May cause performance issues. Disable assert to continue anyway.");

    DrawCommand(const GenericDrawCommand& cmd) noexcept : _drawCommands{ { cmd } } {}

    CommandContainer _drawCommands;
END_COMMAND(DrawCommand);


BEGIN_COMMAND(SetViewportCommand, CommandType::SET_VIEWPORT);
    SetViewportCommand(const Rect<I32>& viewport) noexcept : _viewport(viewport) {}

    Rect<I32> _viewport;
END_COMMAND(SetViewportCommand);

BEGIN_COMMAND(PushViewportCommand, CommandType::PUSH_VIEWPORT);
    PushViewportCommand(const Rect<I32>& viewport) noexcept : _viewport(viewport) {}

    Rect<I32> _viewport;
END_COMMAND(PushViewportCommand);

BEGIN_COMMAND(PopViewportCommand, CommandType::POP_VIEWPORT);
END_COMMAND(PopViewportCommand);

BEGIN_COMMAND(BeginRenderPassCommand, CommandType::BEGIN_RENDER_PASS);
    RenderTargetID _target;
    RTDrawDescriptor _descriptor;
    Str64 _name = "";
 END_COMMAND(BeginRenderPassCommand);

BEGIN_COMMAND(EndRenderPassCommand, CommandType::END_RENDER_PASS);
    bool _setDefaultRTState = true;
END_COMMAND(EndRenderPassCommand);

BEGIN_COMMAND(BeginPixelBufferCommand, CommandType::BEGIN_PIXEL_BUFFER);
    PixelBuffer* _buffer = nullptr;
    DELEGATE<void, bufferPtr> _command;
END_COMMAND(BeginPixelBufferCommand);

BEGIN_COMMAND(EndPixelBufferCommand, CommandType::END_PIXEL_BUFFER);
END_COMMAND(EndPixelBufferCommand);

BEGIN_COMMAND(BeginRenderSubPassCommand, CommandType::BEGIN_RENDER_SUB_PASS);
    U16 _mipWriteLevel = std::numeric_limits<U16>::max();
    vectorEASTL<RenderTarget::DrawLayerParams> _writeLayers;
END_COMMAND(BeginRenderSubPassCommand);

BEGIN_COMMAND(SetBlendStateCommand, CommandType::SET_BLEND_STATE);
    RTBlendStates _blendStates;
END_COMMAND(SetBlendStateCommand);

BEGIN_COMMAND(EndRenderSubPassCommand, CommandType::END_RENDER_SUB_PASS);
END_COMMAND(EndRenderSubPassCommand);

BEGIN_COMMAND(BlitRenderTargetCommand, CommandType::BLIT_RT);
    // Depth layer to blit
    DepthBlitEntry _blitDepth;
    // List of colours + colour layer to blit
    std::array<ColourBlitEntry, RT_MAX_COLOUR_ATTACHMENTS> _blitColours;
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

BEGIN_COMMAND(CopyTextureCommand, CommandType::COPY_TEXTURE);
    TextureData _source;
    TextureData _destination;
    CopyTexParams _params;
END_COMMAND(CopyTextureCommand);

BEGIN_COMMAND(ComputeMipMapsCommand, CommandType::COMPUTE_MIPMAPS);
    Texture* _texture = nullptr;
    vec2<U32> _layerRange = { 0, 1 };
    bool _defer = true;
END_COMMAND(ComputeMipMapsCommand);

BEGIN_COMMAND(SetScissorCommand, CommandType::SET_SCISSOR);
    Rect<I32> _rect;
END_COMMAND(SetScissorCommand);

BEGIN_COMMAND(SetBlendCommand, CommandType::SET_BLEND);
    BlendingProperties _blendProperties;
END_COMMAND(SetBlendCommand);

BEGIN_COMMAND(SetCameraCommand, CommandType::SET_CAMERA);
    SetCameraCommand(const CameraSnapshot& cameraSnapshot) noexcept : _cameraSnapshot(cameraSnapshot) {}

    CameraSnapshot _cameraSnapshot;
END_COMMAND(SetCameraCommand);

BEGIN_COMMAND(PushCameraCommand, CommandType::PUSH_CAMERA);
    PushCameraCommand(const CameraSnapshot& cameraSnapshot) noexcept : _cameraSnapshot(cameraSnapshot) {}

    CameraSnapshot _cameraSnapshot;
END_COMMAND(PushCameraCommand);

BEGIN_COMMAND(PopCameraCommand, CommandType::POP_CAMERA);
END_COMMAND(PopCameraCommand);

BEGIN_COMMAND(SetClipPlanesCommand, CommandType::SET_CLIP_PLANES);
    SetClipPlanesCommand(const FrustumClipPlanes& clippingPlanes) noexcept : _clippingPlanes(clippingPlanes) {}

    FrustumClipPlanes _clippingPlanes;
END_COMMAND(SetClipPlanesCommand);

BEGIN_COMMAND(BindDescriptorSetsCommand, CommandType::BIND_DESCRIPTOR_SETS);
    DescriptorSet _set;
END_COMMAND(BindDescriptorSetsCommand);

BEGIN_COMMAND(SetTextureMipLevelsCommand, CommandType::SET_MIP_LEVELS);
    Texture* _texture = nullptr;
    U16 _baseLevel = 0;
    U16 _maxLevel = 1000;
END_COMMAND(SetTextureMipLevelsCommand);

BEGIN_COMMAND(BeginDebugScopeCommand, CommandType::BEGIN_DEBUG_SCOPE);
    BeginDebugScopeCommand(const char* scopeName) noexcept : _scopeName(scopeName) {}

    Str64 _scopeName;
END_COMMAND(BeginDebugScopeCommand);

BEGIN_COMMAND(EndDebugScopeCommand, CommandType::END_DEBUG_SCOPE);
END_COMMAND(EndDebugScopeCommand);

BEGIN_COMMAND(DrawTextCommand, CommandType::DRAW_TEXT);
    DrawTextCommand(TextElementBatch&& batch) noexcept : _batch(std::move(batch)) {}
    DrawTextCommand(const TextElementBatch& batch) noexcept : _batch(batch) {}

    TextElementBatch _batch;
END_COMMAND(DrawTextCommand);

BEGIN_COMMAND(DrawIMGUICommand, CommandType::DRAW_IMGUI);
    ImDrawData* _data = nullptr;
    I64 _windowGUID = 0;
END_COMMAND(DrawIMGUICommand);

BEGIN_COMMAND(DispatchComputeCommand, CommandType::DISPATCH_COMPUTE);
    vec3<U32> _computeGroupSize;
END_COMMAND(DispatchComputeCommand);

BEGIN_COMMAND(MemoryBarrierCommand, CommandType::MEMORY_BARRIER);
    U32 _barrierMask = 0;
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

BEGIN_COMMAND(SetClippingStateCommand, CommandType::SET_CLIPING_STATE)
    bool _lowerLeftOrigin = true;
    bool _negativeOneToOneDepth = true;
END_COMMAND(SetClippingStateCommand);

BEGIN_COMMAND(ExternalCommand, CommandType::EXTERNAL);
    std::function<void()> _cbk;
END_COMMAND(ExternalCommand);

}; //namespace GFX
}; //namespace Divide

#endif //_GFX_COMMAND_H_
