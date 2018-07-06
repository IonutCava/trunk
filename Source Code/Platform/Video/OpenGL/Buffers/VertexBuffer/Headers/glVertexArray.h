/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _GL_VERTEX_ARRAY_H_
#define _GL_VERTEX_ARRAY_H_

#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Headers/ByteBuffer.h"

/// Always bind a shader, even a dummy one when rendering geometry. No more
/// fixed matrix API means no more VBs or VAs
/// One VAO contains: one VB for data, one IB for indices and uploads to the
/// shader vertex attribs for:
///- Vertex Data  bound to location Divide::VERTEX_POSITION
///- Colors       bound to location Divide::VERTEX_COLOR
///- Normals      bound to location Divide::VERTEX_NORMAL
///- TexCoords    bound to location Divide::VERTEX_TEXCOORD
///- Tangents     bound to location Divide::VERTEX_TANGENT
///- Bone weights bound to location Divide::VERTEX_BONE_WEIGHT
///- Bone indices bound to location Divide::VERTEX_BONE_INDICE
///- Line width   bound to location Divide::VERTEX_WIDTH

namespace Divide {

class glVertexArray : public VertexBuffer {
    DECLARE_ALLOCATOR
   public:
    glVertexArray(GFXDevice& context);
    ~glVertexArray();

    bool create(bool staticDraw = true);
    void destroy();

    /// Never call Refresh() just queue it and the data will update before
    /// drawing
    inline bool queueRefresh() {
        _refreshQueued = true;
        return true;
    }

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& commands,
              bool useCmdBuffer = false);

   protected:
    friend class GL_API;
    void reset() override;
    /// If we have a shader, we create a VAO, if not, we use simple VB + IB. If
    /// that fails, use VA
    bool refresh();
    /// Internally create the VB
    bool createInternal();
    /// Enable full VAO based VB (all pointers are tracked by VAO's)
    void uploadVBAttributes(U8 vaoIndex);
    /// Integrity checks
    void checkStatus();
    /// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
    std::pair<bufferPtr, size_t> getMinimalData();

    static GLuint getVao(U32 hash);
    static void setVao(U32 hash, GLuint id);
    static void clearVaos();

    static void cleanup();

    static bool setIfDifferentBindRange(U32 VBOid, U32 offset, U32 size);
   protected:
    GLenum _formatInternal;
    GLuint _IBid;
    // VB GL ID and Offset.
    // This could easily be a std::pair, but having names for variables makes everything clearer
    GLUtil::AllocationHandle _VBHandle;
    GLenum _usage;
    ///< A refresh call might be called before "Create()". This should help with that
    bool _refreshQueued;  
    GLsizei _prevSize;
    GLsizei _prevSizeIndices;
    GLsizei _effectiveEntrySize;
    ByteBuffer _smallData;
    AttribFlags _useAttribute;
    typedef std::array<GLuint, to_const_uint(VertexAttribute::COUNT)> AttribValues;
    AttribValues _attributeOffset;

    std::array<U32, to_const_uint(RenderStage::COUNT)> _vaoHashes;
    std::array<GLuint, to_const_uint(RenderStage::COUNT)> _vaoCaches;
    typedef hashMapImpl<U32, GLuint> VAOMap;
    static VAOMap _VAOMap;

    static vec3<U32> _currentBindConfig;
    static vec3<U32> _tempConfig;
};

};  // namespace Divide

#endif
