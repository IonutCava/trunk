#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
// Once vertex buffers reach a certain size, the for loop grows really really fast up to millions of iterations.
// Multiple if-checks per loop are not an option, so do some template hacks to speed this function up
template<bool normals, bool tangents, bool colour, bool texcoords, bool bones>
void fillSmallData5(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut)
{
    for (const VertexBuffer::Vertex& data : dataIn) {
        dataOut << data._position;

        if (normals) {
            dataOut << data._normal;
        }

        if (tangents) {
            dataOut << data._tangent;
        }

        if (colour) {
            dataOut << data._colour.r;
            dataOut << data._colour.g;
            dataOut << data._colour.b;
            dataOut << data._colour.a;
        }

        if (texcoords) {
            dataOut << data._texcoord.s;
            dataOut << data._texcoord.t;
        }

        if (bones) {
            dataOut << data._weights.i;
            dataOut << data._indices.i;
        }
    }
}

template <bool normals, bool tangents, bool colour, bool texCoords>
void fillSmallData4(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool bones)
{
    if (bones) {
        fillSmallData5<normals, tangents, colour, texCoords, true>(dataIn, dataOut);
    } else {
        fillSmallData5<normals, tangents, colour, texCoords, false>(dataIn, dataOut);
    }
}

template <bool normals, bool tangents, bool colour>
void fillSmallData3(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool texCoords, bool bones)
{
    if (texCoords) {
        fillSmallData4<normals, tangents, colour, true>(dataIn, dataOut, bones);
    } else {
        fillSmallData4<normals, tangents, colour, false>(dataIn, dataOut, bones);
    }
}

template <bool normals, bool tangents>
void fillSmallData2(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool colour, bool texCoords, bool bones)
{
    if (colour) {
        fillSmallData3<normals, tangents, true>(dataIn, dataOut, texCoords, bones);
    } else {
        fillSmallData3<normals, tangents, false>(dataIn, dataOut, texCoords, bones);
    }
}

template <bool normals>
void fillSmallData1(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool tangents, bool colour, bool texCoords, bool bones)
{
    if (tangents) {
        fillSmallData2<normals, true>(dataIn, dataOut, colour, texCoords, bones);
    } else {
        fillSmallData2<normals, false>(dataIn, dataOut, colour, texCoords, bones);
    }
}

void fillSmallData(const vectorImpl<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool normals, bool tangents, bool colour, bool texCoords, bool bones)
{
    if (normals) {
        fillSmallData1<true>(dataIn, dataOut, tangents, colour, texCoords, bones);
    } else {
        fillSmallData1<false>(dataIn, dataOut, tangents, colour, texCoords, bones);
    }
}
};

glVertexArray::VAOMap glVertexArray::_VAOMap;

IMPLEMENT_CUSTOM_ALLOCATOR(glVertexArray, 0, 0)

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
    ACKNOWLEDGE_UNUSED(result);
}

void glVertexArray::clearVaos() {
    for (VAOMap::value_type& value : _VAOMap) {
        if (value.second != 0) {
            GLUtil::_vaoPool.deallocate(value.second);
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

    for (U8 i = 0; i < to_const_U32(RenderPassType::COUNT); ++i) {
        _vaoCaches[i].fill(0);
        _vaoHashes[i].fill(0);
    }

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
}

glVertexArray::~glVertexArray()
{
    GLUtil::releaseVBO(_VBHandle._id, _VBHandle._offset);
}

void glVertexArray::reset() {
    _usage = GL_STATIC_DRAW;
    _prevSize = -1;
    _prevSizeIndices = -1;
    for (U8 i = 0; i < to_const_U32(RenderPassType::COUNT); ++i) {
        _vaoHashes[i].fill(0);
    }
    _useAttribute.fill(false);
    _attributeOffset.fill(0);
    VertexBuffer::reset();
}

/// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
std::pair<bufferPtr, size_t> glVertexArray::getMinimalData() {
    bool useColour    = _useAttribute[to_const_U32(VertexAttribute::ATTRIB_COLOR)];
    bool useNormals   = _useAttribute[to_const_U32(VertexAttribute::ATTRIB_NORMAL)];
    bool useTangents  = _useAttribute[to_const_U32(VertexAttribute::ATTRIB_TANGENT)];
    bool useTexcoords = _useAttribute[to_const_U32(VertexAttribute::ATTRIB_TEXCOORD)];
    bool useBoneData  = _useAttribute[to_const_U32(VertexAttribute::ATTRIB_BONE_INDICE)];

    size_t prevOffset = sizeof(vec3<F32>);
    if (useNormals) {
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_NORMAL)] = to_U32(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useTangents) {
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_TANGENT)] = to_U32(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useColour) {
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_COLOR)] = to_U32(prevOffset);
        prevOffset += sizeof(vec4<U8>);
    }

    if (useTexcoords) {
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_TEXCOORD)] = to_U32(prevOffset);
        prevOffset += sizeof(vec2<F32>);
    }

    if (useBoneData) {
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_BONE_WEIGHT)] = to_U32(prevOffset);
        prevOffset += sizeof(U32);
        _attributeOffset[to_const_U32(VertexAttribute::ATTRIB_BONE_INDICE)] = to_U32(prevOffset);
        prevOffset += sizeof(U32);
    }

    _effectiveEntrySize = static_cast<GLsizei>(prevOffset);

    _smallData.reserve(_data.size() * _effectiveEntrySize);

    fillSmallData(_data, _smallData, useNormals, useTangents, useColour, useTexcoords, useBoneData);

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
    assert(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty() &&
           "glVertexArray::refresh error: Invalid index data on Refresh()!");

    GLsizei nSizeIndices =
        (GLsizei)(usesLargeIndices() ? _hardwareIndicesL.size() * sizeof(U32)
                                     : _hardwareIndicesS.size() * sizeof(U16));

    bool indicesChanged = (nSizeIndices != _prevSizeIndices);
    _prevSizeIndices = nSizeIndices;
    _refreshQueued = indicesChanged;

    /// Can only add attributes for now. No removal support. -Ionut
    for (U8 i = 0; i < to_const_U32(VertexAttribute::COUNT); ++i) {
        _useAttribute[i] = _useAttribute[i] || _attribDirty[i];
        if (!_refreshQueued) {
           if (_attribDirty[i]) {
               _refreshQueued = true;
           }
        }
    }
    _attribDirty.fill(false);


    Console::printfn("VAO HASHES: ");
    std::array<bool, to_const_U32(RenderStage::COUNT)> vaoCachesDirty[to_const_U32(RenderPassType::COUNT)];
    std::array<AttribFlags, to_const_U32(RenderStage::COUNT)> attributesPerStage;
    for (U8 pass = 0; pass < to_const_U8(RenderPassType::COUNT); ++pass) {
        vaoCachesDirty[pass].fill(false);
        for (U8 i = 0; i < to_const_U8(RenderStage::COUNT); ++i) {
            const AttribFlags& stageMask = _attribMaskPerStage[pass][i];

            AttribFlags& stageUsage = attributesPerStage[i];
            for (U8 j = 0; j < to_const_U8(VertexAttribute::COUNT); ++j) {
                stageUsage[j] = _useAttribute[j] && stageMask[j];
            }
            size_t crtHash = std::hash<AttribFlags>()(stageUsage);
            _vaoHashes[pass][i] = crtHash;
            _vaoCaches[pass][i] = getVao(crtHash);
            if (_vaoCaches[pass][i] == 0) {
                // Generate a "Vertex Array Object"
                _vaoCaches[pass][i] = GLUtil::_vaoPool.allocate();
                assert(_vaoCaches[pass][i] != 0 && Locale::get(_ID("ERROR_VAO_INIT")));
                setVao(crtHash, _vaoCaches[pass][i]);
                vaoCachesDirty[pass][i] = true;
            }

            Console::printfn("      %d : %d (pass: %s)", i, to_U32(crtHash), (pass == 0 ? "PrePass" : "FwdPass"));
        }
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
        assert(_VBHandle._id != 0 && Locale::get(_ID("ERROR_VB_INIT")));
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

    vectorImpl<GLuint> vaos;
    for (U8 pass = 0; pass < to_const_U8(RenderPassType::COUNT); ++pass) {
        for (U8 i = 0; i < to_const_U8(RenderStage::COUNT); ++i) {
            if (vaoCachesDirty[pass][i]) {
                GLuint crtVao = _vaoCaches[pass][i];
                if (std::find_if(std::cbegin(vaos),
                                 std::cend(vaos),
                                     [crtVao](GLuint vao) {
                                         return crtVao == vao;
                                     }) == std::cend(vaos))
                {
                    vaos.push_back(crtVao);
                }
            }
        }
    }

    for (U8 i = 0; i < to_U8(vaos.size()); ++i) {
        // Set vertex attribute pointers
        uploadVBAttributes(vaos[i]);
    }

    for (U8 pass = 0; pass < to_const_U8(RenderPassType::COUNT); ++pass) {
        vaoCachesDirty[pass].fill(false);
    }
    
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
    assert(_VBHandle._id == 0 && "glVertexArray error: Attempted to double create a VB");
    // Position data is a minim requirement
    if (_data.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_VB_POSITION")));
        return false;
    }
    VertexBuffer::createInternal();

    _formatInternal = GLUtil::glDataFormat[to_U32(_format)];
    // Generate an "Index Buffer Object"
    glCreateBuffers(1, &_IBid);
    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_BUFFER,
                      _IBid,
                      -1,
                      Util::StringFormat("DVD_VAO_INDEX_BUFFER_%d", _IBid).c_str());
    }
    // Validate buffer creation
    // Assert if the IB creation failed
    assert(_IBid != 0 && Locale::get(_ID("ERROR_IB_INIT")));
    // Calling refresh updates all stored information and sends it to the GPU
    return queueRefresh();
}

/// Render the current buffer data using the specified command
void glVertexArray::draw(const GenericDrawCommand& command) {
    bool useCmdBuffer = command.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_INDIRECT);

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

    // Bind the vertex array object that in turn activates all of the bindings and pointers set on creation
    const RenderStagePass& stagePass = _context.getRenderStage();
    GLuint vao = _vaoCaches[to_U32(stagePass._passType)][to_U32(stagePass._stage)];
    if (GL_API::setActiveVAO(vao)) {
        // If this is the first time the VAO is bound in the current loop, check
        // for primitive restart requests
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled);
    }
    // Bind the the vertex buffer and index buffer
    GL_API::bindActiveBuffer(vao,
                             0,
                             _VBHandle._id,
                             _VBHandle._offset * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES,
                             _effectiveEntrySize);

    GLUtil::submitRenderCommand(command, useCmdBuffer, _formatInternal, _IBid);
}

/// Activate and set all of the required vertex attributes.
void glVertexArray::uploadVBAttributes(GLuint VAO) {
    // Bind the current VAO to save our attributes
    GL_API::setActiveVAO(VAO);
    static const U32 positionLoc = to_const_U32(AttribLocation::VERTEX_POSITION);
    static const U32 colourLoc = to_const_U32(AttribLocation::VERTEX_COLOR);
    static const U32 normalLoc = to_const_U32(AttribLocation::VERTEX_NORMAL);
    static const U32 texCoordLoc = to_const_U32(AttribLocation::VERTEX_TEXCOORD);
    static const U32 tangentLoc = to_const_U32(AttribLocation::VERTEX_TANGENT);
    static const U32 boneWeightLoc = to_const_U32(AttribLocation::VERTEX_BONE_WEIGHT);
    static const U32 boneIndiceLoc = to_const_U32(AttribLocation::VERTEX_BONE_INDICE);
    
    glEnableVertexAttribArray(positionLoc);
    glVertexAttribFormat(positionLoc, 3, GL_FLOAT, GL_FALSE, _attributeOffset[positionLoc]);
    glVertexAttribBinding(positionLoc, 0);

    if (_useAttribute[colourLoc]) {
        glEnableVertexAttribArray(colourLoc);
        glVertexAttribFormat(colourLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, _attributeOffset[colourLoc]);
        glVertexAttribBinding(colourLoc, 0);
    } else {
        glDisableVertexAttribArray(colourLoc);
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
