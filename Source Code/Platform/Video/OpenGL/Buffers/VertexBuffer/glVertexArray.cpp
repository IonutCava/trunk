#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    class VBO {
       public:

        static const U32 MAX_VBO_CHUNK_SIZE_BYTES = 512 * 1024;
        //nVidia recommended (years ago) to use up to 4 megs per VBO
        static const U32 MAX_VBO_CHUNK_COUNT = 8; 
        static const U32 MAX_VBO_SIZE_BYTES = MAX_VBO_CHUNK_SIZE_BYTES * MAX_VBO_CHUNK_COUNT;
        //keep track of what chunks we are using
        //for each chunk, keep track how many next chunks are also part of the same allocation
        std::array<std::pair<bool, U32>, MAX_VBO_CHUNK_COUNT> _chunkUsageState;

        static U32 getChunkCountForSize(size_t sizeInBytes) {
            return to_uint(std::ceil(to_float(sizeInBytes) / MAX_VBO_CHUNK_SIZE_BYTES));
        }

        VBO() : _handle(0),
            _usage(GL_NONE)
        {
        }

        ~VBO()
        {
            if (_handle != 0) {
                GLUtil::freeBuffer(_handle);
            }
        }

        U32 handle() {
            return _handle;
        }

        bool checkChunksAvailability(U32 offset, U32 count) {
            // Currently hardcoded to use for a single VBO per glVertexArray
            for (std::pair<bool, U32>& chunk : _chunkUsageState) {
                if (chunk.first) {
                    return false;
                }
                return true;
            }

            std::pair<bool, U32>& chunk = _chunkUsageState[offset];
            U32 freeChunkCount = 0;
            if (!chunk.first) {
                freeChunkCount++;
                for (U32 j = 1; j < MAX_VBO_CHUNK_COUNT - offset; ++j) {
                    std::pair<bool, U32>& chunkChild = _chunkUsageState[offset + j];
                    if (chunkChild.first) {
                        break;
                    } else {
                        freeChunkCount++;
                    }
                }
            }

            return freeChunkCount >= count;
        }

        bool allocateChunks(U32 count, GLenum usage, U32& offsetOut) {
            assert(count < MAX_VBO_CHUNK_COUNT);

            if (_usage == GL_NONE || _usage == usage) {
                for (U32 i = 0; i < MAX_VBO_CHUNK_COUNT; ++i) {
                    if (checkChunksAvailability(i, count)) {
                        if (_handle == 0) {
                            GLUtil::createAndAllocBuffer(count * MAX_VBO_CHUNK_SIZE_BYTES, usage, _handle);
                            _usage = usage;
                        }
                        offsetOut = i;
                        _chunkUsageState[i].first = true;
                        _chunkUsageState[i].second = count;
                        for (U32 j = 1; j < count; ++j) {
                            _chunkUsageState[j + i].first = true;
                        } 
                        return true;
                    }
                }
            }

            return false;
        }

        bool releaseChunks(U32 offset) {
            assert(offset < MAX_VBO_CHUNK_COUNT);

            std::pair<bool, U32>& chunk = _chunkUsageState[offset];
            if (chunk.first) {
                chunk.first = false;
                for (U32 i = 0; i < chunk.second; ++i) {
                    std::pair<bool, U32>& chunkChild = _chunkUsageState[i + offset + 1];
                    assert(chunkChild.first && chunkChild.second == 0);
                    chunkChild.first = false;
                    chunkChild.second = 0;
                }
                chunk.second = 0;
                return true;
            }

            return false;
        }

    private:
        GLuint _handle;
        GLenum _usage;
    };

    static vectorImpl<VBO> g_globalVBOs;

    static bool commitVBO(U32 chunkCount, GLenum usage, GLuint& handleOut, U32& offsetOut) {
        for (U32 i = 0; i < g_globalVBOs.size() + 2; ++i) {
            for (VBO& vbo : g_globalVBOs) {
                if (vbo.allocateChunks(chunkCount, usage, offsetOut)) {
                    handleOut = vbo.handle();
                    return true;
                }
            }
            g_globalVBOs.emplace_back();
        }

        return false;
    }

    static bool releaseVBO(GLuint& handle, U32 offset) {
        for (VBO& vbo : g_globalVBOs) {
            if (vbo.handle() == handle) {
                handle = 0;
                return vbo.releaseChunks(offset);
            }
        }

        return false;
    }

    vec3<U32> g_currentBindConfig;
    vec3<U32> g_tempConfig;
    bool setIfDifferentBindRange(U32 UBOid, U32 offset, U32 size) {
        g_tempConfig.set(UBOid, offset, size);
        if (g_tempConfig != g_currentBindConfig) {
            g_currentBindConfig.set(g_tempConfig);
            return true;
        }

        return false;
    }
};

glVertexArray::VAOMap glVertexArray::_VAOMap;

void glVertexArray::cleanup() {
    clearVaos();
    g_globalVBOs.clear();
}


GLuint glVertexArray::getVao(U32 hash) {
    VAOMap::const_iterator result = _VAOMap.find(hash);
    return result != std::end(_VAOMap) ? result->second
                                       : 0;
}

void glVertexArray::setVao(U32 hash, GLuint id) {
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
    _IBid = 0;
    _VBHandle.first = 0;
    _VBHandle.second = 0;
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
    _bufferEntrySize = -1;
    _vaoHashes.fill(0);

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
    VertexBuffer::reset();
}

/// Delete buffer
void glVertexArray::destroy() {
    GLUtil::freeBuffer(_IBid);
    releaseVBO(_VBHandle.first, _VBHandle.second);
}

/// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
std::pair<bufferPtr, size_t> glVertexArray::getMinimalData() {
    bool useColor     = _useAttribute[to_uint(VertexAttribute::ATTRIB_COLOR)];
    bool useNormals   = _useAttribute[to_uint(VertexAttribute::ATTRIB_NORMAL)];
    bool useTangents  = _useAttribute[to_uint(VertexAttribute::ATTRIB_TANGENT)];
    bool useTexcoords = _useAttribute[to_uint(VertexAttribute::ATTRIB_TEXCOORD)];
    bool useBoneData  = _useAttribute[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)];

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
        prevOffset += sizeof(U32);
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
            _smallData << data._weights.i;
            _smallData << data._indices.i;
        }
    }

    if (_staticBuffer) {
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


    Console::printfn("VAO HASHES: ");
    std::array<bool, to_const_uint(RenderStage::COUNT)> vaoCachesDirty;
    vaoCachesDirty.fill(false);
    std::array<AttribFlags, to_const_uint(RenderStage::COUNT)> attributesPerStage;
    for (U8 i = 0; i < to_uint(RenderStage::COUNT); ++i) {
        const AttribFlags& stageMask = _attribMaskPerStage[i];

        AttribFlags& stageUsage = attributesPerStage[i];
        for (U8 j = 0; j < to_uint(VertexAttribute::COUNT); ++j) {
            stageUsage[j] = _useAttribute[j] && stageMask[j];
        }
        U32 crtHash = to_uint(std::hash<AttribFlags>()(stageUsage));
        _vaoHashes[i] = crtHash;
        _vaoCaches[i] = getVao(crtHash);
        if (_vaoCaches[i] == 0) {
            // Generate a "Vertex Array Object"
            glCreateVertexArrays(1, &_vaoCaches[i]);
            DIVIDE_ASSERT(_vaoCaches[i] != 0, Locale::get("ERROR_VAO_INIT"));
            setVao(crtHash, _vaoCaches[i]);
            vaoCachesDirty[i] = true;
        }

        Console::printfn("      %d : %d", i, crtHash);
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
         Console::d_printfn(Locale::get("DISPLAY_VB_METRICS"), size, VBO::MAX_VBO_CHUNK_SIZE_BYTES);
    }
    
    if (_VBHandle.first == 0) {
        // Generate a new Vertex Buffer Object
        commitVBO(VBO::getChunkCountForSize(size),_usage, _VBHandle.first, _VBHandle.second);
        DIVIDE_ASSERT(_VBHandle.first != 0, Locale::get("ERROR_VB_INIT"));
        // Allocate sufficient space in our buffer
        glNamedBufferSubData(_VBHandle.first, _VBHandle.second * VBO::MAX_VBO_CHUNK_SIZE_BYTES, size, bufferData.first);
    }

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
    for (U8 i = 0; i < to_uint(RenderStage::COUNT); ++i) {
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
    DIVIDE_ASSERT(_VBHandle.first == 0,
                  "glVertexArray error: Attempted to double create a VB");
    // Position data is a minim requirement
    if (_data.empty()) {
        Console::errorfn(Locale::get("ERROR_VB_POSITION"));
        return false;
    }

    _formatInternal = GLUtil::glDataFormat[to_uint(_format)];
    // Generate an "Index Buffer Object"
    glCreateBuffers(1, &_IBid);
    // Validate buffer creation
    // Assert if the IB creation failed
    DIVIDE_ASSERT(_IBid != 0, Locale::get("ERROR_IB_INIT"));
    // Calling refresh updates all stored information and sends it to the GPU
    return queueRefresh();
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
    // Get the OpenGL specific command from the generic one
    const IndirectDrawCommand& cmd = command.cmd();
    // Instance count can be generated programmatically,
    // so we need to make sure it's valid
    if (cmd.primCount == 0) {
        return;
    }
    // Make sure the buffer is current
    if (!setActive()) {
        return;
    }
  
    static const size_t cmdSize = sizeof(IndirectDrawCommand);

    bufferPtr offset = (bufferPtr)(cmd.baseInstance * cmdSize);
    U16 drawCount = command.drawCount();
    GLenum mode = GLUtil::glPrimitiveTypeTable[to_uint(command.primitiveType())];

    if (useCmdBuffer) {
        if (command.renderGeometry()) {
            glMultiDrawElementsIndirect(mode, _formatInternal, offset, drawCount, cmdSize);
        }
        if (command.renderWireframe()) {
            glMultiDrawElementsIndirect(GL_LINE_LOOP, _formatInternal, offset, drawCount, cmdSize);
        }
    } else {
        if (drawCount > 1) {
            vectorImpl<GLsizei> count(drawCount, cmd.indexCount);
            vectorImpl<U32> indices(drawCount, cmd.firstIndex);
            if (command.renderGeometry()) {
                glMultiDrawElements(mode, count.data(), _formatInternal, (bufferPtr*)indices.data(), drawCount);
            }
            if (command.renderWireframe()) {
                glMultiDrawElements(GL_LINE_LOOP, count.data(), _formatInternal, (bufferPtr*)indices.data(), drawCount);
            }
        } else {
            if (command.renderGeometry()) {
                glDrawElements(mode, cmd.indexCount, _formatInternal, bufferOffset(cmd.firstIndex));
            }
            if (command.renderWireframe()) {
                glDrawElements(GL_LINE_LOOP, cmd.indexCount, _formatInternal, bufferOffset(cmd.firstIndex));
            }
        }
    }
}

/// Set the current buffer as active
bool glVertexArray::setActive() {
    // Make sure we have valid data (buffer creation is deferred to the first
    // activate call)
    if (_VBHandle.first == 0) {
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
    if (GL_API::setActiveVAO(_vaoCaches[to_uint(GFX_DEVICE.getRenderStage())])) {
        // If this is the first time the VAO is bound in the current loop, check
        // for primitive restart requests
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled);
    }

    // Bind the the vertex buffer and index buffer
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);
    if (setIfDifferentBindRange(_VBHandle.first, _VBHandle.second * VBO::MAX_VBO_CHUNK_SIZE_BYTES, _bufferEntrySize)) {
        glBindVertexBuffer(0, _VBHandle.first, _VBHandle.second * VBO::MAX_VBO_CHUNK_SIZE_BYTES, _bufferEntrySize);
    }
    return true;
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
