#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

// ToDo: This will return false if the number of shader buffers or number of
// textures does not match between the 2 packages although said buffers/textures
// might be compatible and batchable between the two.
// Obviously, this is not desirable. Fix it! -Ionut
bool GFXDevice::RenderPackage::isCompatible(const RenderPackage& other) const {
    vectorAlg::vecSize bufferCount = other._shaderBuffers.size();
    if (_shaderBuffers.size() == bufferCount) {
        for (vectorAlg::vecSize i = 0; i < bufferCount; i++) {
            const GFXDevice::ShaderBufferList::value_type& buffer1 =
                _shaderBuffers[i];
            const GFXDevice::ShaderBufferList::value_type& buffer2 =
                other._shaderBuffers[i];


            I64 guid1 = buffer1._buffer ? buffer1._buffer->getGUID() : -1;
            I64 guid2 = buffer2._buffer ? buffer2._buffer->getGUID() : -1;
            if (buffer1._slot != buffer2._slot ||
                buffer1._range != buffer2._range ||
                guid1 != guid2) 
            {
                return false;
            }
        }
    } else {
        return false;
    }

    vectorAlg::vecSize textureCount = other._textureData.size();
    if (_textureData.size() == textureCount) {
        U64 handle1 = 0, handle2 = 0;
        for (vectorAlg::vecSize i = 0; i < textureCount; ++i) {
            const TextureData& data1 = _textureData[i];
            const TextureData& data2 = other._textureData[i];
            data1.getHandle(handle1);
            data2.getHandle(handle2);
            if (handle1 != handle2 ||
                data1._samplerHash != data2._samplerHash ||
                data1._textureType != data2._textureType) {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

void GFXDevice::uploadGPUBlock() {
    if (_gpuBlock._updated) {
        // We flush the entire buffer on update to inform the GPU that we don't
        // need the previous data. Might avoid some driver sync
        _gfxDataBuffer->setData(&_gpuBlock._data);
        _gpuBlock._updated = false;
    }
}

/// A draw command is composed of a target buffer and a command. The command
/// part is processed here
bool GFXDevice::setBufferData(const GenericDrawCommand& cmd) {
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(cmd.shaderProgram() != nullptr,
                  "GFXDevice error: Draw shader state is not valid for the "
                  "current draw operation!");

    // Set the proper render states
    setStateBlock(cmd.stateHash());

    // Try to bind the shader program. If it failed to load, or isn't loaded
    // yet, cancel the draw request for this frame
    return cmd.shaderProgram()->bind();
}

void GFXDevice::submitCommand(const GenericDrawCommand& cmd, bool useIndirectRender) {
    // We may choose the instance count programmatically, and it may turn out to be 0, so skip draw
    if (setBufferData(cmd)) {
        /// Submit a single draw command
        DIVIDE_ASSERT(cmd.sourceBuffer() != nullptr, "GFXDevice error: Invalid vertex buffer submitted!");
        // Same rules about pre-processing the draw command apply
        cmd.sourceBuffer()->draw(cmd, useIndirectRender);
        if (cmd.renderGeometry()) {
            registerDrawCall();
        }
        if (cmd.renderWireframe()) {
            registerDrawCall();
        }
    }
}

void GFXDevice::flushRenderQueues() {
    uploadGPUBlock();

    ReadLock lock(_renderQueueLock);
    for (RenderQueue& renderQueue : _renderQueues) {
        if (!renderQueue.empty()) {
            U32 queueSize = renderQueue.size();
            for (U32 idx = 0; idx < queueSize; ++idx) {
                RenderPackage& package = renderQueue.getPackage(idx);
                vectorImpl<GenericDrawCommand>& drawCommands = package._drawCommands;
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
                    for (ShaderBufferList::value_type& it : package._shaderBuffers) {
                        it._buffer->bindRange(it._slot, it._range.x, it._range.y);
                    }

                    _api->makeTexturesResident(package._textureData);
                    submitCommands(package._drawCommands, true);
                }
            }

            renderQueue.clear();
        }
        renderQueue.unlock();
    }
}

void GFXDevice::addToRenderQueue(U32 queueIndex, const RenderPackage& package) {
    ReadLock lock(_renderQueueLock);
    assert(_renderQueues.size() > queueIndex);

    if (!package.isRenderable()) {
        return;
    }

    RenderQueue& queue = _renderQueues[queueIndex];

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
        RenderQueue& queue = _renderQueues[i];
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
        dataOut._worldMatrix.set(transform->getWorldMatrix(_interpolationFactor));

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
        mat4<F32>::Multiply(normalMatrix, _gpuBlock._data._ViewMatrix, dataOut._normalMatrixWV);
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    dataOut._normalMatrixWV.element(0, 3) = to_float(animComp ? animComp->boneCount() : 0);
    dataOut._normalMatrixWV.setRow(3, node.get<BoundsComponent>()->getBoundingSphere().asVec4());
    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);
    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties, dataOut._extraProperties);
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
    RenderStage currentStage = isPrePass() ? RenderStage::Z_PRE_PASS : getRenderStage();
    if (refreshNodeData) {
        bufferData._lastCommandCount = 0;
        bufferData._lasNodeCount = 0;
    }

    if (currentStage == RenderStage::SHADOW) {
        Light* shadowLight = LightPool::currentShadowCastingLight();
        assert(shadowLight != nullptr);
        if (!COMPARE(_gpuBlock._data._renderProperties.x, shadowLight->getShadowProperties()._arrayOffset.x)) {
            _gpuBlock._data._renderProperties.x = to_float(shadowLight->getShadowProperties()._arrayOffset.x);
            _gpuBlock._updated = true;
        }
        U8 shadowPasses = shadowLight->getLightType() == LightType::DIRECTIONAL
                                                       ? shadowLight->getShadowMapInfo()->numLayers()
                                                       : 1;
        if (!COMPARE(_gpuBlock._data._renderProperties.y, to_float(shadowPasses))) {
            _gpuBlock._data._renderProperties.y = to_float(shadowPasses);
            _gpuBlock._updated = true;
        }
    }

    U32 nodeCount = 0;
    U32 cmdCount = 0;
    std::for_each(std::begin(visibleNodes), std::end(visibleNodes),
        [&](RenderPassCuller::VisibleNode& node) -> void {
        SceneGraphNode_cptr nodeRef = node.second.lock();

        RenderingComponent* renderable = nodeRef->get<RenderingComponent>();
        RenderPackage& pkg = 
            refreshNodeData ? Attorney::RenderingCompGFXDevice::getDrawPackage(*renderable,
                                                                               sceneRenderState,
                                                                               currentStage,
                                                                               cmdCount,
                                                                               nodeCount)
                            : Attorney::RenderingCompGFXDevice::getDrawPackage(*renderable,
                                                                               sceneRenderState,
                                                                               currentStage);

        if (pkg.isRenderable()) {
            if (refreshNodeData) {
                NodeData& dataOut = processVisibleNode(*nodeRef, nodeCount);
                if (isDepthStage()) {
                    for (TextureData& data : pkg._textureData) {
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
        bufferData._renderData->setData(_matricesData.data());

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
        RenderPassManager::instance().getBufferData(RenderStage::DISPLAY, 0);

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

void GFXDevice::draw(const GenericDrawCommand& cmd) {
    if (setBufferData(cmd)) {
        uploadGPUBlock();
        _api->draw(cmd);
        if (cmd.renderGeometry()) {
            registerDrawCall();
        }
        if (cmd.renderWireframe()) {
            registerDrawCall();
        }
    }
}


void GFXDevice::flushDisplay() {
    activeRenderTarget().bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_displayShader);
    draw(triangleCmd);
}

};
