#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"

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
        _gfxDataBuffer->SetData(&_gpuBlock._data);
        _gpuBlock._updated = false;
    }

    _gfxDataBuffer->Bind(ShaderBufferLocation::GPU_BLOCK);
    _nodeBuffer->Bind(ShaderBufferLocation::NODE_INFO);
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
    for (RenderPackage& package : _renderQueue) {
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
                it._buffer->BindRange(it._slot, it._range.x, it._range.y);
            }

            makeTexturesResident(package._textureData);
            submitIndirectRenderCommands(package._drawCommands);
        }
    }
    _renderQueue.resize(0);
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
    }
    _renderQueue.push_back(package);
}

/// Prepare the list of visible nodes for rendering
void GFXDevice::processVisibleNode(const RenderPassCuller::RenderableNode& node,
                                   NodeData& dataOut) {

    assert(node._visibleNode != nullptr);

    const SceneGraphNode& nodeRef = *node._visibleNode;

    RenderingComponent* const renderable =
        nodeRef.getComponent<RenderingComponent>();
    AnimationComponent* const animComp =
        nodeRef.getComponent<AnimationComponent>();
    PhysicsComponent* const transform =
        nodeRef.getComponent<PhysicsComponent>();

    mat4<F32>& modelMatrix = dataOut._matrix[0];
    mat4<F32>& normalMatrix = dataOut._matrix[1];

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        modelMatrix.set(transform->getWorldMatrix(_interpolationFactor));
        // Calculate the normal matrix (world * view)
        // If the world matrix is uniform scaled, inverseTranspose is a
        // double transpose (no-op) so we can skip it

        //Avoid allocating a new matrix on the stack.
        // Alternative: normalMatrix.set(mat3<F32>(modelMatrix));
        normalMatrix.set(modelMatrix);
        normalMatrix.setCol(3, 0.0f, 0.0f, 0.0f, 0.0f);

        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            normalMatrix.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            normalMatrix.inverseTranspose();
            normalMatrix.mat[15] = 0.0f;
        } else {
            normalMatrix.setRow(3, 0.0f);
        }

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

vec2<U32> GFXDevice::processNodeBucket(VisibleNodeList::iterator nodeIt,
                                       SceneRenderState& sceneRenderState,
                                       vec2<U32> offset,
                                       bool refreshNodeData) {

    vec2<U32> retValue(offset.x);
    U32& intNodeCount = retValue.x;
    U32& intCmdCount = retValue.y;

    // Loop over the list of nodes to generate a new command list
    RenderStage currentStage = getRenderStage();

    std::for_each(nodeIt + offset.x, nodeIt + offset.y,
        [&](GFXDevice::VisibleNodeList::value_type& node) -> void {
        SceneGraphNode& nodeRef = *node._visibleNode;

        RenderPackage& pkg =
            Attorney::RenderingCompGFXDevice::getDrawPackage(
                *nodeRef.getComponent<RenderingComponent>(),
                sceneRenderState,
                currentStage);

        if (pkg._isRenderable) {
            if (refreshNodeData) {
                processVisibleNode(node, _matricesData[intNodeCount]);
            }

            for (GenericDrawCommand& cmd : pkg._drawCommands) {
                IndirectDrawCommand& iCmd = cmd.cmd();
                iCmd.baseInstance = intNodeCount;

                cmd.drawID(intNodeCount);
                cmd.renderWireframe(cmd.renderWireframe() || sceneRenderState.drawWireframe());
                // Extract the specific rendering commands from the draw commands
                // Rendering commands are stored in GPU memory. Draw commands are not.
                _drawCommandsCache[intCmdCount].set(iCmd);
                intCmdCount++;
            }
            intNodeCount++;
        }
    });

    return retValue;
}

void GFXDevice::buildDrawCommands(VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  bool refreshNodeData) {
    Time::ScopedTimer timer(*_commandBuildTimer);
    // If there aren't any nodes visible in the current pass, don't update
    // anything (but clear the render queue
    if (visibleNodes.empty()) {
        return;
    }
    vectorAlg::vecSize nodeCount = visibleNodes.size();

    if (refreshNodeData) {
        // Pass the list of nodes to the renderer for a pre-render pass
        getRenderer().preRender(_gpuBlock);
    }

    _drawCommandsCache[0].set(_defaultDrawCmd.cmd());

    //Use up to half of our physical cores for command generation
    //(leave some cores available for other thread pools)
    static const bool useThreadedCommands = false;
    static const U32 coreCount = HARDWARE_THREAD_COUNT() / 2;
    const U32 bucketSize = to_uint(std::ceil(to_float(nodeCount) / coreCount));
    const U32 taskCount = std::min(coreCount, to_uint(nodeCount));
    U32 startOffset = 0;
    U32 endOffset = 0;

    vec2<U32> parsedValues;

    if (useThreadedCommands) {
        vectorImpl<Task_ptr> cmds;
        cmds.reserve(taskCount);
        for (U32 i = 0; i < taskCount; ++i) {
            startOffset = std::min(i * bucketSize, to_uint(nodeCount));
            endOffset = std::min(startOffset + bucketSize, to_uint(nodeCount));


            cmds.push_back(
            std::make_shared<Task>(_state.getRenderingPool(), 1, 1,
                                    [&](){
                                        /// Add proper locking/atomics here
                                        parsedValues += processNodeBucket(std::begin(visibleNodes),
                                                                          sceneRenderState,
                                                                          vec2<U32>(startOffset, endOffset),
                                                                          refreshNodeData);
                                    }));
       
        }
    
        for(Task_ptr& cmd : cmds) {
            cmd->startTask();
        }

        _state.getRenderingPool().wait();

    } else {
        for (U32 i = 0; i < taskCount; ++i) {
            startOffset = std::min(i * bucketSize, to_uint(nodeCount));
            endOffset = std::min(startOffset + bucketSize, to_uint(nodeCount));

            parsedValues += processNodeBucket(std::begin(visibleNodes),
                                              sceneRenderState,
                                              vec2<U32>(startOffset, endOffset),
                                              refreshNodeData);
        }
    }

    if (refreshNodeData) {
        _nodeBuffer->UpdateData(0, parsedValues.x, _matricesData.data());
    }

    uploadDrawCommands(_drawCommandsCache, parsedValues.y);
    
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
        if (previousIDC.drawID() + prevCount != currentIDC.drawID()) {
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