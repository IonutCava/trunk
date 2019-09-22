#include "stdafx.h"

#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Editor/Headers/Editor.h"

#include "Rendering/Headers/Renderer.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Configuration.h"
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
        _gpuBlock._needsUpload = false;
    }
}

void GFXDevice::flushCommandBuffer(GFX::CommandBuffer& commandBuffer, bool submitToGPU) {
    if (Config::ENABLE_GPU_VALIDATION) {
        DIVIDE_ASSERT(Runtime::isMainThread(), "GFXDevice::flushCommandBuffer called from worker thread!");

        I32 debugFrame = _context.config().debug.flushCommandBuffersOnFrame;
        if (debugFrame >= 0 && to_U32(FRAME_COUNT) == to_U32(debugFrame)) {
            Console::errorfn(commandBuffer.toString().c_str());
        }
    }

    commandBuffer.batch();

    if (!commandBuffer.validate()) {
        Console::errorfn(Locale::get(_ID("ERROR_GFX_INVALID_COMMAND_BUFFER")), commandBuffer.toString().c_str());
        DIVIDE_ASSERT(false, "GFXDevice::flushCommandBuffer error: Invalid command buffer. Check error log!");
        return;
    }

    _api->preFlushCommandBuffer(commandBuffer);

    const eastl::list<GFX::CommandBuffer::CommandEntry>& commands = commandBuffer();
    for (const GFX::CommandBuffer::CommandEntry& cmd : commands) {
        switch (static_cast<GFX::CommandType>(cmd._typeIndex)) {
            case GFX::CommandType::BLIT_RT: {
                const GFX::BlitRenderTargetCommand& crtCmd = commandBuffer.get<GFX::BlitRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._source);
                RenderTarget& destination = renderTargetPool().renderTarget(crtCmd._destination);

                RenderTarget::RTBlitParams params = {};
                params._inputFB = &source;
                params._blitDepth = crtCmd._blitDepth;
                params._blitColours = crtCmd._blitColours;

                destination.blitFrom(params);
            } break;
            case GFX::CommandType::CLEAR_RT: {
                const GFX::ClearRenderTargetCommand& crtCmd = commandBuffer.get<GFX::ClearRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._target);
                source.clear(crtCmd._descriptor);
            }break;
            case GFX::CommandType::RESET_RT: {
                const GFX::ResetRenderTargetCommand& crtCmd = commandBuffer.get<GFX::ResetRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._source);
                source.setDefaultState(crtCmd._descriptor);
            } break;
            case GFX::CommandType::RESET_AND_CLEAR_RT: {
                const GFX::ResetAndClearRenderTargetCommand& crtCmd = commandBuffer.get<GFX::ResetAndClearRenderTargetCommand>(cmd);
                RenderTarget& source = renderTargetPool().renderTarget(crtCmd._source);
                source.setDefaultState(crtCmd._drawDescriptor);
                source.clear(crtCmd._clearDescriptor);
            } break;
            case GFX::CommandType::READ_BUFFER_DATA: {
                const GFX::ReadBufferDataCommand& crtCmd = commandBuffer.get<GFX::ReadBufferDataCommand>(cmd);
                if (crtCmd._buffer != nullptr && crtCmd._target != nullptr) {
                    crtCmd._buffer->readData(crtCmd._offsetElementCount, crtCmd._elementCount, crtCmd._target);
                }
            } break;
            case GFX::CommandType::CLEAR_BUFFER_DATA: {
                const GFX::ClearBufferDataCommand& crtCmd = commandBuffer.get<GFX::ClearBufferDataCommand>(cmd);
                if (crtCmd._buffer != nullptr) {
                    crtCmd._buffer->clearData(crtCmd._offsetElementCount, crtCmd._elementCount);
                }
            } break;
            case GFX::CommandType::SET_VIEWPORT:
                setViewport(commandBuffer.get<GFX::SetViewportCommand>(cmd)._viewport);
                break;
            case GFX::CommandType::SET_CAMERA: {
                const GFX::SetCameraCommand& crtCmd = commandBuffer.get<GFX::SetCameraCommand>(cmd);
                renderFromCamera(crtCmd._cameraSnapshot, crtCmd._stage);
            } break;
            case GFX::CommandType::SET_CLIP_PLANES:
                setClipPlanes(commandBuffer.get<GFX::SetClipPlanesCommand>(cmd)._clippingPlanes);
                break;
            case GFX::CommandType::EXTERNAL:
                commandBuffer.get<GFX::ExternalCommand>(cmd)._cbk();
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
    _api->postFlushCommandBuffer(commandBuffer, submitToGPU);
}

void GFXDevice::occlusionCull(const RenderPass::BufferData& bufferData,
                              const Texture_ptr& depthBuffer,
                              const Camera& camera,
                              GFX::CommandBuffer& bufferInOut) {

    static Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._shaderProgramHandle = _HIZCullProgram->getGUID();
        pipeline = newPipeline(pipelineDescriptor);
    }

    constexpr U32 GROUP_SIZE_AABB = 64;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = "Occlusion Cull";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::BindPipelineCommand bindPipelineCmd = {};
    bindPipelineCmd._pipeline = pipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    ShaderBufferBinding shaderBuffer = {};
    shaderBuffer._binding = ShaderBufferLocation::GPU_COMMANDS;
    shaderBuffer._buffer = bufferData._cmdBuffer;
    shaderBuffer._elementRange.set(0, to_U16(bufferData._cmdBuffer->getPrimitiveCount()));

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    bindDescriptorSetsCmd._set.addShaderBuffer(shaderBuffer);

    if (bufferData._cullCounter != nullptr) {
        ShaderBufferBinding atomicCount = {};
        atomicCount._binding = ShaderBufferLocation::ATOMIC_COUNTER;
        atomicCount._buffer = bufferData._cullCounter;
        atomicCount._elementRange.set(0, 1);
        bindDescriptorSetsCmd._set.addShaderBuffer(atomicCount); // Atomic counter should be cleared by this point
    }

    bindDescriptorSetsCmd._set._textureData.setTexture(depthBuffer->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);
    
    U32 cmdCount = *bufferData._lastCommandCount;

    GFX::SendPushConstantsCommand HIZPushConstantsCMD;
    HIZPushConstantsCMD._constants.countHint(6);
    HIZPushConstantsCMD._constants.set("dvd_numEntities", GFX::PushConstantType::UINT, cmdCount);
    HIZPushConstantsCMD._constants.set("dvd_nearPlaneDistance", GFX::PushConstantType::FLOAT, camera.getZPlanes().x);
    HIZPushConstantsCMD._constants.set("viewportDimensions", GFX::PushConstantType::VEC2, vec2<F32>(depthBuffer->width(), depthBuffer->height()));
    HIZPushConstantsCMD._constants.set("viewMatrix", GFX::PushConstantType::MAT4, camera.getViewMatrix());
    HIZPushConstantsCMD._constants.set("projectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix());
    HIZPushConstantsCMD._constants.set("viewProjectionMatrix", GFX::PushConstantType::MAT4, mat4<F32>::Multiply(camera.getViewMatrix(), camera.getProjectionMatrix()));
    GFX::EnqueueCommand(bufferInOut, HIZPushConstantsCMD);

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set((cmdCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::EndDebugScopeCommand endDebugScopeCmd = {};
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void GFXDevice::drawText(const TextElementBatch& batch, GFX::CommandBuffer& bufferInOut) const {
    GFX::DrawTextCommand drawTextCommand;
    drawTextCommand._batch = batch;
    drawText(drawTextCommand, bufferInOut);
}

void GFXDevice::drawText(const GFX::DrawTextCommand& cmd, GFX::CommandBuffer& bufferInOut) const {
    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = _textRenderPipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _textRenderConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);
    
    GFX::EnqueueCommand(bufferInOut, cmd);
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

void GFXDevice::drawTextureInRenderWindow(TextureData data, GFX::CommandBuffer& bufferInOut) const {
    Rect<I32> drawRect(0, 0, 1, 1);

    if (Config::Build::ENABLE_EDITOR && context().editor().running()) {
        drawRect.zw(context().editor().getTargetViewport().zw());
    } else {
        drawRect.zw(context().app().windowManager().getMainWindow().getDimensions());
    }

    drawTextureInViewport(data, drawRect, bufferInOut);
}

void GFXDevice::drawTextureInViewport(TextureData data, const Rect<I32>& viewport, GFX::CommandBuffer& bufferInOut) const {
    static Pipeline* pipeline = nullptr;
    if (!pipeline) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _displayShader->getGUID();
        pipeline = newPipeline(pipelineDescriptor);
    }

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
    bindPipelineCmd._pipeline = pipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd;
    bindDescriptorSetsCmd._set._textureData.setTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    GFX::SetViewportCommand viewportCommand;
    viewportCommand._viewport.set(viewport);
    GFX::EnqueueCommand(bufferInOut, viewportCommand);

    // Blit render target to screen
    GFX::DrawCommand drawCmd = { triangleCmd };
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndDebugScopeCommand endDebugScopeCommand;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
}

void GFXDevice::blitToRenderTarget(RenderTargetID targetID, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = targetID;
    beginRenderPassCmd._name = "BLIT_TO_RENDER_TARGET";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    blitToBuffer(targetViewport, bufferInOut);

    GFX::EndRenderPassCommand endRenderPassCmd = {};
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

void GFXDevice::blitToBuffer(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 12345;
    beginDebugScopeCmd._scopeName = "Flush Display";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::ResolveRenderTargetCommand resolveCmd = { };
    resolveCmd._source = RenderTargetID(RenderTargetUsage::SCREEN);
    resolveCmd._resolveColours = true;
    resolveCmd._resolveDepth = false;
    GFX::EnqueueCommand(bufferInOut, resolveCmd);

    RenderTarget& screen = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    TextureData texData = screen.getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO)).texture()->getData();

    drawTextureInViewport(texData, targetViewport, bufferInOut);
    renderUI(targetViewport, bufferInOut);

    GFX::EndDebugScopeCommand endDebugScopeCommand = {};
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
}

void GFXDevice::renderUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 123456;
    beginDebugScopeCmd._scopeName = "Render GUI";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    _parent.platformContext().gui().draw(*this, bufferInOut);

    GFX::EndDebugScopeCommand endDebugScopeCommand = {};
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
}

void GFXDevice::renderDebugUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    constexpr I32 padding = 5;

    // Early out if we didn't request the preview
    if (_debugViewsEnabled) {

        GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
        beginDebugScopeCmd._scopeID = 1234567;
        beginDebugScopeCmd._scopeName = "Render Debug Views";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        renderDebugViews(
            Rect<I32>(targetViewport.x + padding,
                targetViewport.y + padding,
                targetViewport.z - padding * 2,
                targetViewport.w - padding * 2),
            bufferInOut);

        GFX::EndDebugScopeCommand endDebugScopeCommand = {};
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }
}


void GFXDevice::blurTarget(RenderTargetHandle& blurSource,
                           RenderTargetHandle& blurTargetH,
                           RenderTargetHandle& blurTargetV,
                           RTAttachmentType att,
                           U8 index,
                           I32 kernelSize,
                           GFX::CommandBuffer& bufferInOut) const
{

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _blurShader->getGUID();

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    // Blur horizontally
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = blurTargetH._targetID;
    beginRenderPassCmd._name = "BLUR_RENDER_TARGET_HORIZONTAL";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    TextureData data = blurSource._rt->getAttachment(att, index).texture()->getData();
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].push_back(_horizBlur);
    pipelineCmd._pipeline = newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants.set("kernelSize", GFX::PushConstantType::INT, kernelSize);
    pushConstantsCommand._constants.set("size", GFX::PushConstantType::VEC2, vec2<F32>(blurTargetH._rt->getWidth(), blurTargetH._rt->getHeight()));
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    GFX::DrawCommand drawCmd = { triangleCmd };
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    // Blur vertically
    beginRenderPassCmd._target = blurTargetV._targetID;
    beginRenderPassCmd._name = "BLUR_RENDER_TARGET_VERTICAL";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].front() = _vertBlur;
    pipelineCmd._pipeline = newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    pushConstantsCommand._constants.set("size", GFX::PushConstantType::VEC2, vec2<F32>(blurTargetV._rt->getWidth(), blurTargetV._rt->getHeight()));
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    data = blurTargetH._rt->getAttachment(att, index).texture()->getData();
    descriptorSetCmd._set._textureData.setTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

};
