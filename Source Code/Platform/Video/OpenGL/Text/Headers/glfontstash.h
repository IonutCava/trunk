//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Modified by Ionut Cava: Updated rendering for compatibility with OpenGL 3.3+
// Core Context

#ifndef GLFONTSTASH_H
#define GLFONTSTASH_H

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

struct FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(struct FONScontext* ctx);
#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

constexpr GLuint glfons_position_slot = (GLuint)(Divide::AttribLocation::VERTEX_POSITION);
constexpr GLuint glfons_textcoord_slot = (GLuint)(Divide::AttribLocation::VERTEX_TEXCOORD);
constexpr GLuint glfons_colour_slot = (GLuint)(Divide::AttribLocation::VERTEX_COLOR);

struct GLFONScontext {
    GLuint tex;
    GLuint glfons_vaoID;
    GLuint glfons_vboID;
    int width, height;
};

static int glfons__renderCreate(void* userPtr, int width, int height) {
    assert((width > 0 && height > 0) && "glfons__renderCreate error: invalid texture dimensions!");
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    glCreateTextures(GL_TEXTURE_2D, 1, &gl->tex);
    glCreateVertexArrays(1, &gl->glfons_vaoID);
    glCreateBuffers(1, &gl->glfons_vboID);

    Divide::GL_API::setActiveVAO(gl->glfons_vaoID);
    {
        Divide::U32 prevOffset = 0;
        glEnableVertexAttribArray(glfons_position_slot);
        glVertexAttribFormat(glfons_position_slot, 2, GL_FLOAT, GL_FALSE, prevOffset);

        prevOffset += Divide::to_U32(sizeof(float) * 2);
        glEnableVertexAttribArray(glfons_textcoord_slot);
        glVertexAttribFormat(glfons_textcoord_slot, 2, GL_FLOAT, GL_FALSE, Divide::to_U32(prevOffset));

        prevOffset += Divide::to_U32(sizeof(float) * 2);
        glEnableVertexAttribArray(glfons_colour_slot);
        glVertexAttribFormat(glfons_colour_slot, 4, GL_UNSIGNED_BYTE, GL_TRUE, prevOffset);

        glVertexAttribBinding(glfons_position_slot, 0);
        glVertexAttribBinding(glfons_textcoord_slot, 0);
        glVertexAttribBinding(glfons_colour_slot, 0);

        glVertexArrayVertexBuffer(gl->glfons_vaoID, 0, gl->glfons_vboID, 0, sizeof(FONSvert));
    }

    if (Divide::Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_BUFFER,
                      gl->glfons_vaoID,
                      -1,
                      Divide::Util::StringFormat("DVD_FONT_VAO_%d", gl->glfons_vaoID).c_str());
        glObjectLabel(GL_BUFFER,
                      gl->glfons_vboID,
                      -1,
                      Divide::Util::StringFormat("DVD_FONT_VB_%d", gl->glfons_vboID).c_str());
    }

    if (!gl->tex || !gl->glfons_vaoID || !gl->glfons_vboID) {
        return 0;
    }

    gl->width = width;
    gl->height = height;

    glTextureStorage2D(gl->tex, 1, GL_R8, gl->width, gl->height);
    glTextureParameteri(gl->tex, GL_TEXTURE_MIN_FILTER, Divide::to_base(GL_LINEAR));

    return 1;
}

static void glfons__renderUpdate(void* userPtr,
                                 int* rect,
                                 const unsigned char* data) {
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;

    if (gl->tex != 0) {
        int w = rect[2] - rect[0];
        int h = rect[3] - rect[1];
        Divide::GL_API::setPixelUnpackAlignment(1, gl->width, rect[1], rect[0]);
        glTextureSubImage2D(gl->tex, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, (Divide::bufferPtr)data);
    }
}

static void glfons__renderDraw(void* userPtr,
                               const FONSvert* verts,
                               int nverts) {
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    if (gl->tex == 0) {
        return;
    }

    GLuint bufferID = gl->glfons_vboID;
    Divide::GL_API::setActiveVAO(gl->glfons_vaoID);
    Divide::GL_API::setActiveBuffer(GL_ARRAY_BUFFER, gl->glfons_vboID);
    Divide::GL_API::bindTexture(0, gl->tex);

    glInvalidateBufferData(bufferID);
    glNamedBufferData(bufferID, nverts * sizeof(FONSvert), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, nverts);
}

static void glfons__renderDelete(void* userPtr) {
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    if (gl->tex)
        glDeleteTextures(1, &gl->tex);
    if (gl->glfons_vaoID)
        glDeleteVertexArrays(1, &gl->glfons_vaoID);
    gl->tex = 0;
    gl->glfons_vaoID = 0;
    Divide::GLUtil::freeBuffer(gl->glfons_vboID);
    free(gl);
}

struct FONScontext* glfonsCreate(int width, int height, int flags) {
    struct FONSparams params;
    struct GLFONScontext* gl;

    gl = (struct GLFONScontext*)malloc(sizeof(struct GLFONScontext));
    if (gl == nullptr)
        goto error;
    memset(gl, 0, sizeof(struct GLFONScontext));

    memset(&params, 0, sizeof(params));
    params.width = width;
    params.height = height;
    params.flags = static_cast<unsigned char>(flags);
    params.renderCreate = glfons__renderCreate;
    params.renderUpdate = glfons__renderUpdate;
    params.renderDraw = glfons__renderDraw;
    params.renderDelete = glfons__renderDelete;
    params.userPtr = gl;

    return fonsCreateInternal(&params);

error:
    free(gl);
    return nullptr;
}

void glfonsDelete(struct FONScontext* ctx) {
    fonsDeleteInternal(ctx);
}

#endif
