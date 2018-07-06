#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

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
        _gfxDataBuffer->setData(&_gpuBlock._data);
        _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);
        _gpuBlock._needsUpload = false;
    }
}

void GFXDevice::renderQueueToSubPasses(RenderPassCmd& commandsInOut) {
    ReadLock lock(_renderQueueLock);
    for (RenderPackageQueue& renderQueue : _renderQueues) {
        if (!renderQueue.empty()) {
            U32 queueSize = renderQueue.size();
            for (U32 idx = 0; idx < queueSize; ++idx) {
                RenderSubPassCmd subPassCmd;
                RenderPackage& package = renderQueue.getPackage(idx);
                GenericDrawCommands& drawCommands = package._drawCommands;
                vectorAlg::vecSize commandCount = drawCommands.size();
                if (commandCount > 0) {
                    vectorAlg::vecSize previousCommandIndex = 0;
                    vectorAlg::vecSize currentCommandIndex = 1;
                    for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
                        GenericDrawCommand& previousCommand = drawCommands[previousCommandIndex];
                        GenericDrawCommand& currentCommand = drawCommands[currentCommandIndex];
                        if (!batchCommands(previousCommand, currentCommand))
                        {
                            previousCommandIndex = currentCommandIndex;
                        }
                    }

                    std::for_each(std::begin(drawCommands),
                                  std::end(drawCommands),
                                  [&](GenericDrawCommand& cmd) -> void {
                                      cmd.enableOption(GenericDrawCommand::RenderOptions::RENDER_INDIRECT);
                                  });

                    for (ShaderBufferList::value_type& it : package._shaderBuffers) {
                        subPassCmd._shaderBuffers.emplace_back(it._buffer, it._slot, it._range);
                    }

                    subPassCmd._textures.set(package._textureData);
                    subPassCmd._commands.insert(std::cbegin(subPassCmd._commands),
                                                std::cbegin(drawCommands),
                                                std::cend(drawCommands));
                    commandsInOut._subPassCmds.push_back(subPassCmd);
                }
            }
            renderQueue.clear();
        }
        renderQueue.unlock();
    }
}

void GFXDevice::flushCommandBuffer(const CommandBuffer& commandBuffer) {
    uploadGPUBlock();
    _api->flushCommandBuffer(commandBuffer);
}

void GFXDevice::addToRenderQueue(U32 queueIndex, const RenderPackage& package) {
    if (!package.isRenderable()) {
        return;
    }

    ReadLock lock(_renderQueueLock);
    assert(_renderQueues.size() > queueIndex);
    RenderPackageQueue& queue = _renderQueues[queueIndex];

    if (!queue.empty()) {
        RenderPackage& previous = queue.back();

        if (previous.isCompatible(package)) {
            previous._drawCommands.insert(std::cend(previous._drawCommands),
                                          std::cbegin(package._drawCommands),
                                          std::cend(package._drawCommands));
            return;
        }
    } else {
        queue.reserve(Config::MAX_VISIBLE_NODES);
    }

    queue.push_back(package);
}

I32 GFXDevice::reserveRenderQueue() {
    UpgradableReadLock ur_lock(_renderQueueLock);
    //ToDo: Nothing about this bloody thing is threadsafe
    I32 queueCount = to_int(_renderQueues.size());
    for (I32 i = 0; i < queueCount; ++i) {
        RenderPackageQueue& queue = _renderQueues[i];
        if (queue.empty() && !queue.locked()) {
            queue.lock();
            return i;
        }
    }

    UpgradeToWriteLock uw_lock(ur_lock);
    _renderQueues.emplace_back();
    _renderQueues[queueCount].lock();
    return queueCount;
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData& GFXDevice::processVisibleNode(const SceneGraphNode& node, U32 dataIndex) {
    NodeData& dataOut = _matricesData[dataIndex];

    RenderingComponent* const renderable = node.get<RenderingComponent>();
    AnimationComponent* const animComp = node.get<AnimationComponent>();
    PhysicsComponent* const transform = node.get<PhysicsComponent>();

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        dataOut._worldMatrix.set(transform->getWorldMatrix(getFrameInterpolationFactor()));

        mat4<F32> normalMatrix(dataOut._worldMatrix);
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            normalMatrix.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            normalMatrix.inverseTranspose();
            normalMatrix.mat[15] = 0.0f;
        }
        normalMatrix.setRow(3, 0.0f, 0.0f, 0.0f, 0.0f);

        // Calculate the normal matrix (world * view)
        mat4<F32>::Multiply(normalMatrix, getMatrix(MATRIX::VIEW), dataOut._normalMatrixWV);
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    dataOut._normalMatrixWV.element(0, 3) = to_float(animComp ? animComp->boneCount() : 0);
    dataOut._normalMatrixWV.setRow(3, node.get<BoundsComponent>()->getBoundingSphere().asVec4());
    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties, dataOut._normalMatrixWV.element(1, 3), dataOut._normalMatrixWV.element(2, 3));
    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    return dataOut;
}

void GFXDevice::buildDrawCommands(RenderPassCuller::VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  RenderPass::BufferData& bufferData,
                                  bool refreshNodeData)
{
    Time::ScopedTimer timer(_commandBuildTimer);
    // If there aren't any nodes visible in the current pass, don't update
    // anything (but clear the render queue

    U32 textureHandle = 0;
    U32 lastUnit0Handle = 0;
    U32 lastUnit1Handle = 0;
    U32 lastUsedSlot = 0;
    RenderStage currentStage = getRenderStage();
    if (refreshNodeData) {
        bufferData._lastCommandCount = 0;
        bufferData._lasNodeCount = 0;
    }

    if (currentStage == RenderStage::SHADOW) {
        Light* shadowLight = LightPool::currentShadowCastingLight();
        assert(shadowLight != nullptr);
        if (!COMPARE(_gpuBlock._data._renderProperties.x, shadowLight->getShadowProperties()._arrayOffset.x)) {
            _gpuBlock._data._renderProperties.x = to_float(shadowLight->getShadowProperties()._arrayOffset.x);
            _gpuBlock._needsUpload = true;
        }
        U8 shadowPasses = shadowLight->getLightType() == LightType::DIRECTIONAL
                                                       ? shadowLight->getShadowMapInfo()->numLayers()
                                                       : 1;
        if (!COMPARE(_gpuBlock._data._renderProperties.y, to_float(shadowPasses))) {
            _gpuBlock._data._renderProperties.y = to_float(shadowPasses);
            _gpuBlock._needsUpload = true;
        }
    }

    std::for_each(std::begin(visibleNodes), std::end(visibleNodes),
                 [&](RenderPassCuller::VisibleNode& node) -> void
    {

        const SceneGraphNode* nodeRef = node.second;
        RenderingComponent* renderable = nodeRef->get<RenderingComponent>();
        Attorney::RenderingCompGFXDevice::prepareDrawPackage(*renderable, sceneRenderState, currentStage);
    });

    U32 nodeCount = 0;
    U32 cmdCount = 0;
    std::for_each(std::begin(visibleNodes), std::end(visibleNodes),
        [&](RenderPassCuller::VisibleNode& node) -> void
    {
        const SceneGraphNode* nodeRef = node.second;

        RenderingComponent* renderable = nodeRef->get<RenderingComponent>();
        RenderPackage& pkg = Attorney::RenderingCompGFXDevice::getDrawPackage(*renderable,
                                                                               sceneRenderState,
                                                                               currentStage,
                                                                               refreshNodeData ? cmdCount
                                                                                               : renderable->commandOffset(),
                                                                               refreshNodeData ? nodeCount
                                                                                               : renderable->commandIndex());

        if (pkg.isRenderable()) {
            if (refreshNodeData) {
                NodeData& dataOut = processVisibleNode(*nodeRef, nodeCount);
                if (isDepthStage()) {
                    for (TextureData& data : pkg._textureData.textures()) {
                        if (data.getHandleLow() == to_const_uint(ShaderProgram::TextureUsage::UNIT0)) {
                            textureHandle = data.getHandleHigh();
                            if ((!(lastUnit0Handle == 0 || textureHandle == lastUnit0Handle) &&
                                  (lastUnit1Handle == 0 || textureHandle == lastUnit1Handle))                              
                                || (lastUsedSlot == 0 && lastUnit0Handle != 0))
                            {
                                data.setHandleLow(to_const_uint(ShaderProgram::TextureUsage::UNIT1));
                                    // Set this to 1 if we need to use texture UNIT1 instead of UNIT0 as the main texture
                                    dataOut._properties.w = 1;
                                    lastUnit1Handle = textureHandle;
                                    lastUsedSlot = 1;
                            } else {
                                lastUnit0Handle = textureHandle;
                                lastUsedSlot = 0;
                            }
                        }
                    }
                } else {
                    //set properties.w to -1 to skip occlusion culling for the node
                    dataOut._properties.w = pkg.isOcclusionCullable() ? 1.0f : -1.0f;
                }

                for (GenericDrawCommand& cmd : pkg._drawCommands) {
                    _drawCommandsCache[cmdCount++].set(cmd.cmd());
                }
            }
            nodeCount++;
        }
    });
    
    if (refreshNodeData) {
        bufferData._lastCommandCount = cmdCount;
        bufferData._lasNodeCount = nodeCount;

        assert(cmdCount >= nodeCount);
        // If the buffer update required is large enough, just replace the entire thing
        if (nodeCount > Config::MAX_VISIBLE_NODES / 2) {
            bufferData._renderData->setData(_matricesData.data());
        } else {
            // Otherwise, just update the needed range to save bandwidth
            bufferData._renderData->updateData(0, nodeCount, _matricesData.data());
        }

        ShaderBuffer& cmdBuffer = *bufferData._cmdBuffer;
        cmdBuffer.setData(_drawCommandsCache.data());
        _api->registerCommandBuffer(cmdBuffer);

        // This forces a sync for each buffer to make sure all data is properly uploaded in VRAM
        bufferData._renderData->bind(ShaderBufferLocation::NODE_INFO);
    }
}

void GFXDevice::occlusionCull(const RenderPass::BufferData& bufferData, const Texture_ptr& depthBuffer) {
    static const U32 GROUP_SIZE_AABB = 64;
    uploadGPUBlock();

    bufferData._cmdBuffer->bind(ShaderBufferLocation::GPU_COMMANDS);
    bufferData._cmdBuffer->bindAtomicCounter();

    depthBuffer->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH));
    U32 cmdCount = bufferData._lastCommandCount;

    _HIZCullProgram->bind();
    _HIZCullProgram->Uniform("dvd_numEntities", cmdCount);
    _HIZCullProgram->DispatchCompute((cmdCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    _HIZCullProgram->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::COUNTER);
}

U32 GFXDevice::getLastCullCount() const {
    const RenderPass::BufferData& bufferData = 
        parent().renderPassManager().getBufferData(RenderStage::DISPLAY, 0);

    U32 cullCount = bufferData._cmdBuffer->getAtomicCounter();
    if (cullCount > 0) {
        bufferData._cmdBuffer->resetAtomicCounter();
    }
    return cullCount;
}

bool GFXDevice::batchCommands(GenericDrawCommand& previousIDC,
                              GenericDrawCommand& currentIDC) const {
    if (previousIDC.compatible(currentIDC) &&
        // Batchable commands must share the same buffer
        previousIDC.sourceBuffer()->getGUID() ==
        currentIDC.sourceBuffer()->getGUID() &&
        // And the same shader program
        previousIDC.shaderProgram()->getID() ==
        currentIDC.shaderProgram()->getID())
    {
        U32 prevCount = previousIDC.drawCount();
        if (previousIDC.cmd().baseInstance + prevCount != currentIDC.cmd().baseInstance) {
            return false;
        }
        // If the rendering commands are batchable, increase the draw count for
        // the previous one
        previousIDC.drawCount(to_ushort(prevCount + currentIDC.drawCount()));
        // And set the current command's draw count to zero so it gets removed
        // from the list later on
        currentIDC.drawCount(0);

        return true;
    }

    return false;
}

bool GFXDevice::draw(const GenericDrawCommand& cmd) {
    uploadGPUBlock();
    if (_api->draw(cmd)) {
        if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY)) {
            registerDrawCall();
        }
        if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME)) {
            registerDrawCall();
        }
        return true;
    }

    return false;
}


void GFXDevice::flushDisplay(const vec4<I32>& targetViewport) {
    RenderTarget& screen = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    screen.bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0),
                RTAttachment::Type::Colour,
                to_const_ubyte(ScreenTargets::ALBEDO));


    GFX::ScopedViewport targetArea(*this, targetViewport);

    // Blit render target to screen
    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_displayShader);
    draw(triangleCmd);


    // Render all 2D debug info and call API specific flush function
    if (Application::instance().mainLoopActive()) {
        GFX::Scoped2DRendering scoped2D(*this);
        ReadLock r_lock(_2DRenderQueueLock);
        for (std::pair<U32, GUID2DCbk>& callbackFunction : _2dRenderQueue) {
            callbackFunction.second.second();
        }
    }
}

};
