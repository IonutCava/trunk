/*
   Copyright (c) 2015 DIVIDE-Studio
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
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Platform/Headers/ByteBuffer.h"

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
   public:
    glVertexArray();
    ~glVertexArray();

    bool create(bool staticDraw = true);
    void destroy();

    virtual bool setActive();

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
    /// If we have a shader, we create a VAO, if not, we use simple VB + IB. If
    /// that fails, use VA
    bool refresh();
    /// Internally create the VB
    bool createInternal();
    /// Enable full VAO based VB (all pointers are tracked by VAO's)
    void uploadVBAttributes();
    /// Integrity checks
    void checkStatus();
    /// Trim down the Vertex vector to only upload the minimal ammount of data to the GPU
    std::pair<bufferPtr, size_t> getMinimalData();

    static GLuint getVao(size_t hash);
    static void setVao(size_t hash, GLuint id);
    static void clearVaos();

   protected:
    GLenum _formatInternal;
    GLuint _IBid;
    GLuint _VBid;
    GLenum _usage;
    bool _refreshQueued;  ///< A refresh call might be called before "Create()".
                          ///This should help with that
    GLsizei _prevSize;
    GLsizei _prevSizeIndices;
    GLsizei _bufferEntrySize;
    ByteBuffer _smallData;
    typedef std::array<bool, to_const_uint(VertexAttribute::COUNT)> AttribFlags;
    AttribFlags _useAttribute;
    typedef std::array<GLuint, to_const_uint(VertexAttribute::COUNT)> AttribValues;
    AttribValues _attributeOffset;

    size_t _vaoHash;
    GLuint _vaoCache;
    typedef hashMapImpl<size_t, GLuint> VaoMap;
    static VaoMap _vaos;
};

};  // namespace Divide

#endif
