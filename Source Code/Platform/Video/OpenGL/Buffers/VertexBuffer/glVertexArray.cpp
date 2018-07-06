#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

/// Default destructor
glVertexArray::glVertexArray()
    : VertexBuffer(),
      _refreshQueued(false),
      _animationData(false),
      _formatInternal(GL_NONE) {
    // We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    _prevSize.fill(-1);
    _prevSizeIndices = -1;
    _VAOid = _VBid = _IBid = 0;
}

glVertexArray::~glVertexArray() {
    Destroy();
}

/// Delete buffer
void glVertexArray::Destroy() {
    GLUtil::freeBuffer(_IBid);
    GLUtil::freeBuffer(_VBid);
    if (_VAOid > 0)
        glDeleteVertexArrays(1, &_VAOid);
    _VAOid = 0;
}

/// Create a dynamic or static VB
bool glVertexArray::Create(bool staticDraw) {
    // If we want a dynamic buffer, then we are doing something outdated, such
    // as software skinning, or software water rendering
    if (!staticDraw) {
        // OpenGLES support isn't added, but this check doesn't break anything,
        // so I'll just leave it here -Ionut
        GLenum usage = (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::OpenGLES)
                           ? GL_STREAM_DRAW
                           : GL_DYNAMIC_DRAW;
        if (usage != _usage) {
            _usage = usage;
            _refreshQueued = true;
        }
    }

    return true;
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexArray::Refresh() {
    // Dynamic LOD elements (such as terrain) need dynamic indices
    // We can manually override index usage (again, used by the Terrain
    // rendering system)
    DIVIDE_ASSERT(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty(),
                  "glVertexArray error: Invalid index data on Refresh()!");
    // Get the size of each data container
    std::array<GLsizei, to_const_uint(VertexAttribute::COUNT)> nSize = { 0 };

    nSize[to_uint(VertexAttribute::ATTRIB_POSITION)] =
        (GLsizei)(_dataPosition.size() * sizeof(vec3<F32>));
    nSize[to_uint(VertexAttribute::ATTRIB_COLOR)] =
        (GLsizei)(_dataColor.size() * sizeof(vec4<U8>));
    nSize[to_uint(VertexAttribute::ATTRIB_NORMAL)] =
        (GLsizei)(_dataNormal.size() * sizeof(vec3<F32>));
    nSize[to_uint(VertexAttribute::ATTRIB_TEXCOORD)] =
        (GLsizei)(_dataTexcoord.size() * sizeof(vec2<F32>));
    nSize[to_uint(VertexAttribute::ATTRIB_TANGENT)] =
        (GLsizei)(_dataTangent.size() * sizeof(F32));
    nSize[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] =
        (GLsizei)(_boneWeights.size() * sizeof(vec4<F32>));
    nSize[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)] =
        (GLsizei)(_boneIndices.size() * sizeof(P32));
    GLsizei nSizeIndices =
        (GLsizei)(usesLargeIndices() ? _hardwareIndicesL.size() * sizeof(U32)
                                     : _hardwareIndicesS.size() * sizeof(U16));
    // If any of the VBO's components changed size, we need to recreate the
    // entire buffer.
    bool sizeChanged = false;
    for (U8 i = 0; i < to_uint(VertexAttribute::COUNT); ++i) {
        if (nSize[i] != _prevSize[i]) {
            sizeChanged = true;
            _prevSize[i] = nSize[i];
        }
    }
    // Same logic applies to indices.
    bool indicesChanged = (nSizeIndices != _prevSizeIndices);
    _prevSizeIndices = nSizeIndices;
    _refreshQueued = sizeChanged || indicesChanged;

    if (!_refreshQueued) {
        for (U8 i = 0; i < to_uint(VertexAttribute::COUNT); ++i) {
            if (_attribDirty[i]) {
                _refreshQueued = true;
                break;
            }
        }
    }

    if (!_refreshQueued) {
        return false;
    }

    // Check if we have any animation data.
    _animationData = (!_boneWeights.empty() && !_boneIndices.empty());
    // Refresh buffer data (if this is the first call to refresh, this will be
    // true)
    if (sizeChanged) {
        // Calculated the needed size for our buffer
        GLsizei vbSize = 0;
        for (U8 i = 0; i < to_uint(VertexAttribute::COUNT); ++i) {
            vbSize += nSize[i];
        }
        Console::d_printfn(Locale::get("DISPLAY_VB_METRICS"), vbSize,
                           4 * 1024 * 1024);
        // Allocate sufficient space in our buffer
        glNamedBufferData(_VBid, vbSize, NULL, _usage);
        // Refresh all attributes
        _attribDirty.fill(true);
    }

    if (_attribDirty[to_uint(VertexAttribute::ATTRIB_TANGENT)]) {
        _tangentSmall.resize(0);
        _tangentSmall.reserve(_dataTangent.size());
        for (const vec3<F32>& tangent : _dataTangent) {
            _tangentSmall.push_back(Util::PACK_VEC3(tangent));
        }
    }

    // Get the buffer data for each attribute
    std::array<bufferPtr, to_const_uint(VertexAttribute::COUNT)> dataBuffer = {
        nullptr};

    dataBuffer[to_uint(VertexAttribute::ATTRIB_POSITION)] =
        _dataPosition.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_COLOR)] = _dataColor.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_NORMAL)] = _dataNormal.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_TEXCOORD)] =
        _dataTexcoord.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_TANGENT)] = _tangentSmall.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] =
        _boneWeights.data();
    dataBuffer[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)] =
        _boneIndices.data();

    // Get position data offset
    U32 attrib = to_uint(VertexAttribute::ATTRIB_POSITION);
    _VBoffset[attrib] = 0;
    GLsizei previousOffset = static_cast<GLsizei>(_VBoffset[attrib]);
    GLsizei previousCount = nSize[attrib];
    // Loop over all possible attributes
    for (; attrib < to_uint(VertexAttribute::COUNT); ++attrib) {
        // Only process available attributes
        if (nSize[attrib] > 0) {
            // Get it's offset
            if (attrib > to_uint(VertexAttribute::ATTRIB_POSITION)) {
                _VBoffset[attrib] = previousOffset + previousCount;
                previousOffset = static_cast<GLsizei>(_VBoffset[attrib]);
                previousCount = nSize[attrib];
            }
            // Add it to the VBO
            if (_attribDirty[attrib]) {
                glNamedBufferSubData(_VBid, _VBoffset[attrib], nSize[attrib], dataBuffer[attrib]);
                if (attrib == to_uint(VertexAttribute::ATTRIB_TANGENT)) {
                    _tangentSmall.clear();
                }
            }
        }
    }

    _attribDirty.fill(false);
    // Check if we need to update the IBO (will be true for the first Refresh()
    // call)
    if (indicesChanged) {
        bufferPtr data = usesLargeIndices()
                             ? static_cast<bufferPtr>(_hardwareIndicesL.data())
                             : static_cast<bufferPtr>(_hardwareIndicesS.data());
        // Update our IB
        glNamedBufferData(_IBid, nSizeIndices, data, GL_STATIC_DRAW);
    }
    // Set vertex attribute pointers
    Upload_VB_Attributes();
    // Validate the buffer
    checkStatus();
    // Possibly clear client-side buffer for all non-required attributes?
    // foreach attribute if !required then delete else skip ?
    _refreshQueued = false;
    return true;
}

/// This method creates the initial VAO and VB OpenGL objects and queues a
/// Refresh call
bool glVertexArray::CreateInternal() {
    // Avoid double create calls
    DIVIDE_ASSERT(!_created,
                  "glVertexArray error: Attempted to double create a VB");
    _created = false;
    // Position data is a minim requirement
    if (_dataPosition.empty()) {
        Console::errorfn(Locale::get("ERROR_VB_POSITION"));
        return _created;
    }

    _formatInternal = GLUtil::glDataFormat[to_uint(_format)];
    // Enforce all update flags. Kind of useless, but it doesn't hurt
    _attribDirty.fill(true);
    // Generate a "Vertex Array Object"
    glCreateVertexArrays(1, &_VAOid);
    // Generate a new Vertex Buffer Object
    glCreateBuffers(1, &_VBid);
    // Generate an "Index Buffer Object"
    glCreateBuffers(1, &_IBid);
    // Validate buffer creation
    DIVIDE_ASSERT(_VAOid != 0, Locale::get("ERROR_VAO_INIT"));
    DIVIDE_ASSERT(_VBid != 0, Locale::get("ERROR_VB_INIT"));
    // Assert if the IB creation failed
    DIVIDE_ASSERT(_IBid != 0, Locale::get("ERROR_IB_INIT"));
    // Calling refresh updates all stored information and sends it to the GPU
    _created = queueRefresh();
    return _created;
}

/// Render the current buffer data using the specified command
void glVertexArray::Draw(const GenericDrawCommand& command,
                         std::shared_ptr<HardwareQuery> hardwareQuery,
                         bool useCmdBuffer) {
    // Process the actual draw command
    if (Config::Profile::DISABLE_DRAWS) {
        return;
    }

    DIVIDE_ASSERT(command.primitiveType() != PrimitiveType::COUNT,
                  "glVertexArray error: Draw command's type is not valid!");
    // Instance count can be generated programmatically,
    // so we need to make sure it's valid
    if (command.cmd().primCount == 0) {
        return;
    }
    // Make sure the buffer is current
    if (!SetActive()) {
        return;
    }
    bool generateOccQuery = false;
    glHardwareQuery* query = static_cast<glHardwareQuery*>(hardwareQuery.get());
    if (!query->enabled()) {
        query->enabled(true);
        generateOccQuery = true;
        glBeginQuery(GL_ANY_SAMPLES_PASSED, query->getID());
    } else {
        //glBeginConditionalRender(query->getID(), GL_QUERY_BY_REGION_WAIT);
    }

    static const size_t cmdSize = sizeof(IndirectDrawCommand);

    bufferPtr offset = (bufferPtr)(command.drawID() * cmdSize);
    if (!useCmdBuffer) {
        GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        offset = (bufferPtr)(&command.cmd());
    }

    U16 drawCount = command.drawCount();
    if (command.renderGeometry()) {
        GLenum mode = GLUtil::glPrimitiveTypeTable[to_uint(command.primitiveType())];
        // Submit the draw command
        if (drawCount > 1) {
            glMultiDrawElementsIndirect(mode, _formatInternal, offset, drawCount, cmdSize);
        } else {
            glDrawElementsIndirect(mode, _formatInternal, offset);
        }
        // Always update draw call counter after draw calls
        GFX_DEVICE.registerDrawCall();
    }

    if (generateOccQuery) {
        glEndQuery(GL_ANY_SAMPLES_PASSED);
    } else {
        //glEndConditionalRender();
        query->enabled(false);

        if (command.renderWireframe()) {
            if (drawCount > 1) {
                glMultiDrawElementsIndirect(GL_LINE_LOOP, _formatInternal, offset, drawCount, cmdSize);
            } else {
                glDrawElementsIndirect(GL_LINE_LOOP, _formatInternal, offset);
            }
            // Always update draw call counter after draw calls
            GFX_DEVICE.registerDrawCall();
        }
    }
}

/// Set the current buffer as active
bool glVertexArray::SetActive() {
    // Make sure we have valid data (buffer creation is deferred to the first
    // activate call)
    if (!_created) {
        if (!CreateInternal()) {
            return false;
        }
    }
    // Check if we have a refresh request queued up
    if (_refreshQueued) {
        if (!Refresh()) {
            return false;
        }
    }

    // Bind the vertex array object that in turn activates all of the bindings
    // and pointers set on creation
    if (GL_API::setActiveVAO(_VAOid)) {
        // If this is the first time the VAO is bound in the current loop, check
        // for primitive restart requests
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled);
    }
    return true;
}

/// Activate and set all of the required vertex attributes.
void glVertexArray::Upload_VB_Attributes() {
    // Bind the current VAO to save our attributes
    GL_API::setActiveVAO(_VAOid);
    // Bind the the vertex buffer and index buffer
    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _VBid);
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);

    glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_POSITION));
    glVertexAttribPointer(
        to_uint(AttribLocation::VERTEX_POSITION), 3, GL_FLOAT, GL_FALSE,
        sizeof(vec3<F32>),
        (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_POSITION)]));

    if (!_dataColor.empty()) {
        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_COLOR));
        glVertexAttribIPointer(
            to_uint(AttribLocation::VERTEX_COLOR), 4, GL_UNSIGNED_BYTE,
            sizeof(vec4<U8>),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_COLOR)]));
    } else {
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_COLOR));
    }

    if (!_dataNormal.empty()) {
        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_NORMAL));
        glVertexAttribPointer(
            to_uint(AttribLocation::VERTEX_NORMAL), 3, GL_FLOAT, GL_FALSE,
            sizeof(vec3<F32>),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_NORMAL)]));
    } else {
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_NORMAL));
    }

    if (!_dataTexcoord.empty()) {
        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_TEXCOORD));
        glVertexAttribPointer(
            to_uint(AttribLocation::VERTEX_TEXCOORD), 2, GL_FLOAT, GL_FALSE,
            sizeof(vec2<F32>),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_TEXCOORD)]));
    } else {
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_TEXCOORD));
    }

    if (!_dataTangent.empty()) {
        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_TANGENT));
        glVertexAttribPointer(
            to_uint(AttribLocation::VERTEX_TANGENT), 1, GL_FLOAT, GL_FALSE,
            sizeof(F32),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_TANGENT)]));
    } else {
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_TANGENT));
    }

    if (_animationData) {
        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_BONE_WEIGHT));
        glVertexAttribPointer(
            to_uint(AttribLocation::VERTEX_BONE_WEIGHT), 4, GL_FLOAT, GL_FALSE,
            sizeof(vec4<F32>),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)]));

        glEnableVertexAttribArray(to_uint(AttribLocation::VERTEX_BONE_INDICE));
        glVertexAttribIPointer(
            to_uint(AttribLocation::VERTEX_BONE_INDICE), 1, GL_UNSIGNED_INT,
            sizeof(P32),
            (bufferPtr)(_VBoffset[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)]));
    } else {
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_BONE_WEIGHT));
        glDisableVertexAttribArray(to_uint(AttribLocation::VERTEX_BONE_INDICE));
    }
}

/// Various post-create checks
void glVertexArray::checkStatus() {
}
};
