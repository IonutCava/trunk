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

            if (buffer1._slot != buffer2._slot ||
                buffer1._range != buffer2._range ||
                buffer1._buffer->getGUID() != buffer2._buffer->getGUID()) {
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

void GFXDevice::uploadGlobalBufferData() {
    if (_buffersDirty[to_uint(GPUBuffer::NODE_BUFFER)]) {
        _nodeBuffer->UpdateData(0, _lastNodeCount, _matricesData.data());
        _buffersDirty[to_uint(GPUBuffer::NODE_BUFFER)] = false;
    }

    if (_buffersDirty[to_uint(GPUBuffer::CMD_BUFFER)]) {
        uploadDrawCommands(_drawCommandsCache, _lastCmdCount);
        _buffersDirty[to_uint(GPUBuffer::CMD_BUFFER)] = false;
    }

    if (_buffersDirty[to_uint(GPUBuffer::GPU_BUFFER)]) {
        // We flush the entire buffer on update to inform the GPU that we don't
        // need the previous data. Might avoid some driver sync
        _gfxDataBuffer->SetData(&_gpuBlock);
        _buffersDirty[to_uint(GPUBuffer::GPU_BUFFER)] = false;
    }
}

/// A draw command is composed of a target buffer and a command. The command
/// part is processed here
bool GFXDevice::setBufferData(const GenericDrawCommand& cmd) {
    // We also need a valid draw command ID so we can index the node buffer
    // properly
    DIVIDE_ASSERT(IS_IN_RANGE_INCLUSIVE(
                      cmd.drawID(), 0u,
                      std::max(static_cast<U32>(_lastNodeCount - 1), 1u)),
                  "GFXDevice error: Invalid draw ID encountered!");
    DIVIDE_ASSERT(cmd.primCount() > 0 && cmd.drawCount() > 0,
                  "GFXDevice error: Invalid draw command!");
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(cmd.shaderProgram() != nullptr,
                  "GFXDevice error: Draw shader state is not valid for the "
                  "current draw operation!");

    // Try to bind the shader program. If it failed to load, or isn't loaded
    // yet, cancel the draw request for this frame
    if (cmd.shaderProgram()->bind()) {
        cmd.shaderProgram()->Uniform("baseInstance", cmd.drawID());
        // Finally, set the proper render states
        setStateBlock(cmd.stateHash());
        // Continue with the draw call
        return true;
    }

    return false;
}

void GFXDevice::submitRenderCommand(const GenericDrawCommand& cmd) {
    _useIndirectCommands = false;
    processCommand(cmd);
}

void GFXDevice::submitRenderCommands(
    const vectorImpl<GenericDrawCommand>& cmds) {
    _useIndirectCommands = false;
    processCommands(cmds);
}

void GFXDevice::submitIndirectRenderCommand(const GenericDrawCommand& cmd) {
    _useIndirectCommands = true;
    processCommand(cmd);
}

void GFXDevice::submitIndirectRenderCommands(
    const vectorImpl<GenericDrawCommand>& cmds) {
    _useIndirectCommands = true;
    processCommands(cmds);
}

void GFXDevice::processCommand(const GenericDrawCommand& cmd) {
    /// Submit a single draw command
    DIVIDE_ASSERT(cmd.sourceBuffer() != nullptr,
                  "GFXDevice error: Invalid vertex buffer submitted!");
    // We may choose the instance count programmatically, and it may turn out to
    // be 0, so skip draw
    if (setBufferData(cmd)) {
        // Same rules about pre-processing the draw command apply
        cmd.sourceBuffer()->Draw(cmd, _useIndirectCommands);
    }
}

/// Submit multiple draw commands that use the same source buffer (e.g. terrain
/// or batched meshes)
void GFXDevice::processCommands(const vectorImpl<GenericDrawCommand>& cmds) {
    for (const GenericDrawCommand& cmd : cmds) {
        // Data validation is handled in the single command version
        processCommand(cmd);
    }
}

void GFXDevice::flushRenderQueue() {
    if (_renderQueue.empty()) {
        return;
    }
    uploadGlobalBufferData();
    for (RenderPackage& package : _renderQueue) {
        vectorImpl<GenericDrawCommand>& drawCommands = package._drawCommands;
        vectorAlg::vecSize commandCount = drawCommands.size();
        if (commandCount > 0) {
            GenericDrawCommand& previousCmd = drawCommands[0];
            for (vectorAlg::vecSize i = 1; i < commandCount; i++) {
                GenericDrawCommand& currentCmd = drawCommands[i];
                if (!batchCommands(previousCmd, currentCmd)) {
                    previousCmd = currentCmd;
                }
            }
            drawCommands.erase(
                std::remove_if(std::begin(drawCommands), std::end(drawCommands),
                               [](const GenericDrawCommand& cmd) -> bool {
                                   return cmd.drawCount() == 0 ||
                                          cmd.primCount() == 0;
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
    if (!package._drawCommands.empty()) {
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
}

/// Prepare the list of visible nodes for rendering
void GFXDevice::processVisibleNode(const RenderPassCuller::RenderableNode& node,
                                   NodeData& dataOut) {

    RenderingComponent* const renderable =
        node._visibleNode->getComponent<RenderingComponent>();
    AnimationComponent* const animComp =
        node._visibleNode->getComponent<AnimationComponent>();
    PhysicsComponent* const transform =
        node._visibleNode->getComponent<PhysicsComponent>();

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        dataOut._matrix[0].set(transform->getWorldMatrix(_interpolationFactor));
        // Calculate the normal matrix (world * view)
        // If the world matrix is uniform scaled, inverseTranspose is a
        // double transpose (no-op) so we can skip it
        dataOut._matrix[1].set(
            mat3<F32>(dataOut._matrix[0] * _gpuBlock._ViewMatrix));
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            dataOut._matrix[1].set(mat3<F32>(dataOut._matrix[0]));
            dataOut._matrix[1].inverseTranspose();
            dataOut._matrix[1] *= _gpuBlock._ViewMatrix;
        }
    }
    // Since the normal matrix is 3x3, we can use the extra row and column
    // to store additional data
    dataOut._matrix[1].element(3, 2, true) =
        static_cast<F32>(LightManager::getInstance().getLights().size());
    dataOut._matrix[1].element(3, 3, true) =
        static_cast<F32>(animComp ? animComp->boneCount() : 0);

    // Get the color matrix (diffuse, ambient, specular, etc.)
    renderable->getMaterialColorMatrix(dataOut._matrix[2]);
    // Get the material property matrix (alpha test, texture count,
    // texture operation, etc.)
    renderable->getMaterialPropertyMatrix(dataOut._matrix[3]);

    _buffersDirty[to_uint(GPUBuffer::NODE_BUFFER)] = true;
}

void GFXDevice::buildDrawCommands(VisibleNodeList& visibleNodes,
                                  SceneRenderState& sceneRenderState,
                                  bool refreshNodeData) {
    // If there aren't any nodes visible in the current pass, don't update
    // anything (but clear the render queue
    if (visibleNodes.empty()) {
        return;
    }

    Time::START_TIMER(_commandBuildTimer);

    vectorAlg::vecSize nodeCount = visibleNodes.size();
    if (refreshNodeData) {
        // Pass the list of nodes to the renderer for a pre-render pass
        getRenderer().processVisibleNodes(visibleNodes, _gpuBlock);
        // Generate and upload all lighting data
        LightManager::getInstance().updateAndUploadLightData(
            _gpuBlock._ViewMatrix);
            _lastNodeCount = 1;
    }
    _renderQueue.reserve(nodeCount);

    _lastCmdCount = 0;
    _drawCommandsCache[_lastCmdCount++].set(_defaultDrawCmd.cmd());

    U32 drawID = 1;
    // Loop over the list of nodes to generate a new command list
    RenderStage currentStage = getRenderStage();
    for (VisibleNodeList::value_type& node : visibleNodes) {
        vectorImpl<GenericDrawCommand>& nodeDrawCommands =
            Attorney::RenderingCompGFXDevice::getDrawCommands(
                *node._visibleNode->getComponent<RenderingComponent>(),
                sceneRenderState, currentStage);

        if (!nodeDrawCommands.empty()) {
            for (GenericDrawCommand& cmd : nodeDrawCommands) {
                cmd.drawID(drawID);
                // Extract the specific rendering commands from the draw commands
                // Rendering commands are stored in GPU memory. Draw commands are not.
                _drawCommandsCache[_lastCmdCount++].set(cmd.cmd());
            }
            if (refreshNodeData) {
               processVisibleNode(node, _matricesData[_lastNodeCount++]);
            }
            drawID += 1;
        }
    }
    _buffersDirty[to_uint(GPUBuffer::CMD_BUFFER)] = true;

    Time::STOP_TIMER(_commandBuildTimer);
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
        previousIDC.drawCount(prevCount + currentIDC.drawCount());
        // And set the current command's draw count to zero so it gets removed
        // from the list later on
        currentIDC.drawCount(0);
        return true;
    }

    return false;
}

/// This is just a short-circuit system (hack) to send a list of points to the
/// shader
/// It's used, mostly, to draw full screen quads using geometry shaders
void GFXDevice::drawPoints(U32 numPoints, size_t stateHash,
                           ShaderProgram* const shaderProgram) {
    // We need a valid amount of points. Check lower limit
    if (numPoints == 0) {
        Console::errorfn(Locale::get("ERROR_GFX_POINTS_UNDERFLOW"));
        return;
    }
    // Also check upper limit
    if (numPoints > Config::MAX_POINTS_PER_BATCH) {
        Console::errorfn(Locale::get("ERROR_GFX_POINTS_OVERFLOW"));
        return;
    }
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

/// GUI specific elements, require custom handling by the rendering API
void GFXDevice::drawGUIElement(GUIElement* guiElement) {
    DIVIDE_ASSERT(guiElement != nullptr,
                  "GFXDevice error: Invalid GUI element specified for the "
                  "drawGUI command!");
    // Skip hidden elements
    if (!guiElement->isVisible()) {
        return;
    }
    // Set the elements render states
    setStateBlock(guiElement->getStateBlockHash());
    // Choose the appropriate rendering path
    switch (guiElement->getType()) {
        case GUIType::GUI_TEXT: {
            const GUIText& text = static_cast<GUIText&>(*guiElement);
            drawText(text, text.getPosition());
        } break;
        case GUIType::GUI_FLASH: {
            static_cast<GUIFlash*>(guiElement)->playMovie();
        } break;
        default: {
            // not supported
        } break;
    };
    // Update internal timer
    guiElement->lastDrawTimer(Time::ElapsedMicroseconds());
}

/// Draw the outlines of a box defined by min and max as extents using the
/// specified world matrix
void GFXDevice::drawBox3D(const vec3<F32>& min,
                          const vec3<F32>& max,
                          const vec4<U8>& color) {
    // Grab an available primitive
    IMPrimitive* priv = getOrCreatePrimitive();
    // Prepare it for rendering lines
    priv->_hasLines = true;
    priv->_lineWidth = 4.0f;
    priv->stateHash(_defaultStateBlockHash);
    // Create the object
    priv->beginBatch();
    // Set it's color
    priv->attribute4ub("inColorData", color);
    // Draw the bottom loop
    priv->begin(PrimitiveType::LINE_LOOP);
    priv->vertex(min.x, min.y, min.z);
    priv->vertex(max.x, min.y, min.z);
    priv->vertex(max.x, min.y, max.z);
    priv->vertex(min.x, min.y, max.z);
    priv->end();
    // Draw the top loop
    priv->begin(PrimitiveType::LINE_LOOP);
    priv->vertex(min.x, max.y, min.z);
    priv->vertex(max.x, max.y, min.z);
    priv->vertex(max.x, max.y, max.z);
    priv->vertex(min.x, max.y, max.z);
    priv->end();
    // Connect the top to the bottom
    priv->begin(PrimitiveType::LINES);
    priv->vertex(min.x, min.y, min.z);
    priv->vertex(min.x, max.y, min.z);
    priv->vertex(max.x, min.y, min.z);
    priv->vertex(max.x, max.y, min.z);
    priv->vertex(max.x, min.y, max.z);
    priv->vertex(max.x, max.y, max.z);
    priv->vertex(min.x, min.y, max.z);
    priv->vertex(min.x, max.y, max.z);
    priv->end();
    // Finish our object
    priv->endBatch();
}

/// Render a list of lines within the specified constraints
void GFXDevice::drawLines(const vectorImpl<Line>& lines,
                          const mat4<F32>& globalOffset,
                          const vec4<I32>& viewport,
                          const bool inViewport,
                          const bool disableDepth) {
    // Check if we have a valid list. The list can be programmatically
    // generated, so this check is required
    if (lines.empty()) {
        return;
    }
    // Grab an available primitive
    IMPrimitive* priv = getOrCreatePrimitive();
    // Prepare it for line rendering
    priv->_hasLines = true;
    priv->_lineWidth = 5.0f;
    priv->stateHash(getDefaultStateBlock(disableDepth));
    // Set the world matrix
    priv->worldMatrix(globalOffset);
    // If we need to render it into a specific viewport, set the pre and post
    // draw functions to set up the
    // needed viewport rendering (e.g. axis lines)
    if (inViewport) {
        priv->setRenderStates(
            DELEGATE_BIND(&GFXDevice::setViewport, this, viewport),
            DELEGATE_BIND(&GFXDevice::restoreViewport, this));
    }
    // Create the object containing all of the lines
    priv->beginBatch();
    priv->attribute4ub("inColorData", lines[0]._color);
    // Set the mode to line rendering
    priv->begin(PrimitiveType::LINES);
    // Add every line in the list to the batch
    for (const Line& line : lines) {
        priv->attribute4ub("inColorData", line._color);
        priv->vertex(line._startPoint);
        priv->vertex(line._endPoint);
    }
    priv->end();
    // Finish our object
    priv->endBatch();
}
};