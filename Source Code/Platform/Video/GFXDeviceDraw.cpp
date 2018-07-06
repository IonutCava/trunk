#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Platform/Video/Headers/IMPrimitive.h"
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
    // Get the color matrix (diffuse, specular, etc.)
    renderable->getMaterialColorMatrix(dataOut._colorMatrix);
    // Get the material property matrix (alpha test, texture count,
    // texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties);

    return dataOut;
}

void GFXDevice::buildDrawCommands(RenderPassCuller::VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  bool refreshNodeData,
                                  U32 pass)
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
        _lastCommandCount[getNodeBufferIndexForStage(currentStage)] = 0;
        _lastNodeCount[getNodeBufferIndexForStage(currentStage)] = 0;
    }

    if (currentStage == RenderStage::SHADOW) {
        Light* shadowLight = LightPool::currentShadowCastingLight();
        assert(shadowLight != nullptr);
        if (_gpuBlock._data._renderProperties.x != shadowLight->getShadowProperties()._arrayOffset.x) {
            _gpuBlock._data._renderProperties.x = to_float(shadowLight->getShadowProperties()._arrayOffset.x);
            _gpuBlock._updated = true;
        }
        _gpuBlock._data._renderProperties.y = 
            to_float(shadowLight->getLightType() == LightType::DIRECTIONAL
                                                  ? shadowLight->getShadowMapInfo()->numLayers()
                                                  : 1);
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
        _lastCommandCount[getNodeBufferIndexForStage(currentStage)] = cmdCount;
        _lastNodeCount[getNodeBufferIndexForStage(currentStage)] = nodeCount;
        assert(cmdCount >= nodeCount);
        getNodeBuffer(currentStage, pass).setData(_matricesData.data());

        ShaderBuffer& cmdBuffer = getCommandBuffer(currentStage, pass);
        cmdBuffer.setData(_drawCommandsCache.data());
        _api->registerCommandBuffer(cmdBuffer);

        // This forces a sync for each buffer to make sure all data is properly uploaded in VRAM
        getNodeBuffer(currentStage, pass).bind(ShaderBufferLocation::NODE_INFO);
    }
}

void GFXDevice::occlusionCull(U32 pass) {
    static const U32 GROUP_SIZE_AABB = 64;
    uploadGPUBlock();

    RenderStage currentStage = getRenderStage();
    getCommandBuffer(currentStage, pass).bind(ShaderBufferLocation::GPU_COMMANDS);
    getCommandBuffer(currentStage, pass).bindAtomicCounter();

    Framebuffer* screenTarget = _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._buffer;

    screenTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), TextureDescriptor::AttachmentType::Depth);

    U32 cmdCount = _lastCommandCount[getNodeBufferIndexForStage(currentStage)];
    _HIZCullProgram->bind();
    _HIZCullProgram->Uniform("dvd_numEntities", cmdCount);
    _HIZCullProgram->DispatchCompute((cmdCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    _HIZCullProgram->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::COUNTER);
}

U32 GFXDevice::getLastCullCount(U32 pass) const {
    U32 cullCount = getCommandBuffer(getRenderStage(), pass).getAtomicCounter();
    if (cullCount > 0) {
        getCommandBuffer(getRenderStage(), pass).resetAtomicCounter();
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

/// This is just a short-circuit system (hack) to send a list of points to the shader
/// It's used, mostly, to draw full screen quads using geometry shaders
void GFXDevice::drawPoints(U32 numPoints, size_t stateHash, const ShaderProgram_ptr& shaderProgram) {
    static const char* msgUnderflow = Locale::get(_ID("ERROR_GFX_POINTS_UNDERFLOW"));
    static const char* msgOverflow = Locale::get(_ID("ERROR_GFX_POINTS_OVERFLOW"));
    // We need a valid amount of points. Check lower limit
    DIVIDE_ASSERT(numPoints > 0, msgUnderflow);
    // Also check upper limit
    DIVIDE_ASSERT(numPoints <= Config::MAX_POINTS_PER_BATCH, msgOverflow);

    // We require a state hash value to set proper states
    _defaultDrawCmd.stateHash(stateHash);
    // We also require a shader program (although it may already be bound.
    // Better safe ...)
    _defaultDrawCmd.shaderProgram(shaderProgram);
    // If the draw command was successfully parsed
    if (setBufferData(_defaultDrawCmd)) {
        // Tell the rendering API to upload the needed number of points
        drawPoints(numPoints);
    }
}

/// This is just a short-circuit system (hack) to quickly send 3 vertices to the shader
/// It's used, mostly, to draw full screen quads using vertex shaders
void GFXDevice::drawTriangle(size_t stateHash, const ShaderProgram_ptr& shaderProgram) {
    // We require a state hash value to set proper states
    _defaultDrawCmd.stateHash(stateHash);
    // We also require a shader program (although it may already be bound. Better safe ...)
    _defaultDrawCmd.shaderProgram(shaderProgram);
    // If the draw command was successfully parsed
    if (setBufferData(_defaultDrawCmd)) {
        // Tell the rendering API to upload the needed number of points
        drawTriangle();
    }

}

void GFXDevice::drawSphere3D(IMPrimitive& primitive,
                             const vec3<F32>& center,
                             F32 radius,
                             const vec4<U8>& color) {

    U32 slices = 8, stacks = 8;
    F32 drho = to_float(M_PI) / stacks;
    F32 dtheta = 2.0f * to_float(M_PI) / slices;
    F32 ds = 1.0f / slices;
    F32 dt = 1.0f / stacks;
    F32 t = 1.0f;
    F32 s = 0.0f;
    U32 i, j;  // Looping variables
    primitive.paused(false);
    // Create the object
    primitive.beginBatch(true, stacks * ((slices + 1) * 2), 1);
    primitive.attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(color));
    primitive.begin(PrimitiveType::LINE_LOOP);
    for (i = 0; i < stacks; i++) {
        F32 rho = i * drho;
        F32 srho = std::sin(rho);
        F32 crho = std::cos(rho);
        F32 srhodrho = std::sin(rho + drho);
        F32 crhodrho = std::cos(rho + drho);
        s = 0.0f;
        for (j = 0; j <= slices; j++) {
            F32 theta = (j == slices) ? 0.0f : j * dtheta;
            F32 stheta = -std::sin(theta);
            F32 ctheta = std::cos(theta);

            F32 x = stheta * srho;
            F32 y = ctheta * srho;
            F32 z = crho;
            primitive.vertex((x * radius) + center.x,
                             (y * radius) + center.y,
                             (z * radius) + center.z);
            x = stheta * srhodrho;
            y = ctheta * srhodrho;
            z = crhodrho;
            s += ds;
            primitive.vertex((x * radius) + center.x,
                             (y * radius) + center.y,
                             (z * radius) + center.z);
        }
        t -= dt;
    }
    primitive.end();
    primitive.endBatch();
}

/// Draw the outlines of a box defined by min and max as extents using the
/// specified world matrix
void GFXDevice::drawBox3D(IMPrimitive& primitive,
                          const vec3<F32>& min,
                          const vec3<F32>& max,
                          const vec4<U8>& color)  {
    primitive.paused(false);
    // Create the object
    primitive.beginBatch(true, 16, 1);
    // Set it's color
    primitive.attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(color));
    // Draw the bottom loop
    primitive.begin(PrimitiveType::LINE_LOOP);
    primitive.vertex(min.x, min.y, min.z);
    primitive.vertex(max.x, min.y, min.z);
    primitive.vertex(max.x, min.y, max.z);
    primitive.vertex(min.x, min.y, max.z);
    primitive.end();
    // Draw the top loop
    primitive.begin(PrimitiveType::LINE_LOOP);
    primitive.vertex(min.x, max.y, min.z);
    primitive.vertex(max.x, max.y, min.z);
    primitive.vertex(max.x, max.y, max.z);
    primitive.vertex(min.x, max.y, max.z);
    primitive.end();
    // Connect the top to the bottom
    primitive.begin(PrimitiveType::LINES);
    primitive.vertex(min.x, min.y, min.z);
    primitive.vertex(min.x, max.y, min.z);
    primitive.vertex(max.x, min.y, min.z);
    primitive.vertex(max.x, max.y, min.z);
    primitive.vertex(max.x, min.y, max.z);
    primitive.vertex(max.x, max.y, max.z);
    primitive.vertex(min.x, min.y, max.z);
    primitive.vertex(min.x, max.y, max.z);
    primitive.end();
    // Finish our object
    primitive.endBatch();
}

/// Render a list of lines within the specified constraints
void GFXDevice::drawLines(IMPrimitive& primitive,
                          const vectorImpl<Line>& lines,
                          const vec4<I32>& viewport,
                          const bool inViewport) {

    static const vec3<F32> vertices[] = {
        vec3<F32>(-1.0f, -1.0f,  1.0f),
        vec3<F32>(1.0f, -1.0f,  1.0f),
        vec3<F32>(-1.0f,  1.0f,  1.0f),
        vec3<F32>(1.0f,  1.0f,  1.0f),
        vec3<F32>(-1.0f, -1.0f, -1.0f),
        vec3<F32>(1.0f, -1.0f, -1.0f),
        vec3<F32>(-1.0f,  1.0f, -1.0f),
        vec3<F32>(1.0f,  1.0f, -1.0f)
    };

    static const U16 indices[] = {
        0, 1, 2,
        3, 7, 1,
        5, 4, 7,
        6, 2, 4,
        0, 1
    };
    // Check if we have a valid list. The list can be programmatically
    // generated, so this check is required
    if (!lines.empty()) {
        vec4<F32> tempFloatColor;
        primitive.paused(false);
        // If we need to render it into a specific viewport, set the pre and post
        // draw functions to set up the
        // needed viewport rendering (e.g. axis lines)
        if (inViewport) {
            primitive.setRenderStates(
                [&, viewport]() {
                    setViewport(viewport);
                },
                [&]() {
                    restoreViewport();
                });
        }
        // Create the object containing all of the lines
        primitive.beginBatch(true, to_uint(lines.size()) * 2 * 14, 1);
        Util::ToFloatColor(lines[0]._colorStart, tempFloatColor);
        primitive.attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
        // Set the mode to line rendering
        //primitive.begin(PrimitiveType::TRIANGLE_STRIP);
        primitive.begin(PrimitiveType::LINES);
        primitive.drawShader(_imShaderLines.get());
        //vec3<F32> tempVertex;
        // Add every line in the list to the batch
        for (const Line& line : lines) {
            Util::ToFloatColor(line._colorStart, tempFloatColor);
            primitive.attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
            /*for (U16 idx : indices) {
                tempVertex.set(line._startPoint * vertices[idx]);
                tempVertex *= line._widthStart;

                primitive.vertex(tempVertex);
            }*/
            primitive.vertex(line._startPoint);

            Util::ToFloatColor(line._colorEnd, tempFloatColor);
            primitive.attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
            /*for (U16 idx : indices) {
                tempVertex.set(line._endPoint * vertices[idx]);
                tempVertex *= line._widthEnd;

                primitive.vertex(tempVertex);
            }*/

            primitive.vertex(line._endPoint);
            
        }
        primitive.end();
        // Finish our object
        primitive.endBatch();
    }

}

void GFXDevice::flushDisplay() {
    activeRenderTarget().bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));
    drawTriangle(getDefaultStateBlock(true), _displayShader);
}

};
