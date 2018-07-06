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

struct GLFONScontext {
    GLuint tex;
    GLuint glfons_vaoID;
    GLuint glfons_vboID;
    GLuint glfons_prevVertDataSize;
    int width, height;
};

static int glfons__renderCreate(void* userPtr, int width, int height) {
    assert((width > 0 && height > 0) && "glfons__renderCreate error: invalid texture dimensions!");
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    glCreateTextures(GL_TEXTURE_2D, 1, &gl->tex);
    glCreateVertexArrays(1, &gl->glfons_vaoID);
    glCreateBuffers(1, &gl->glfons_vboID);

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

    gl->glfons_prevVertDataSize = 0;

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
                               const float* verts,
                               const float* tcoords,
                               const unsigned char* colours,
                               int nverts) {
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    if (gl->tex == 0) {
        return;
    }

    GLuint bufferID = gl->glfons_vboID;
    Divide::GL_API::setActiveVAO(gl->glfons_vaoID);
    Divide::GL_API::setActiveBuffer(GL_ARRAY_BUFFER, gl->glfons_vboID);

    GLuint vertDataSize = sizeof(float) * 2 * nverts;
    if (vertDataSize != gl->glfons_prevVertDataSize) {
        gl->glfons_prevVertDataSize = vertDataSize;
        glNamedBufferData(bufferID,
                          2 * vertDataSize + sizeof(unsigned char) * 4 * nverts,
                          NULL,
                          GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)(0));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (char*)0 + (vertDataSize));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char) * 4, (char*)0 + (2 * vertDataSize));

    } else {
        glInvalidateBufferData(bufferID);
    }

    glNamedBufferSubData(bufferID, 0, vertDataSize, (Divide::bufferPtr)verts);
    glNamedBufferSubData(bufferID, vertDataSize, vertDataSize, (Divide::bufferPtr)tcoords);
    glNamedBufferSubData(bufferID, 2 * vertDataSize, sizeof(unsigned char) * 4 * nverts, (Divide::bufferPtr)colours);
    Divide::GL_API::bindTexture(0, gl->tex, GL_TEXTURE_2D);
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
    gl->glfons_prevVertDataSize = 0;
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
