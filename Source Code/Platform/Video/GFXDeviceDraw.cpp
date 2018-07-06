#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

#include "Managers/Headers/SceneManager.h"

#include "Core/Headers/ProfileTimer.h"
#include "Core/Headers/ApplicationTimer.h"

namespace Divide {

// ToDo: This will return false if the number of shader buffers or number of
// textures does not match between the 2 packages altough said buffers/textures
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
        for (vectorAlg::vecSize i = 0; i < textureCount; i++) {
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

    // This forces a sync for each buffer to make sure all data is properly uploaded in VRAM
    _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);
    _nodeBuffer->bind(ShaderBufferLocation::NODE_INFO);
}

/// A draw command is composed of a target buffer and a command. The command
/// part is processed here
bool GFXDevice::setBufferData(const GenericDrawCommand& cmd) {
    DIVIDE_ASSERT(cmd.cmd().primCount > 0 && cmd.drawCount() > 0,
                  "GFXDevice error: Invalid draw command!");
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
            // Is this really faster than just letting the various draw commands handle the checks? Needs testing. -Ionut
            drawCommands.erase(
                std::remove_if(std::begin(drawCommands), std::end(drawCommands),
                               [](const GenericDrawCommand& cmd) -> bool {
                                   return cmd.drawCount() == 0 ||
                                          cmd.cmd().primCount == 0;
                               }),
                std::end(drawCommands));

            std::for_each(std::begin(drawCommands), std::end(drawCommands),
                          [](GenericDrawCommand& cmd) -> void { cmd.lock(); });

            for (ShaderBufferList::value_type& it : package._shaderBuffers) {
                it._buffer->bindRange(it._slot, it._range.x, it._range.y);
            }

            makeTexturesResident(package._textureData);
            submitIndirectRenderCommands(package._drawCommands);
        }
    }

    _renderQueue.clear();
}

void GFXDevice::addToRenderQueue(const RenderPackage& package) {
    if (!package._isRenderable) {
        return;
    }

    if (!_renderQueue.empty()) {
        RenderPackage& previous = _renderQueue.back();

        if (previous.isCompatible(package)) {
            previous._drawCommands.insert(std::end(previous._drawCommands),
                                          std::begin(package._drawCommands),
                                          std::end(package._drawCommands));
            return;
        }
    } else {
        _renderQueue.resize(RenderPassManager::getInstance().getQueue().getRenderQueueStackSize());
    }
    _renderQueue.push_back(package);
}

/// Prepare the list of visible nodes for rendering
void GFXDevice::processVisibleNode(SceneGraphNode_wptr node, NodeData& dataOut) {

    SceneGraphNode_ptr nodePtr = node.lock();
    assert(nodePtr);

    RenderingComponent* const renderable = nodePtr->getComponent<RenderingComponent>();
    AnimationComponent* const animComp = nodePtr->getComponent<AnimationComponent>();
    PhysicsComponent* const transform = nodePtr->getComponent<PhysicsComponent>();

    mat4<F32>& modelMatrix = dataOut._matrix[0];
    mat4<F32>& normalMatrix = dataOut._matrix[1];

    dataOut._boundingSphere.set(nodePtr->getBoundingSphereConst().asVec4());

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        modelMatrix.set(transform->getWorldMatrix(_interpolationFactor, normalMatrix));
        // Calculate the normal matrix (world * view)
        normalMatrix.set(normalMatrix * _gpuBlock._data._ViewMatrix);
    }

    // Since the normal matrix is 3x3, we can use the extra row and column
    // to store additional data
    normalMatrix.element(3, 2, true) = to_float(animComp ? animComp->boneCount() : 0);

    // Get the color matrix (diffuse, ambient, specular, etc.)
    renderable->getMaterialColorMatrix(dataOut._matrix[2]);
    // Get the material property matrix (alpha test, texture count,
    // texture operation, etc.)
    renderable->getMaterialPropertyMatrix(dataOut._matrix[3]);
}

void GFXDevice::buildDrawCommands(VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  bool refreshNodeData) {
    Time::ScopedTimer timer(*_commandBuildTimer);
    // If there aren't any nodes visible in the current pass, don't update
    // anything (but clear the render queue
    if (visibleNodes.empty()) {
        _lastCommandCount = _lastNodeCount = 0;
    }

    if (refreshNodeData) {
        // Pass the list of nodes to the renderer for a pre-render pass
        getRenderer().preRender(_gpuBlock);
    }

    _drawCommandsCache[0].set(_defaultDrawCmd.cmd());

    RenderStage currentStage = getRenderStage();

    U32 nodeCount = 1; U32 cmdCount = 1;
    std::for_each(std::begin(visibleNodes), std::end(visibleNodes),
        [&](GFXDevice::VisibleNodeList::value_type& node) -> void {
        SceneGraphNode_ptr nodeRef = node.lock();

        RenderPackage& pkg =
            Attorney::RenderingCompGFXDevice::getDrawPackage(
                *nodeRef->getComponent<RenderingComponent>(),
                sceneRenderState,
                currentStage);

        if (pkg._isRenderable) {
            if (refreshNodeData) {
                processVisibleNode(node, _matricesData[nodeCount]);
            }

            for (GenericDrawCommand& cmd : pkg._drawCommands) {
                IndirectDrawCommand& iCmd = cmd.cmd();
                iCmd.baseInstance = nodeCount;
                cmd.renderWireframe(cmd.renderWireframe() || sceneRenderState.drawWireframe());
                // Extract the specific rendering commands from the draw commands
                // Rendering commands are stored in GPU memory. Draw commands are not.
                _drawCommandsCache[nodeCount].set(iCmd);
                cmdCount++;
            }
            nodeCount++;
        }
    });
    
    

    _indirectCommandBuffer->updateData(0, cmdCount, _drawCommandsCache.data());
    _lastCommandCount = cmdCount;

    if (refreshNodeData) {
        _nodeBuffer->updateData(0, nodeCount, _matricesData.data());
        _lastNodeCount = nodeCount;
    } else {
        occlusionCull();
    }
}

void GFXDevice::occlusionCull() {
    static const U32 GROUP_SIZE_AABB = 64;
    uploadGPUBlock();

    _HIZCullProgram->bind();
    _HIZCullProgram->Uniform("dvd_numEntities", _lastCommandCount);
    getRenderTarget(RenderTarget::DEPTH)->Bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH),
                                               TextureDescriptor::AttachmentType::Depth);
    _indirectCommandBuffer->bind(ShaderBufferLocation::GPU_COMMANDS);
    _indirectCommandBuffer->bindAtomicCounter();
    _HIZCullProgram->DispatchCompute((_lastCommandCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    _HIZCullProgram->SetMemoryBarrier();
}

U32 GFXDevice::getLastCullCount() const {
    U32 cullCount = _indirectCommandBuffer->getAtomicCounter();
    if (cullCount > 0) {
        _indirectCommandBuffer->resetAtomicCounter();
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

void GFXDevice::drawRenderTarget(Framebuffer* renderTarget, const vec4<I32>& viewport) {
}

void GFXDevice::postProcessRenderTarget(RenderTarget renderTarget) {
    switch(renderTarget) {
        case RenderTarget::DEPTH: 
            constructHIZ();
            break;
        case RenderTarget::ANAGLYPH:
        case RenderTarget::SCREEN:
            break;
    }
}

/// This is just a short-circuit system (hack) to send a list of points to the
/// shader
/// It's used, mostly, to draw full screen quads using geometry shaders
void GFXDevice::drawPoints(U32 numPoints, size_t stateHash,
                           ShaderProgram* const shaderProgram) {
    // We need a valid amount of points. Check lower limit
    DIVIDE_ASSERT(numPoints != 0, Locale::get("ERROR_GFX_POINTS_UNDERFLOW"));
    // Also check upper limit
    DIVIDE_ASSERT(numPoints <= Config::MAX_POINTS_PER_BATCH, Locale::get("ERROR_GFX_POINTS_OVERFLOW"));

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
void GFXDevice::drawTriangle(size_t stateHash,
                             ShaderProgram* const shaderProgram) {
    // We require a state hash value to set proper states
    _defaultDrawCmd.stateHash(stateHash);
    // We also require a shader program (although it may already be bound.
    // Better safe ...)
    _defaultDrawCmd.shaderProgram(shaderProgram);
    // If the draw command was successfully parsed
    if (setBufferData(_defaultDrawCmd)) {
        // Tell the rendering API to upload the needed number of points
        drawTriangle();
    }

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
                DELEGATE_BIND(&GFXDevice::setViewport, this, viewport),
                DELEGATE_BIND(&GFXDevice::restoreViewport, this));
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

};