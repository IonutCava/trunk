/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _GL_VERTEX_ARRAY_H_
#define _GL_VERTEX_ARRAY_H_

#include "Core/Headers/ByteBuffer.h"

#include "glVAOCache.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glMemoryManager.h"

#include <EASTL/fixed_vector.h>

/// Always bind a shader, even a dummy one when rendering geometry. No more
/// fixed matrix API means no more VBs or VAs
/// One VAO contains: one VB for data, one IB for indices and uploads to the
/// shader vertex attribs for:
///- Vertex Data  bound to location Divide::VERTEX_POSITION
///- Colours      bound to location Divide::VERTEX_COLOR
///- Normals      bound to location Divide::VERTEX_NORMAL
///- TexCoords    bound to location Divide::VERTEX_TEXCOORD
///- Tangents     bound to location Divide::VERTEX_TANGENT
///- Bone weights bound to location Divide::VERTEX_BONE_WEIGHT
///- Bone indices bound to location Divide::VERTEX_BONE_INDICE
///- Line width   bound to location Divide::VERTEX_WIDTH

namespace Divide {

class glVertexArray final : public VertexBuffer {
   public:
    explicit glVertexArray(GFXDevice& context);
    ~glVertexArray();

    bool create(bool staticDraw = true);

    /// Never call Refresh() just queue it and the data will update before
    /// drawing
    inline bool queueRefresh() {
        _refreshQueued = true;
        return true;
    }

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& commands, I32 passIdx = -1) override;

   protected:
    friend class GL_API;
    void reset() override;
    /// Prepare data for upload
    bool refresh();
    /// Create vao objects for the current pass
    void upload(I32 passIdx);
    /// Internally create the VB
    bool createInternal();
    /// Enable full VAO based VB (all pointers are tracked by VAO's)
    void uploadVBAttributes(GLuint VAO);
    /// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
    std::pair<bufferPtr, size_t> getMinimalData();

    static void cleanup();

    void rebuildCountAndIndexData(U32 drawCount, U32 indexCount, U32 firstIndex);

   protected:
    GLenum _formatInternal;
    GLuint _IBid;
    // VB GL ID and Offset.
    // This could easily be a std::pair, but having names for variables makes everything clearer
    GLUtil::AllocationHandle _VBHandle;
    GLenum _usage;
    ///< A refresh call might be called before "Create()". This should help with that
    bool _refreshQueued;  
    bool _uploadQueued;
    GLsizei _prevSize;
    GLsizei _prevSizeIndices;
    GLsizei _effectiveEntrySize;
    ByteBuffer _smallData;
    AttribFlags _useAttribute;
    typedef std::array<GLuint, to_base(AttribLocation::COUNT)> AttribValues;
    AttribValues _attributeOffset;

    // Both for forward pass and pre-pass
    std::array<GLuint, to_base(RenderStagePass::count())> _vaoCaches;
    
    static GLUtil::glVAOCache _VAOMap;

    bool _drawIndexed;
    U32 _lastDrawCount = 0;
    U32 _lastIndexCount = 0;
    U32 _lastFirstIndex = 0;

    std::array<GLsizei, Config::MAX_VISIBLE_NODES> _countData;
    eastl::fixed_vector<GLuint, Config::MAX_VISIBLE_NODES * 256> _indexOffsetData;
};

};  // namespace Divide

#endif
