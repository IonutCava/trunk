#include "stdafx.h"

#include "Headers/glVertexArray.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
// Once vertex buffers reach a certain size, the for loop grows really really fast up to millions of iterations.
// Multiple if-checks per loop are not an option, so do some template hacks to speed this function up
template<bool texCoords, bool normals, bool tangents, bool colour, bool bones>
void fillSmallData5(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut)
{
    for (const VertexBuffer::Vertex& data : dataIn) {
        dataOut << data._position;

        if (texCoords) {
            dataOut << data._texcoord.s;
            dataOut << data._texcoord.t;
        }

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

        if (bones) {
            dataOut << data._weights.i;
            dataOut << data._indices.i;
        }
    }
}

template <bool texCoords, bool normals, bool tangents, bool colour>
void fillSmallData4(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool bones)
{
    if (bones) {
        fillSmallData5<texCoords, normals, tangents, colour, true>(dataIn, dataOut);
    } else {
        fillSmallData5<texCoords, normals, tangents, colour, false>(dataIn, dataOut);
    }
}

template <bool texCoords, bool normals, bool tangents>
void fillSmallData3(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool colour, bool bones)
{
    if (colour) {
        fillSmallData4<texCoords, normals, tangents, true>(dataIn, dataOut, bones);
    } else {
        fillSmallData4<texCoords, normals, tangents, false>(dataIn, dataOut, bones);
    }
}

template <bool texCoords, bool normals>
void fillSmallData2(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool tangents, bool colour, bool bones)
{
    if (tangents) {
        fillSmallData3<texCoords, normals, true>(dataIn, dataOut, colour, bones);
    } else {
        fillSmallData3<texCoords, normals, false>(dataIn, dataOut, colour, bones);
    }
}

template <bool texCoords>
void fillSmallData1(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool normals, bool tangents, bool colour, bool bones)
{
    if (normals) {
        fillSmallData2<texCoords, true>(dataIn, dataOut, tangents, colour, bones);
    } else {
        fillSmallData2<texCoords, false>(dataIn, dataOut, tangents, colour, bones);
    }
}

void fillSmallData(const vector<VertexBuffer::Vertex>& dataIn, ByteBuffer& dataOut, bool texCoords, bool normals, bool tangents, bool colour, bool bones)
{
    if (texCoords) {
        fillSmallData1<true>(dataIn, dataOut, normals, tangents, colour, bones);
    } else {
        fillSmallData1<false>(dataIn, dataOut, normals, tangents, colour, bones);
    }
}
};

GLUtil::glVAOCache glVertexArray::_VAOMap;

void glVertexArray::cleanup() {
    _VAOMap.clear();
}

/// Default destructor
glVertexArray::glVertexArray(GFXDevice& context)
    : VertexBuffer(context),
      _refreshQueued(false),
      _drawIndexed(true),
      _formatInternal(GL_NONE)
{
    // We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    _prevSize = -1;
    _prevSizeIndices = -1;
    _effectiveEntrySize = -1;
    _IBid = 0;
    _lastDrawCount = 0;
    _lastIndexCount = 0;
    _lastFirstIndex = 0;
    _vaoCaches.fill(0);

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

    _useAttribute.fill(false);
    _attributeOffset.fill(0);
    VertexBuffer::reset();
}

/// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
std::pair<bufferPtr, size_t> glVertexArray::getMinimalData() {
    bool useTexcoords = _useAttribute[to_base(AttribLocation::TEXCOORD)];
    bool useNormals   = _useAttribute[to_base(AttribLocation::NORMAL)];
    bool useTangents  = _useAttribute[to_base(AttribLocation::TANGENT)];
    bool useColour    = _useAttribute[to_base(AttribLocation::COLOR)];
    bool useBoneData  = _useAttribute[to_base(AttribLocation::BONE_INDICE)];

    size_t prevOffset = sizeof(vec3<F32>);

    if (useTexcoords) {
        _attributeOffset[to_base(AttribLocation::TEXCOORD)] = to_U32(prevOffset);
        prevOffset += sizeof(vec2<F32>);
    }

    if (useNormals) {
        _attributeOffset[to_base(AttribLocation::NORMAL)] = to_U32(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useTangents) {
        _attributeOffset[to_base(AttribLocation::TANGENT)] = to_U32(prevOffset);
        prevOffset += sizeof(F32);
    }

    if (useColour) {
        _attributeOffset[to_base(AttribLocation::COLOR)] = to_U32(prevOffset);
        prevOffset += sizeof(UColour);
    }

    if (useBoneData) {
        _attributeOffset[to_base(AttribLocation::BONE_WEIGHT)] = to_U32(prevOffset);
        prevOffset += sizeof(U32);
        _attributeOffset[to_base(AttribLocation::BONE_INDICE)] = to_U32(prevOffset);
        prevOffset += sizeof(U32);
    }

    _effectiveEntrySize = static_cast<GLsizei>(prevOffset);

    _smallData.reserve(_data.size() * _effectiveEntrySize);

    fillSmallData(_data, _smallData, useTexcoords, useNormals, useTangents, useColour, useBoneData);

    if (_staticBuffer && !keepData()) {
        _data.clear();
    } else {
        _data.shrink_to_fit();
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
    for (U8 i = 0; i < to_base(AttribLocation::COUNT); ++i) {
        _useAttribute[i] = _useAttribute[i] || _attribDirty[i];
        if (!_refreshQueued) {
           if (_attribDirty[i]) {
               _refreshQueued = true;
           }
        }
    }
    _attribDirty.fill(false);


    Console::printfn("VAO HASHES: ");
    std::array<bool, to_base(RenderStagePass::count())> vaoCachesDirty;
    std::array<AttribFlags, to_base(RenderStagePass::count())> attributesPerStage;

    vaoCachesDirty.fill(false);
    for (RenderStagePass::StagePassIndex i = 0; i < RenderStagePass::count(); ++i) {
        const AttribFlags& stageMask = _attribMasks[i];

        AttribFlags& stageUsage = attributesPerStage[i];
        for (U8 j = 0; j < to_base(AttribLocation::COUNT); ++j) {
            stageUsage[j] = _useAttribute[j] && stageMask[j];
        }

        size_t crtHash = 0;
        // Dirty on a VAO map cache miss
        vaoCachesDirty[i] = !_VAOMap.getVAO(stageUsage, _vaoCaches[i], crtHash);
        Console::printfn("      %s : %zu (pass: %s)", TypeUtil::renderStageToString(RenderStagePass::stage(i)), crtHash, TypeUtil::renderPassTypeToString(RenderStagePass::pass(i)));    }

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

    vectorEASTL<GLuint> vaos;
    vaos.reserve(RenderStagePass::count());

    for (RenderStagePass::StagePassIndex pass = 0; pass < RenderStagePass::count(); ++pass) {
        if (vaoCachesDirty[pass]) {
            GLuint crtVao = _vaoCaches[pass];
            if (eastl::find_if(eastl::cbegin(vaos),
                               eastl::cend(vaos),
                               [crtVao](GLuint vao) {
                                   return crtVao == vao;
                               }) == eastl::cend(vaos))
            {
                vaos.push_back(crtVao);
            }
        }
    }

    for (U8 i = 0; i < to_U8(vaos.size()); ++i) {
        // Set vertex attribute pointers
        uploadVBAttributes(vaos[i]);
    }

    vaoCachesDirty.fill(false);

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
    bool useCmdBuffer = isEnabledOption(command, CmdRenderOptions::RENDER_INDIRECT);

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
    GLuint vao = _vaoCaches[command._bufferIndex];
    GL_API::getStateTracker().setActiveVAO(vao);
    GL_API::getStateTracker().togglePrimitiveRestart(_primitiveRestartEnabled);
    // VAOs store vertex formats and are reused by multiple 3d objects, so the Index Buffer and Vertex Buffers need to be double checked
    GL_API::getStateTracker().setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);
    GL_API::getStateTracker().bindActiveBuffer(vao, 0, _VBHandle._id, _VBHandle._offset * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES, _effectiveEntrySize);

    if (useCmdBuffer) {
        GLUtil::submitRenderCommand(command, _drawIndexed, useCmdBuffer, _formatInternal);
    } else {
        rebuildCountAndIndexData(command._drawCount, command._cmd.indexCount, command._cmd.firstIndex);
        GLUtil::submitRenderCommand(command, _drawIndexed, useCmdBuffer, _formatInternal, _countData.data(), (bufferPtr)_indexOffsetData.data());
    }
}

void glVertexArray::rebuildCountAndIndexData(U32 drawCount, U32 indexCount, U32 firstIndex) {
    STUBBED("ToDo: Move all of this somewhere outside of glVertexArray so that we can gather proper data from all of the batched commands -Ionut");

    if (_lastDrawCount == drawCount && _lastIndexCount == indexCount && _lastFirstIndex == firstIndex) {
        return;
    }

    if (_lastDrawCount != drawCount || _lastIndexCount != indexCount) {
        eastl::fill(eastl::begin(_countData), eastl::begin(_countData) + drawCount, indexCount);
    }

    if (_lastDrawCount != drawCount || _lastFirstIndex != firstIndex) {
        U32 idxCount = drawCount * getIndexCount();
        if (_indexOffsetData.size() < idxCount) {
            _indexOffsetData.resize(idxCount, firstIndex);
        }
        if (_lastFirstIndex != firstIndex) {
            eastl::fill(eastl::begin(_indexOffsetData), eastl::end(_indexOffsetData), firstIndex);
        }
    }
    _lastDrawCount = drawCount;
    _lastIndexCount = indexCount;
    _lastFirstIndex = firstIndex;
}

/// Activate and set all of the required vertex attributes.
void glVertexArray::uploadVBAttributes(GLuint VAO) {
    // Bind the current VAO to save our attributes
    GL_API::getStateTracker().setActiveVAO(VAO);
    // Bind the vertex buffer and index buffer
    GL_API::getStateTracker().bindActiveBuffer(VAO,
                                               0,
                                               _VBHandle._id,
                                               _VBHandle._offset * GLUtil::VBO::MAX_VBO_CHUNK_SIZE_BYTES,
                                               _effectiveEntrySize);

    static const U32 positionLoc = to_base(AttribLocation::POSITION);
    static const U32 texCoordLoc = to_base(AttribLocation::TEXCOORD);
    static const U32 normalLoc   = to_base(AttribLocation::NORMAL);
    static const U32 tangentLoc = to_base(AttribLocation::TANGENT);
    static const U32 colourLoc     = to_base(AttribLocation::COLOR);
    static const U32 boneWeightLoc = to_base(AttribLocation::BONE_WEIGHT);
    static const U32 boneIndiceLoc = to_base(AttribLocation::BONE_INDICE);

    glEnableVertexAttribArray(positionLoc);
    glVertexAttribFormat(positionLoc, 3, GL_FLOAT, GL_FALSE, _attributeOffset[positionLoc]);
    glVertexAttribBinding(positionLoc, 0);

    if (_useAttribute[texCoordLoc]) {
        glEnableVertexAttribArray(texCoordLoc);
        glVertexAttribFormat(texCoordLoc, 2, GL_FLOAT, GL_FALSE, _attributeOffset[texCoordLoc]);
        glVertexAttribBinding(texCoordLoc, 0);
    } else {
        glDisableVertexAttribArray(texCoordLoc);
    }

    if (_useAttribute[normalLoc]) {
        glEnableVertexAttribArray(normalLoc);
        glVertexAttribFormat(normalLoc, 1, GL_FLOAT, GL_FALSE, _attributeOffset[normalLoc]);
        glVertexAttribBinding(normalLoc, 0);
    } else {
        glDisableVertexAttribArray(normalLoc);
    }

    if (_useAttribute[tangentLoc]) {
        glEnableVertexAttribArray(tangentLoc);
        glVertexAttribFormat(tangentLoc, 1, GL_FLOAT, GL_FALSE, _attributeOffset[tangentLoc]);
        glVertexAttribBinding(tangentLoc, 0);
    } else {
        glDisableVertexAttribArray(tangentLoc);
    }

    if (_useAttribute[colourLoc]) {
        glEnableVertexAttribArray(colourLoc);
        glVertexAttribFormat(colourLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, _attributeOffset[colourLoc]);
        glVertexAttribBinding(colourLoc, 0);
    } else {
        glDisableVertexAttribArray(colourLoc);
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

};
