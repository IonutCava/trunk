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
/// Always bind a shader, even a dummy one when rendering geometry. No more
/// fixed matrix API means no more VBs or VAs
/// One VAO contains: one VB for data, one IB for indices and uploads to the
/// shader vertex attribs for:
///- Vertex Data  bound to location Divide::VERTEX_POSITION
///- Colors       bound to location Divide::VERTEX_COLOR
///- Normals      bound to location Divide::VERTEX_NORMAL
///- TexCoords    bound to location Divide::VERTEX_TEXCOORD
///- Tangents     bound to location Divide::VERTEX_TANGENT
///- BiTangents   bound to location Divide::VERTEX_BITANGENT
///- Bone weights bound to location Divide::VERTEX_BONE_WEIGHT
///- Bone indices bound to location Divide::VERTEX_BONE_INDICE

namespace Divide {

class glVertexArray : public VertexBuffer {
   public:
    glVertexArray();
    ~glVertexArray();

    bool Create(bool staticDraw = true);
    void Destroy();

    virtual bool SetActive();

    /// Never call Refresh() just queue it and the data will update before
    /// drawing
    inline bool queueRefresh() {
        _refreshQueued = true;
        return true;
    }

   protected:
    friend class GFXDevice;
    void Draw(const GenericDrawCommand& commands,
              bool useCmdBuffer = false,
              bool skipBind = false);

   protected:
    /// If we have a shader, we create a VAO, if not, we use simple VB + IB. If
    /// that fails, use VA
    bool Refresh();
    /// Internally create the VB
    bool CreateInternal();
    /// Enable full VAO based VB (all pointers are tracked by VAO's)
    void Upload_VB_Attributes();
    /// Integrity checks
    void checkStatus();

   protected:
    GLenum _formatInternal;
    GLuint _IBid;
    GLuint _VBid;
    GLuint _VAOid;
    GLenum _usage;
    bool _animationData;  ///< Used to bind an extra set of vertex attributes
                          ///for bone indices and bone weights
    bool _refreshQueued;  ///< A refresh call might be called before "Create()".
                          ///This should help with that
    vectorImpl<vec4<GLhalf> > _normalsSmall;
    vectorImpl<vec4<GLshort> > _tangentSmall;
    vectorImpl<vec4<GLshort> > _bitangentSmall;

    std::array<GLsizei, to_const_uint(VertexAttribute::COUNT)> _prevSize;
    GLsizei _prevSizeIndices;
};

};  // namespace Divide

#endif
