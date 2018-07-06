#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glVertexArray::VAOMap glVertexArray::_VAOMap;
vec3<U32> glVertexArray::_currentBindConfig;
vec3<U32> glVertexArray::_tempConfig;

IMPLEMENT_ALLOCATOR(glVertexArray, 0, 0)
bool glVertexArray::setIfDifferentBindRange(U32 VBOid, U32 offset, U32 size) {
    _tempConfig.set(VBOid, offset, size);
    if (_tempConfig != _currentBindConfig) {
        _currentBindConfig.set(_tempConfig);
        glBindVertexBuffer(0, VBOid, offset, size);
        return true;
    }

    return false;
}

void glVertexArray::cleanup() {
    clearVaos();
}


GLuint glVertexArray::getVao(size_t hash) {
    VAOMap::const_iterator result = _VAOMap.find(hash);
    return result != std::end(_VAOMap) ? result->second  : 0;
}

void glVertexArray::setVao(size_t hash, GLuint id) {
    std::pair<VAOMap::const_iterator, bool> result =
        hashAlg::emplace(_VAOMap, hash, id);
    assert(result.second);
}

void glVertexArray::clearVaos() {
    for (VAOMap::value_type& value : _VAOMap) {
        if (value.second != 0) {
            glDeleteVertexArrays(1, &value.second);
        }
    }
    _VAOMap.clear();
}

/// Default destructor
glVertexArray::glVertexArray(GFXDevice& context)
    : VertexBuffer(context),
      _refreshQueued(false),
      _formatInternal(GL_NONE)
{
    // We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    _prevSize = -1;
    _prevSizeIndices = -1;
    _effectiveEntrySize = -1;
    _IBid = 0;
    _vaoCaches.fill(0);
    _vaoHashes.fill(0);

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
}

glVertexArray::~glVertexArray()
{
    destroy();
}

void glVertexArray::reset() {
    _usage = GL_STATIC_DRAW;
    _prevSize = -1;
    _prevSizeIndices = -1;
    _vaoHashes.fill(0);

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
    VertexBuffer::reset();
}

/// Delete buffer
void glVertexArray::destroy() {
    GLUtil::releaseVBO(_VBHandle._id, _VBHandle._offset);
}

/// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
std::pair<bufferPtr, size_t> glVertexArray::getMinimalData() {
    bool useColor     = _useAttribute[to_const_uint(VertexAttribute::ATTRIB_COLOR)];
    bool useNormals   = _useAttribute[to_const_uint(VertexAttribute::ATTRIB_NORMAL)];
    bool useTangents  = _useAttribute[to_const_uint(VertexAttribute::ATTRIB_TANGENT)];
    bool useTexcoords = _useAttribute[to_const_uint(VertexAttribute::ATTRIB_TEXCOORD)];
    bool useBoneData  = _useAttribute[to_const_uint(VertexAttribute::ATTRIB_BONE_INDICE)];

    size_t prevOffset = sizeof(vec3<F32>);
    if (useNormals) {
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_NORMAL)] = to_uint(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useTangents) {
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_TANGENT)] = to_uint(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useColor) {
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_COLOR)] = to_uint(prevOffset);
        prevOffset += sizeof(vec4<U8>);
    }

    if (useTexcoords) {
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_TEXCOORD)] = to_uint(prevOffset);
        prevOffset += sizeof(vec2<F32>);
    }

    if (useBoneData) {
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] = to_uint(prevOffset);
        prevOffset += sizeof(U32);
        _attributeOffset[to_const_uint(VertexAttribute::ATTRIB_BONE_INDICE)] = to_uint(prevOffset);
        prevOffset += sizeof(U32);
    }

    _effectiveEntrySize = static_cast<GLsizei>(prevOffset);

    _smallData.reserve(_data.size() * _effectiveEntrySize);

    for (const Vertex& data : _data) {
        _smallData << data._position;

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
            _smallData << data._weights.i;
            _smallData << data._indices.i;
        }
    }

    if (_staticBuffer && !keepData()) {
        _data.clear();
    } else {
        vectorAlg::shrinkToFit(_data);
    }

    return std::make_pair((bufferPtr)_smallData.contents(), _smallData.size());
}

/// Create a dynamic or static VB
bool glVertexArray::create(bool staticDraw) {
    // If we want a dynamic buffer, then we are doing something outdated, such
    // as software skinning, or software water rendering
    if (!staticDraw) {
        // OpenGLES support isn't added, but this check doesn't break anything,
        // so I'll just leave it here -Ionut
        GLenum usage = (_context.getAPI() == RenderAPI::OpenGLES)
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
    for (U8 i = 0; i < to_const_uint(VertexAttribute::COUNT); ++i) {
        _useAttribute[i] = _useAttribute[i] || _attribDirty[i];
        if (!_refreshQueued) {
           if (_attribDirty[i]) {
               _refreshQueued = true;
           }
        }
    }
    _attribDirty.fill(false);


    Console::printfn("VAO HASHES: ");
    std::array<bool, to_const_uint(RenderStage::COUNT)> vaoCachesDirty;
    vaoCachesDirty.fill(false);
    std::array<AttribFlags, to_const_uint(RenderStage::COUNT)> attributesPerStage;
    for (U8 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        const AttribFlags& stageMask = _attribMaskPerStage[i];

        AttribFlags& stageUsage = attributesPerStage[i];
        for (U8 j = 0; j < to_const_uint(VertexAttribute::COUNT); ++j) {
            stageUsage[j] = _useAttribute[j] && stageMask[j];
        }
        size_t crtHash = std::hash<AttribFlags>()(stageUsage);
        _vaoHashes[i] = crtHash;
        _vaoCaches[i] = getVao(crtHash);
        if (_vaoCaches[i] == 0) {
            // Generate a "Vertex Array Object"
            glCreateVertexArrays(1, &_vaoCaches[i]);
            DIVIDE_ASSERT(_vaoCaches[i] != 0, Locale::get(_ID("ERROR_VAO_INIT")));
            setVao(crtHash, _vaoCaches[i]);
            vaoCachesDirty[i] = true;
        }

        Console::printfn("      %d : %d", i, to_uint(crtHash));
    }


    std::pair<bufferPtr, size_t> bufferData = getMinimalData();
    // If any of the VBO's components changed size, we need to recreate the
    // entire buffer.

    GLsizei size = static_cast<GLsizei>(bufferData.second);
    bool sizeChanged = size != _prevSize;
    bool needsReallocation = false;
    U32 countRequirement = GLUtil::VBO::getChunkCountForSize(size);
    if (sizeChanged) {
        needsReallocation = countRequirement > GLUtil::VBO::getChunkCountForSize(_prevSize);
        _prevSize = size;
    }

    if (!_refreshQueued && !sizeChanged) {
        return false;
    }

    if (_VBHandle._id == 0 || needsReallocation) {
        if (_VBHandle._id != 0) {
            GLUtil::releaseVBO(_VBHandle._id, _VBHandle._offset);
        }
        // Generate a new Vertex Buffer Object
        GLUtil::commitVBO(countRequirement,_usage, _VBHandle._id, _VBHandle._offset);
        DIVIDE_ASSERT(_VBHandle._id != 0, Locale::get(_ID("ERROR_VB_INIT")));
    }
    // Refresh buffer data (if this is the first call to refresh, this will be true)
    if (sizeChanged) {
        Console::d_printfn(Locale::get(_ID("DISPLAY_VB_METRICS")),
                           _VBHandle._id,
                           size,
                           countRequirement * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES,
                           GLUtil::getVBOMemUsage(_VBHandle._id),
                           GLUtil::getVBOCount());
    }

    // Allocate sufficient space in our buffer
    glNamedBufferSubData(_VBHandle._id, _VBHandle._offset * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES, size, bufferData.first);

    _smallData.clear();

    // Check if we need to update the IBO (will be true for the first Refresh() call)
    if (indicesChanged) {
        bufferPtr data = usesLargeIndices()
                             ? static_cast<bufferPtr>(_hardwareIndicesL.data())
                             : static_cast<bufferPtr>(_hardwareIndicesS.data());
        // Update our IB
        glNamedBufferData(_IBid, nSizeIndices, data, GL_STATIC_DRAW);
    }
    for (U8 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        if (vaoCachesDirty[i]) {
            // Set vertex attribute pointers
            uploadVBAttributes(i);
        }
    }
    vaoCachesDirty.fill(false);
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
    DIVIDE_ASSERT(_VBHandle._id == 0,
                  "glVertexArray error: Attempted to double create a VB");
    // Position data is a minim requirement
    if (_data.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_VB_POSITION")));
        return false;
    }
    VertexBuffer::createInternal();

    _formatInternal = GLUtil::glDataFormat[to_uint(_format)];
    // Generate an "Index Buffer Object"
    glCreateBuffers(1, &_IBid);
    // Validate buffer creation
    // Assert if the IB creation failed
    DIVIDE_ASSERT(_IBid != 0, Locale::get(_ID("ERROR_IB_INIT")));
    // Calling refresh updates all stored information and sends it to the GPU
    return queueRefresh();
}

/// Render the current buffer data using the specified command
void glVertexArray::draw(const GenericDrawCommand& command, bool useCmdBuffer) {
    // Make sure the buffer is current
    // Make sure we have valid data (buffer creation is deferred to the first activate call)
    if (_IBid == 0) {
        if (!createInternal()) {
            return;
        }
    }

    // Check if we have a refresh request queued up
    if (_refreshQueued) {
        if (!refresh()) {
            return;
        }
    }

    // Bind the vertex array object that in turn activates all of the bindings
    // and pointers set on creation
    if (GL_API::setActiveVAO(_vaoCaches[to_uint(_context.getRenderStage())])) {
        // If this is the first time the VAO is bound in the current loop, check
        // for primitive restart requests
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled);
        _currentBindConfig.reset();
    }

    // Bind the the vertex buffer and index buffer
    setIfDifferentBindRange(_VBHandle._id, _VBHandle._offset * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES, _effectiveEntrySize);
    GLUtil::submitRenderCommand(command, useCmdBuffer, _formatInternal, _IBid);
}

/// Activate and set all of the required vertex attributes.
void glVertexArray::uploadVBAttributes(U8 vaoIndex) {
    // Bind the current VAO to save our attributes
    GL_API::setActiveVAO(_vaoCaches[vaoIndex]);
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
        glVertexAttribFormat(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, _attributeOffset[colorLoc]);
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
        glVertexAttribFormat(boneWeightLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, _attributeOffset[boneWeightLoc]);
        glVertexAttribIFormat(boneIndiceLoc, 4, GL_UNSIGNED_BYTE, _attributeOffset[boneIndiceLoc]);
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
