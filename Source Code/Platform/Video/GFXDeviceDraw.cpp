#include "stdafx.h"

#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

void GFXDevice::uploadGPUBlock() {
    if (_gpuBlock._needsUpload) {
        // We flush the entire buffer on update to inform the GPU that we don't
        // need the previous data. Might avoid some driver sync
        _gfxDataBuffer->writeData(&_gpuBlock._data);
        _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);
        _api->updateClipPlanes(_clippingPlanes);

        _gpuBlock._needsUpload = false;
    }
}

void GFXDevice::flushCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    if (Config::ENABLE_GPU_VALIDATION) {
        DIVIDE_ASSERT(Runtime::isMainThread(), "GFXDevice::flushCommandBuffer called from worker thread!");
    }

    commandBuffer.batch();

    if (!commandBuffer.validate()) {
        Console::errorfn(Locale::get(_ID("ERROR_GFX_INVALID_COMMAND_BUFFER")), commandBuffer.toString().c_str());
        DIVIDE_ASSERT(false, "GFXDevice::flushCommandBuffer error: Invalid command buffer. Check error log!");
        return;
    }

    const vectorEASTL<GFX::CommandBuffer::CommandEntry>& commands = commandBuffer();
    for (const GFX::CommandBuffer::CommandEntry& cmd : commands) {
        switch (cmd.type<GFX::CommandType::_enumerated>()) {
            case GFX::CommandType::BLIT_RT: {
                const GFX::BlitRenderTargetCommand& crtCmd = commandBuffer.getCommand<GFX::BlitRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._source);
                RenderTarget& destination = renderTargetPool().renderTarget(crtCmd._destination);
                destination.blitFrom(&source, crtCmd._blitColour, crtCmd._blitDepth);
            } break;
            case GFX::CommandType::RESET_RT: {
                const GFX::ResetRenderTargetCommand& crtCmd = commandBuffer.getCommand<GFX::ResetRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._source);
                source.setDefaultState(crtCmd._descriptor);
            }break;
            case GFX::CommandType::READ_ATOMIC_COUNTER: {
                const GFX::ReadAtomicCounterCommand& crtCmd = commandBuffer.getCommand<GFX::ReadAtomicCounterCommand>(cmd);
                if (crtCmd._buffer != nullptr && crtCmd._target != nullptr) {
                    *crtCmd._target = crtCmd._buffer->getAtomicCounter(crtCmd._offset);
                    if (*crtCmd._target > 0 && crtCmd._resetCounter) {
                        crtCmd._buffer->resetAtomicCounter(crtCmd._offset);
                    }
                }
            } break;

            case GFX::CommandType::SET_VIEWPORT:
                setViewport(commandBuffer.getCommand<GFX::SetViewportCommand>(cmd)._viewport);
                break;
            case GFX::CommandType::SET_CAMERA:
                renderFromCamera(commandBuffer.getCommand<GFX::SetCameraCommand>(cmd)._cameraSnapshot);
                break;
            case GFX::CommandType::SET_CLIP_PLANES:
                setClipPlanes(commandBuffer.getCommand<GFX::SetClipPlanesCommand>(cmd)._clippingPlanes);
                break;
            case GFX::CommandType::EXTERNAL:
                commandBuffer.getCommand<GFX::ExternalCommand>(cmd)._cbk();
                break;

            case GFX::CommandType::DRAW_TEXT:
            case GFX::CommandType::DRAW_IMGUI:
            case GFX::CommandType::DRAW_COMMANDS:
            case GFX::CommandType::DISPATCH_COMPUTE:
                uploadGPUBlock(); /*no break. fall-through*/

            default:
                _api->flushCommand(cmd, commandBuffer);
                break;
        }
    }
}

void GFXDevice::occlusionCull(const RenderPass::BufferData& bufferData,
                              U32 bufferIndex,
                              const Texture_ptr& depthBuffer,
                              GFX::CommandBuffer& bufferInOut) const {

    constexpr U32 GROUP_SIZE_AABB = 64;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = "Occlusion Cull";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::BindPipelineCommand bindPipelineCmd;
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._shaderProgramHandle = _HIZCullProgram->getID();
    bindPipelineCmd._pipeline = newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    ShaderBufferBinding shaderBuffer;
    shaderBuffer._binding = ShaderBufferLocation::GPU_COMMANDS;
    shaderBuffer._buffer = bufferData._cmdBuffers[bufferIndex];
    shaderBuffer._range.set(0, to_U16(bufferData._cmdBuffers[bufferIndex]->getPrimitiveCount()));
    shaderBuffer._atomicCounter.first = true;
    
    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd;
    bindDescriptorSetsCmd._set = newDescriptorSet();
    bindDescriptorSetsCmd._set->_shaderBuffers.push_back(shaderBuffer);
    bindDescriptorSetsCmd._set->_textureData.addTexture(depthBuffer->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);
    
    U32 cmdCount = bufferData._lastCommandCount;

    GFX::SendPushConstantsCommand sendPushConstantsCmd;
    sendPushConstantsCmd._constants.set("dvd_numEntities", GFX::PushConstantType::UINT, cmdCount);
    GFX::EnqueueCommand(bufferInOut, sendPushConstantsCmd);

    GFX::DispatchComputeCommand computeCmd;
    computeCmd._params._barrierType = MemoryBarrierType::COUNTER;
    computeCmd._params._groupSize = vec3<U32>((cmdCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void GFXDevice::drawText(const TextElementBatch& batch, GFX::CommandBuffer& bufferInOut) const {
    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = _textRenderPipeline;

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _textRenderConstants;

    GFX::DrawTextCommand drawTextCommand;
    drawTextCommand._batch = batch;

    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);
    
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);
    GFX::EnqueueCommand(bufferInOut, drawTextCommand);
}

void GFXDevice::drawText(const TextElementBatch& batch) {
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    GFX::CommandBuffer& buffer = sBuffer();

    // Assume full game window viewport for text
    GFX::SetViewportCommand viewportCommand;
    RenderTarget& screenRT = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    U16 width = screenRT.getWidth();
    U16 height = screenRT.getHeight();
    viewportCommand._viewport.set(0, 0, width, height);
    GFX::EnqueueCommand(buffer, viewportCommand);

    drawText(batch, buffer);
    flushCommandBuffer(sBuffer());
}

void GFXDevice::drawTextureInRenderViewport(TextureData data, GFX::CommandBuffer& bufferInOut) const {
    drawTextureInViewport(data, _baseViewport, bufferInOut);
}

void GFXDevice::drawTextureInRenderWindow(TextureData data, GFX::CommandBuffer& bufferInOut) const {
    const vec2<U16>& dim = context().app().windowManager().getActiveWindow().getDimensions();
    drawTextureInViewport(data, Rect<I32>(0, 0, dim.width, dim.height), bufferInOut);
}

void GFXDevice::drawTextureInViewport(TextureData data, const Rect<I32>& viewport, GFX::CommandBuffer& bufferInOut) const {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _displayShader->getID();

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 123456332;
    beginDebugScopeCmd._scopeName = "Draw Fullscreen Texture";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd;
    bindDescriptorSetsCmd._set = newDescriptorSet();
    bindDescriptorSetsCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    GFX::SetViewportCommand viewportCommand;
    viewportCommand._viewport.set(viewport);
    GFX::EnqueueCommand(bufferInOut, viewportCommand);

    // Blit render target to screen
    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndDebugScopeCommand endDebugScopeCommand;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
}

void GFXDevice::blitToScreen(const Rect<I32>& targetViewport) {
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    GFX::CommandBuffer& buffer = sBuffer();

    blitToBuffer(targetViewport, buffer);
    
    flushCommandBuffer(buffer);
}

void GFXDevice::blitToRenderTarget(RenderTargetID targetID, const Rect<I32>& targetViewport) {
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    GFX::CommandBuffer& buffer = sBuffer();

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = targetID;
    beginRenderPassCmd._name = "BLIT_TO_RENDER_TARGET";
    GFX::EnqueueCommand(buffer, beginRenderPassCmd);

    blitToBuffer(targetViewport, buffer);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(buffer, endRenderPassCmd);

    flushCommandBuffer(buffer);
}

void GFXDevice::blitToBuffer(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    {
        RenderTarget& screen = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
        TextureData texData = screen.getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO)).texture()->getData();

        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 12345;
        beginDebugScopeCmd._scopeName = "Flush Display";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        drawTextureInViewport(texData, targetViewport, bufferInOut);

        GFX::EndDebugScopeCommand endDebugScopeCommand;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }
    {
        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 123456;
        beginDebugScopeCmd._scopeName = "Render GUI";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        _parent.platformContext().gui().draw(*this, bufferInOut);

        GFX::EndDebugScopeCommand endDebugScopeCommand;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }

    if (Config::Build::IS_DEBUG_BUILD)
    {
        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 123456;
        beginDebugScopeCmd._scopeName = "Render Debug Views";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        renderDebugViews(bufferInOut);

        GFX::EndDebugScopeCommand endDebugScopeCommand;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }
}
};
