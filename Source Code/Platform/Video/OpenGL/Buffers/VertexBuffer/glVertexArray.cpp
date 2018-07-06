#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace std {
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        typedef array<T, N> argument_type;
        typedef size_t result_type;

        result_type operator()(const argument_type& a) const
        {
            hash<T> hasher;
            result_type h = 0;
            for (result_type i = 0; i < N; ++i)
            {
                h = h * 31 + hasher(a[i]);
            }
            return h;
        }
    };
};

namespace Divide {

glVertexArray::VaoMap glVertexArray::_vaos;

GLuint glVertexArray::getVao(size_t hash) {
    VaoMap::const_iterator result = _vaos.find(hash);
    return result != std::end(_vaos) ? result->second
                                     : 0;
}

void glVertexArray::setVao(size_t hash, GLuint id) {
    std::pair<VaoMap::const_iterator, bool> result = 
        hashAlg::emplace(_vaos, hash, id);
    assert(result.second);
}

void glVertexArray::clearVaos() {
    for (VaoMap::value_type& value : _vaos) {
        if (value.second != 0) {
            glDeleteVertexArrays(1, &value.second);
        }
    }
    _vaos.clear();
}

/// Default destructor
glVertexArray::glVertexArray()
    : VertexBuffer(),
      _refreshQueued(false),
      _formatInternal(GL_NONE)
{
    // We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    _prevSize = -1;
    _prevSizeIndices = -1;
    _bufferEntrySize = -1;
    _vaoCache = _VBid = _IBid = 0;
    _vaoHash = static_cast<size_t>(-1);

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
}

glVertexArray::~glVertexArray()
{
    destroy();
}

/// Delete buffer
void glVertexArray::destroy() {
    GLUtil::freeBuffer(_IBid);
    GLUtil::freeBuffer(_VBid);
}

/// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
std::pair<bufferPtr, size_t> glVertexArray::getMinimalData() {
    bool useColor = _useAttribute[to_uint(VertexAttribute::ATTRIB_COLOR)];
    bool useNormals = _useAttribute[to_uint(VertexAttribute::ATTRIB_NORMAL)];
    bool useTangents = _useAttribute[to_uint(VertexAttribute::ATTRIB_TANGENT)];
    bool useTexcoords = _useAttribute[to_uint(VertexAttribute::ATTRIB_TEXCOORD)];
    bool useBoneData = _useAttribute[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)];

    size_t prevOffset = sizeof(vec3<F32>);
    if (useNormals) {
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_NORMAL)] = to_uint(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useTangents) {
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_TANGENT)] = to_uint(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useColor) {
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_COLOR)] = to_uint(prevOffset);
        prevOffset += sizeof(vec4<U8>);
    }

    if (useTexcoords) {
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_TEXCOORD)] = to_uint(prevOffset);
        prevOffset += sizeof(vec2<F32>);
    }

    if (useBoneData) {
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] = to_uint(prevOffset);
        prevOffset += sizeof(vec4<F32>);
        _attributeOffset[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)] = to_uint(prevOffset);
        prevOffset += sizeof(U32);
    }

    _bufferEntrySize = static_cast<GLsizei>(prevOffset);

    _smallData.reserve(_data.size() * _bufferEntrySize);

    for (const Vertex& data : _data) {
        _smallData << data._position.x;
        _smallData << data._position.y;
        _smallData << data._position.z;

        if (useNormals) {
            _smallData << data._normal;
        }

        if (useTangents) {
            _smallData << data._tangent;
        }

        if (useColor) {
            _smallData << data._color.r;
            _smallData << data._color.g;
            _smallData << data._color.b;
            _smallData << data._color.a;
        }

        if (useTexcoords) {
            _smallData << data._texcoord.s;
            _smallData << data._texcoord.t;
        }

        if (useBoneData) {
            _smallData << data._weights.x;
            _smallData << data._weights.y;
            _smallData << data._weights.z;
            _smallData << data._weights.w;
            _smallData << data._indices.i;
        }
    }

    vectorAlg::shrinkToFit(_data);

    return std::make_pair((bufferPtr)_smallData.contents(), _smallData.size());
}

/// Create a dynamic or static VB
bool glVertexArray::create(bool staticDraw) {
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

    return VertexBuffer::create(staticDraw);
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexArray::refresh() {
    // Dynamic LOD elements (such as terrain) need dynamic indices
    // We can manually override index usage (again, used by the Terrain
    // rendering system)
    DIVIDE_ASSERT(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty(),
                  "glVertexArray::refresh error: Invalid index data on Refresh()!");

    GLsizei nSizeIndices =
        (GLsizei)(usesLargeIndices() ? _hardwareIndicesL.size() * sizeof(U32)
                                     : _hardwareIndicesS.size() * sizeof(U16));

    bool indicesChanged = (nSizeIndices != _prevSizeIndices);
    _prevSizeIndices = nSizeIndices;
    _refreshQueued = indicesChanged;

    /// Can only add attributes for now. No removal support. -Ionut
    for (U8 i = 0; i < to_uint(VertexAttribute::COUNT); ++i) {
        _useAttribute[i] = _useAttribute[i] || _attribDirty[i];
        if (!_refreshQueued) {
           if (_attribDirty[i]) {
               _refreshQueued = true;
           }
        }
    }
    _attribDirty.fill(false);

    _vaoHash = std::hash<AttribFlags>()(_useAttribute);

    Console::printfn("VAO HASH: %d", _vaoHash);

    _vaoCache = getVao(_vaoHash);
    if (_vaoCache == 0) {
        // Generate a "Vertex Array Object"
        glCreateVertexArrays(1, &_vaoCache);
        DIVIDE_ASSERT(_vaoCache != 0, Locale::get("ERROR_VAO_INIT"));
        setVao(_vaoHash, _vaoCache);
    } 
    std::pair<bufferPtr, size_t> bufferData = getMinimalData();
    // If any of the VBO's components changed size, we need to recreate the
    // entire buffer.

    GLsizei size = static_cast<GLsizei>(bufferData.second);
    bool sizeChanged = size != _prevSize;
    _prevSize = size;

    if (!_refreshQueued && !sizeChanged) {
        return false;
    }

    // Refresh buffer data (if this is the first call to refresh, this will be
    // true)
    if (sizeChanged) {
         Console::d_printfn(Locale::get("DISPLAY_VB_METRICS"), size,  4 * 1024 * 1024);
    }
    
    // Allocate sufficient space in our buffer
    glNamedBufferData(_VBid, size, bufferData.first, _usage);
    _smallData.clear();

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
    uploadVBAttributes();
    // Validate the buffer
    checkStatus();
    // Possibly clear client-side buffer for all non-required attributes?
    // foreach attribute if !required then delete else skip ?
    _refreshQueued = false;
    return true;
}

/// This method creates the initial VAO and VB OpenGL objects and queues a
/// Refresh call
bool glVertexArray::createInternal() {
    // Avoid double create calls
    DIVIDE_ASSERT(!_created,
                  "glVertexArray error: Attempted to double create a VB");
    _created = false;
    // Position data is a minim requirement
    if (_data.empty()) {
        Console::errorfn(Locale::get("ERROR_VB_POSITION"));
        return _created;
    }

    _formatInternal = GLUtil::glDataFormat[to_uint(_format)];
    // Generate a new Vertex Buffer Object
    glCreateBuffers(1, &_VBid);
    // Generate an "Index Buffer Object"
    glCreateBuffers(1, &_IBid);
    // Validate buffer creation
    DIVIDE_ASSERT(_VBid != 0, Locale::get("ERROR_VB_INIT"));
    // Assert if the IB creation failed
    DIVIDE_ASSERT(_IBid != 0, Locale::get("ERROR_IB_INIT"));
    // Calling refresh updates all stored information and sends it to the GPU
    _created = queueRefresh();
    return _created;
}

/// Render the current buffer data using the specified command
void glVertexArray::draw(const GenericDrawCommand& command,
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
    if (!setActive()) {
        return;
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

/// Set the current buffer as active
bool glVertexArray::setActive() {
    // Make sure we have valid data (buffer creation is deferred to the first
    // activate call)
    if (!_created) {
        if (!createInternal()) {
            return false;
        }
    }
    // Check if we have a refresh request queued up
    if (_refreshQueued) {
        if (!refresh()) {
            return false;
        }
    }

    // Bind the vertex array object that in turn activates all of the bindings
    // and pointers set on creation
    if (GL_API::setActiveVAO(_vaoCache)) {
        // If this is the first time the VAO is bound in the current loop, check
        // for primitive restart requests
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled);
    }

    // Bind the the vertex buffer and index buffer
    //GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _VBid);
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);

    glBindVertexBuffer(0, _VBid, 0, _bufferEntrySize);
    return true;
}

/// Activate and set all of the required vertex attributes.
void glVertexArray::uploadVBAttributes() {
    // Bind the current VAO to save our attributes
    GL_API::setActiveVAO(_vaoCache);
    static const U32 positionLoc = to_const_uint(AttribLocation::VERTEX_POSITION);
    static const U32 colorLoc = to_const_uint(AttribLocation::VERTEX_COLOR);
    static const U32 normalLoc = to_const_uint(AttribLocation::VERTEX_NORMAL);
    static const U32 texCoordLoc = to_const_uint(AttribLocation::VERTEX_TEXCOORD);
    static const U32 tangentLoc = to_const_uint(AttribLocation::VERTEX_TANGENT);
    static const U32 boneWeightLoc = to_const_uint(AttribLocation::VERTEX_BONE_WEIGHT);
    static const U32 boneIndiceLoc = to_const_uint(AttribLocation::VERTEX_BONE_INDICE);

    glEnableVertexAttribArray(positionLoc);
    glVertexAttribFormat(positionLoc, 3, GL_FLOAT, GL_FALSE, _attributeOffset[positionLoc]);
    glVertexAttribBinding(positionLoc, 0);

    if (_useAttribute[colorLoc]) {
        glEnableVertexAttribArray(colorLoc);
        glVertexAttribIFormat(colorLoc, 4, GL_UNSIGNED_BYTE, _attributeOffset[colorLoc]);
        glVertexAttribBinding(colorLoc, 0);
    } else {
        glDisableVertexAttribArray(colorLoc);
    }

    if (_useAttribute[normalLoc]) {
        glEnableVertexAttribArray(normalLoc);
        glVertexAttribFormat(normalLoc, 1, GL_FLOAT, GL_FALSE, _attributeOffset[normalLoc]);
        glVertexAttribBinding(normalLoc, 0);
    } else {
        glDisableVertexAttribArray(normalLoc);
    }

    if (_useAttribute[texCoordLoc]) {
        glEnableVertexAttribArray(texCoordLoc);
        glVertexAttribFormat(texCoordLoc, 2, GL_FLOAT, GL_FALSE, _attributeOffset[texCoordLoc]);
        glVertexAttribBinding(texCoordLoc, 0);
    } else {
        glDisableVertexAttribArray(texCoordLoc);
    }

    if (_useAttribute[tangentLoc]) {
        glEnableVertexAttribArray(tangentLoc);
        glVertexAttribFormat(tangentLoc, 1, GL_FLOAT, GL_FALSE, _attributeOffset[tangentLoc]);
        glVertexAttribBinding(tangentLoc, 0);
    } else {
        glDisableVertexAttribArray(tangentLoc);
    }

    if (_useAttribute[boneWeightLoc]) {
        assert(_useAttribute[boneIndiceLoc]);

        glEnableVertexAttribArray(boneWeightLoc);
        glEnableVertexAttribArray(boneIndiceLoc);
        glVertexAttribFormat(boneWeightLoc, 4, GL_FLOAT, GL_FALSE, _attributeOffset[boneWeightLoc]);
        glVertexAttribIFormat(boneIndiceLoc,1, GL_UNSIGNED_INT, _attributeOffset[boneIndiceLoc]);
        glVertexAttribBinding(boneWeightLoc, 0);
        glVertexAttribBinding(boneIndiceLoc, 0);
    } else {
        glDisableVertexAttribArray(boneWeightLoc);
        glDisableVertexAttribArray(boneIndiceLoc);
    }
}

/// Various post-create checks
void glVertexArray::checkStatus() {
}
};
