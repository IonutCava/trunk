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
        _api->updateClipPlanes();
    }
}

void GFXDevice::renderQueueToSubPasses(RenderBinType queueType, GFX::CommandBuffer& subPassCmd) {
    RenderPackageQueue& renderQueue = *_renderQueues[queueType._to_integral()].get();

    assert(renderQueue.locked() == false);
    if (!renderQueue.empty()) {
        U32 queueSize = renderQueue.size();
        for (U32 idx = 0; idx < queueSize; ++idx) {
            subPassCmd.add(renderQueue.getCommandBuffer(idx));
        }
    }
}


void GFXDevice::clearRenderQueue(RenderBinType queueType) {
    _renderQueues[queueType._to_integral()]->clear();
}

void GFXDevice::flushCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    commandBuffer.batch();
    if (commandBuffer.validate()) {
        _api->flushCommandBuffer(commandBuffer);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_GFX_INVALID_COMMAND_BUFFER")), commandBuffer.toString().c_str());
        DIVIDE_ASSERT(false, "GFXDevice::flushCommandBuffer error: Invalid command buffer. Check error log!");
    }
}

void GFXDevice::flushAndClearCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    flushCommandBuffer(commandBuffer);
    commandBuffer.clear();
}

void GFXDevice::lockQueue(RenderBinType type) {
    _renderQueues[type._to_integral()]->lock();
}

void GFXDevice::unlockQueue(RenderBinType type) {
    _renderQueues[type._to_integral()]->unlock();
}

U32 GFXDevice::renderQueueSize(RenderBinType queueType) {
    U32 queueIndex = queueType._to_integral();
    assert(_renderQueues[queueIndex]->locked() == false);

    return _renderQueues[queueIndex]->size();
}

void GFXDevice::addToRenderQueue(RenderBinType queueType, const RenderPackage& package) {
    if (!package.isRenderable()) {
        return;
    }

    U32 queueIndex = queueType._to_integral();

    assert(_renderQueues[queueIndex]->locked() == true);

    _renderQueues[queueIndex]->push_back(package);
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData& GFXDevice::processVisibleNode(const SceneGraphNode& node, U32 dataIndex, const Camera& camera, const SceneRenderState& renderState, bool isOcclusionCullable) {
    NodeData& dataOut = _matricesData[dataIndex];

    RenderingComponent* const renderable = node.get<RenderingComponent>();
    TransformComponent* const transform  = node.get<TransformComponent>();

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        if (Config::USE_FIXED_TIMESTEP) {
            dataOut._worldMatrix.set(transform->getWorldMatrix(getFrameInterpolationFactor()));
        } else {
            dataOut._worldMatrix.set(transform->getWorldMatrix());
        }

        dataOut._normalMatrixWV.set(dataOut._worldMatrix);
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            dataOut._normalMatrixWV.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            dataOut._normalMatrixWV.inverseTranspose();
            dataOut._normalMatrixWV.mat[15] = 0.0f;
        }
        dataOut._normalMatrixWV.setRow(3, 0.0f, 0.0f, 0.0f, 0.0f);

        // Calculate the normal matrix (world * view)
        dataOut._normalMatrixWV *= camera.getViewMatrix();
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    if (renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS)) {
        AnimationComponent* const animComp = node.get<AnimationComponent>();
        dataOut._normalMatrixWV.element(0, 3) = to_F32(animComp && animComp->playAnimations() ? animComp->boneCount() : 0);
    } else {
        dataOut._normalMatrixWV.element(0, 3) = 0.0f;
    }
    dataOut._normalMatrixWV.setRow(3, node.get<BoundsComponent>()->getBoundingSphere().asVec4());
    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties, dataOut._normalMatrixWV.element(1, 3), dataOut._normalMatrixWV.element(2, 3));
    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    //set properties.w to -1 to skip occlusion culling for the node
    dataOut._properties.w = isOcclusionCullable ? 1.0f : -1.0f;

    return dataOut;
}

void GFXDevice::buildDrawCommands(const BuildDrawCommandsParams& params)
{
    Time::ScopedTimer timer(_commandBuildTimer);

    RenderPass::BufferData bufferData = *params._bufferData;

    if (params._refreshNodeData) {
        bufferData._lastCommandCount = 0;
        bufferData._lasNodeCount = 0;
    }

    if (params._renderStagePass._stage == RenderStage::SHADOW) {
        Light* shadowLight = LightPool::currentShadowCastingLight();
        assert(shadowLight != nullptr);
        if (!COMPARE(_gpuBlock._data._renderProperties.x, shadowLight->getShadowProperties()._arrayOffset.x)) {
            _gpuBlock._data._renderProperties.x = to_F32(shadowLight->getShadowProperties()._arrayOffset.x);
            _gpuBlock._needsUpload = true;
        }
        U8 shadowPasses = shadowLight->getLightType() == LightType::DIRECTIONAL
                                                       ? shadowLight->getShadowMapInfo()->numLayers()
                                                       : 1;
        if (!COMPARE(_gpuBlock._data._renderProperties.y, to_F32(shadowPasses))) {
            _gpuBlock._data._renderProperties.y = to_F32(shadowPasses);
            _gpuBlock._needsUpload = true;
        }
    }

    U32 nodeCount = 0;
    U32 cmdCount = 0;

    const RenderQueue::SortedQueues& sortedQueues = *params._sortedQueues;
    for (const vectorEASTL<SceneGraphNode*>& queue : sortedQueues) {
        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();
            Attorney::RenderingCompGFXDevice::prepareDrawPackage(renderable, *params._camera, *params._sceneRenderState, params._renderStagePass);
        }

        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();

            const RenderPackage& pkg = Attorney::RenderingCompGFXDevice::getDrawPackage(renderable, params._renderStagePass);
            if (pkg.isRenderable()) {
                if (params._refreshNodeData) {
                    Attorney::RenderingCompGFXDevice::setDrawIDs(renderable, params._renderStagePass, cmdCount, nodeCount);

                    processVisibleNode(*node, nodeCount, *params._camera, *params._sceneRenderState, pkg.isOcclusionCullable());

                    for (I32 cmdIdx = 0; cmdIdx < pkg.drawCommandCount(); ++cmdIdx) {
                        const GFX::DrawCommand& cmd = pkg.drawCommand(cmdIdx);
                        for (const GenericDrawCommand& drawCmd : cmd._drawCommands) {
                            for (U32 i = 0; i < drawCmd._drawCount; ++i) {
                                _drawCommandsCache[cmdCount++] = drawCmd._cmd;
                            }
                        }
                    }
                }
                nodeCount++;
            }
        }
    }

    if (params._refreshNodeData) {
        bufferData._lastCommandCount = cmdCount;
        bufferData._lasNodeCount = nodeCount;

        assert(cmdCount >= nodeCount);
        // If the buffer update required is large enough, just replace the entire thing
        if (nodeCount > Config::MAX_VISIBLE_NODES / 2) {
            bufferData._renderData->writeData(_matricesData.data());
        } else {
            // Otherwise, just update the needed range to save bandwidth
            bufferData._renderData->writeData(0, nodeCount, _matricesData.data());
        }

        ShaderBuffer& cmdBuffer = *bufferData._cmdBuffer;
        cmdBuffer.writeData(_drawCommandsCache.data());
        _api->registerCommandBuffer(cmdBuffer);

        // This forces a sync for each buffer to make sure all data is properly uploaded in VRAM
        bufferData._renderData->bind(ShaderBufferLocation::NODE_INFO);
    }
}

void GFXDevice::occlusionCull(const RenderPass::BufferData& bufferData,
                              const Texture_ptr& depthBuffer,
                              GFX::CommandBuffer& bufferInOut) {

    static const U32 GROUP_SIZE_AABB = 64;

    GFX::BindPipelineCommand bindPipelineCmd;
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._shaderProgramHandle = _HIZCullProgram->getID();
    bindPipelineCmd._pipeline = newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    ShaderBufferBinding shaderBuffer;
    shaderBuffer._binding = ShaderBufferLocation::GPU_COMMANDS;
    shaderBuffer._buffer = bufferData._cmdBuffer;
    shaderBuffer._range.set(0, to_U16(bufferData._cmdBuffer->getPrimitiveCount()));
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
}

U32 GFXDevice::getLastCullCount() const {
    const RenderPass::BufferData& bufferData = parent().renderPassManager().getBufferData(RenderStage::DISPLAY, 0);

    U32 cullCount = bufferData._cmdBuffer->getAtomicCounter();
    if (cullCount > 0) {
        bufferData._cmdBuffer->resetAtomicCounter();
    }
    return cullCount;
}

void GFXDevice::drawText(const TextElementBatch& batch, GFX::CommandBuffer& bufferInOut) const {
    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = _textRenderPipeline;

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _textRenderConstants;

    GFX::DrawTextCommand drawTextCommand;
    drawTextCommand._batch = batch;

    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._camera = Camera::utilityCamera(Camera::UtilityCamera::_2D);
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
    setCameraCommand._camera = Camera::utilityCamera(Camera::UtilityCamera::_2D);
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

    blitToBuffer(buffer, targetViewport);
    
    flushCommandBuffer(buffer);
}

void GFXDevice::blitToRenderTarget(RenderTargetID targetID, const Rect<I32>& targetViewport) {
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    GFX::CommandBuffer& buffer = sBuffer();

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = targetID;
    beginRenderPassCmd._name = "BLIT_TO_RENDER_TARGET";
    GFX::EnqueueCommand(buffer, beginRenderPassCmd);

    blitToBuffer(buffer, targetViewport);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(buffer, endRenderPassCmd);

    flushCommandBuffer(buffer);
}

void GFXDevice::blitToBuffer(GFX::CommandBuffer& bufferInOut, const Rect<I32>& targetViewport) {
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
