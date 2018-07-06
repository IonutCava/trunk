#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ProfileTimer.h"
#include "Core/Headers/ApplicationTimer.h"
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

void GFXDevice::processCommand(const GenericDrawCommand& cmd, bool useIndirectRender) {
    /// Submit a single draw command
    DIVIDE_ASSERT(cmd.sourceBuffer() != nullptr,
                  "GFXDevice error: Invalid vertex buffer submitted!");
    // We may choose the instance count programmatically, and it may turn out to
    // be 0, so skip draw
    if (setBufferData(cmd)) {
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

/// Submit multiple draw commands that use the same source buffer (e.g. terrain
/// or batched meshes)
void GFXDevice::processCommands(const vectorImpl<GenericDrawCommand>& cmds, bool useIndirectRender) {
    for (const GenericDrawCommand& cmd : cmds) {
        // Data validation is handled in the single command version
        processCommand(cmd, useIndirectRender);
    }
}

void GFXDevice::flushRenderQueue() {
    if (_renderQueue.empty()) {
        return;
    }

    uploadGPUBlock();

    U32 queueSize = _renderQueue.size();
    for (U32 idx = 0; idx < queueSize; ++idx) {
        RenderPackage& package = _renderQueue.getPackage(idx);
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
            submitIndirectRenderCommands(package._drawCommands);
        }
    }

    _renderQueue.clear();
}

void GFXDevice::addToRenderQueue(U32 binPropertyMask, const RenderPackage& package) {
    if (!package.isRenderable()) {
        return;
    }

    bool isTranslucent = BitCompare(binPropertyMask, to_uint(RenderBitProperty::TRANSLUCENT));
    ACKNOWLEDGE_UNUSED(isTranslucent);

    if (!_renderQueue.empty()) {
        RenderPackage& previous = _renderQueue.back();

        if (previous.isCompatible(package)) {
            previous._drawCommands.insert(std::cend(previous._drawCommands),
                                          std::cbegin(package._drawCommands),
                                          std::cend(package._drawCommands));
            return;
        }
    } else {
        _renderQueue.resize(RenderPassManager::getInstance().getQueue().getRenderQueueStackSize());
    }
    _renderQueue.push_back(package);
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData& GFXDevice::processVisibleNode(SceneGraphNode_wptr node, U32 dataIndex) {
    NodeData& dataOut = _matricesData[dataIndex];

    SceneGraphNode_ptr nodePtr = node.lock();
    assert(nodePtr);

    RenderingComponent* const renderable = nodePtr->getComponent<RenderingComponent>();
    AnimationComponent* const animComp = nodePtr->getComponent<AnimationComponent>();
    PhysicsComponent* const transform = nodePtr->getComponent<PhysicsComponent>();

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
        Util::Mat4::Multiply(normalMatrix, _gpuBlock._data._ViewMatrix, dataOut._normalMatrixWV);
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    dataOut._normalMatrixWV.element(0, 3) = to_float(animComp ? animComp->boneCount() : 0);
    dataOut._normalMatrixWV.setRow(3, nodePtr->getBoundingSphereConst().asVec4());
    // Get the color matrix (diffuse, specular, etc.)
    renderable->getMaterialColorMatrix(dataOut._colorMatrix);
    // Get the material property matrix (alpha test, texture count,
    // texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties);

    return dataOut;
}

void GFXDevice::buildDrawCommands(VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  bool refreshNodeData,
                                  U32 pass) {
    Time::ScopedTimer timer(*_commandBuildTimer);
    // If there aren't any nodes visible in the current pass, don't update
    // anything (but clear the render queue
    if (refreshNodeData) {
        _lastCommandCount = _lastNodeCount = 0;
    }

    U32 textureHandle = 0;
    U32 lastUnit0Handle = 0;
    U32 lastUnit1Handle = 0;
    U32 lastUsedSlot = 0;
    RenderStage currentStage = getRenderStage();

    if (currentStage == RenderStage::SHADOW) {
        Light* shadowLight = LightManager::getInstance().currentShadowCastingLight();
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
        [&](GFXDevice::VisibleNodeList::value_type& node) -> void {
        SceneGraphNode_ptr nodeRef = node.lock();

        RenderingComponent* renderable = nodeRef->getComponent<RenderingComponent>();
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
                NodeData& dataOut = processVisibleNode(node, nodeCount);
                if (isDepthStage()) {
                    for (TextureData& data : pkg._textureData) {
                        if (data.getHandleLow() == to_uint(ShaderProgram::TextureUsage::UNIT0)) {
                            textureHandle = data.getHandleHigh();
                            if ((!(lastUnit0Handle == 0 || textureHandle == lastUnit0Handle) &&
                                  (lastUnit1Handle == 0 || textureHandle == lastUnit1Handle))                              
                                || (lastUsedSlot == 0 && lastUnit0Handle != 0))
                            {
                                data.setHandleLow(to_uint(ShaderProgram::TextureUsage::UNIT1));
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
        _lastCommandCount = cmdCount;
        _lastNodeCount = nodeCount;
        assert(_lastCommandCount >= _lastNodeCount);
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

    Framebuffer* screenTarget = _renderTarget[anaglyphEnabled()
                                               ? to_uint(RenderTargetID::ANAGLYPH)
                                               : to_uint(RenderTargetID::SCREEN)]._buffer;
    screenTarget->bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH),
                       TextureDescriptor::AttachmentType::Depth);

    _HIZCullProgram->bind();
    _HIZCullProgram->Uniform("dvd_numEntities", _lastCommandCount);
    _HIZCullProgram->DispatchCompute((_lastCommandCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
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
    DIVIDE_ASSERT(previousIDC.sourceBuffer() && currentIDC.sourceBuffer(),
                  "GFXDevice::batchCommands error: a command with an invalid "
                  "source buffer was specified!");
    DIVIDE_ASSERT(previousIDC.shaderProgram() && currentIDC.shaderProgram(),
                  "GFXDevice::batchCommands error: a command with an invalid "
                  "shader program was specified!");

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
void GFXDevice::drawPoints(U32 numPoints, U32 stateHash,
                           ShaderProgram* const shaderProgram) {
    // We need a valid amount of points. Check lower limit
    DIVIDE_ASSERT(numPoints != 0, Locale::get(_ID("ERROR_GFX_POINTS_UNDERFLOW")));
    // Also check upper limit
    DIVIDE_ASSERT(numPoints <= Config::MAX_POINTS_PER_BATCH, Locale::get(_ID("ERROR_GFX_POINTS_OVERFLOW")));

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
void GFXDevice::drawTriangle(U32 stateHash, ShaderProgram* const shaderProgram) {
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
    primitive.beginBatch(true, stacks * ((slices + 1) * 2));
    primitive.attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(color));
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
    primitive.beginBatch(true, 16);
    // Set it's color
    primitive.attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(color));
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
        primitive.beginBatch(true, to_uint(lines.size()) * 2 * 14);
        primitive.attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(lines[0]._colorStart));
        // Set the mode to line rendering
        //primitive.begin(PrimitiveType::TRIANGLE_STRIP);
        primitive.begin(PrimitiveType::LINES);
        primitive.drawShader(_imShaderLines);
        //vec3<F32> tempVertex;
        // Add every line in the list to the batch
        for (const Line& line : lines) {
            primitive.attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(line._colorStart));
            /*for (U16 idx : indices) {
                tempVertex.set(line._startPoint * vertices[idx]);
                tempVertex *= line._widthStart;

                primitive.vertex(tempVertex);
            }*/
            primitive.vertex(line._startPoint);

            primitive.attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(line._colorEnd));
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
    _displayShader->Uniform("anaglyphEnabled", anaglyphEnabled());
    getRenderTarget(RenderTargetID::SCREEN)._buffer->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    getRenderTarget(RenderTargetID::ANAGLYPH)._buffer->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1));
    drawTriangle(getDefaultStateBlock(true), _displayShader);
}

void GFXDevice::flushAnaglyph() {
    Framebuffer* anaglyph = getRenderTarget(RenderTargetID::ANAGLYPH)._buffer;
    Framebuffer* screen = getRenderTarget(RenderTargetID::SCREEN)._buffer;
    anaglyph->blitFrom(screen);
}

};
