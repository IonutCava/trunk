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
// Modified by Ionut Cava: Updated rendering for compatibility with OpenGL 3.3+ Core Context

#ifndef GLFONTSTASH_H
#define GLFONTSTASH_H

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

struct FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(struct FONScontext* ctx);
#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

struct GLFONScontext {
    GLuint tex;
    GLuint glfons_vaoID;
    GLuint glfons_vboID;
    int width, height;
};

static int glfons__renderCreate(void* userPtr, int width, int height)
{
    Divide::DIVIDE_ASSERT(width > 0 && height > 0, "glfons__renderCreate error: invalid texture dimensions!");
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    glGenTextures(1, &gl->tex);
    glGenVertexArrays(1, &gl->glfons_vaoID);
    glGenBuffers(1, &gl->glfons_vboID);
    if (!gl->tex || !gl->glfons_vaoID || !gl->glfons_vboID) return 0;
    gl->width = width;
    gl->height = width;
    Divide::GL_API::bindTexture(0, gl->tex, GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, gl->width, gl->height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    Divide::GL_API::unbindTexture(0, GL_TEXTURE_2D);
    return 1;
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    int w = rect[2] - rect[0];
    int h = rect[3] - rect[1];

    if (gl->tex == 0) return;
    Divide::GL_API::bindTexture(0, gl->tex, GL_TEXTURE_2D);
    Divide::GL_API::setPixelUnpackAlignment(1, gl->width, rect[1], rect[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, data);
    Divide::GL_API::unbindTexture(0, GL_TEXTURE_2D);
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned char* colors, int nverts)
{
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    if (gl->tex == 0) return;

    Divide::GL_API::bindTexture(0, gl->tex, GL_TEXTURE_2D);
    Divide::GL_API::setActiveVAO(gl->glfons_vaoID);
    GLuint vertDataSize = sizeof(float) * 2 * nverts;
    Divide::GL_API::setActiveBuffer(GL_ARRAY_BUFFER, gl->glfons_vboID);
    glBufferData(GL_ARRAY_BUFFER, 2 * vertDataSize + sizeof(unsigned char) * 4 * nverts, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,                vertDataSize,                       verts);
    glBufferSubData(GL_ARRAY_BUFFER, vertDataSize,     vertDataSize,                       tcoords);
    glBufferSubData(GL_ARRAY_BUFFER, 2 * vertDataSize, sizeof(unsigned char) * 4 * nverts, colors);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2,GL_FLOAT,GL_FALSE, sizeof(float)*2, (void*)(0));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2,GL_FLOAT,GL_FALSE, sizeof(float)*2, (char *)0 + (vertDataSize));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char) * 4, (char *)0 + (2 * vertDataSize));

    glDrawArrays(GL_TRIANGLES, 0, nverts);
}

static void glfons__renderDelete(void* userPtr)
{
    struct GLFONScontext* gl = (struct GLFONScontext*)userPtr;
    if (gl->tex)
        glDeleteTextures(1, &gl->tex);
    if(gl->glfons_vboID)
        glDeleteBuffers(1, &gl->glfons_vboID);
    if(gl->glfons_vaoID)
        glDeleteVertexArrays(1, &gl->glfons_vaoID);
    gl->tex = 0;
    gl->glfons_vboID = 0;
    gl->glfons_vaoID = 0;
    free(gl);
}


struct FONScontext* glfonsCreate(int width, int height, int flags)
{
    struct FONSparams params;
    struct GLFONScontext* gl;

    gl = (struct GLFONScontext*)malloc(sizeof(struct GLFONScontext));
    if (gl == nullptr) goto error;
    memset(gl, 0, sizeof(struct GLFONScontext));

    memset(&params, 0, sizeof(params));
    params.width = width;
    params.height = height;
    params.flags = flags;
    params.renderCreate = glfons__renderCreate;
    params.renderUpdate = glfons__renderUpdate;
    params.renderDraw = glfons__renderDraw; 
    params.renderDelete = glfons__renderDelete;
    params.userPtr = gl;

    return fonsCreateInternal(&params);

error:
    if (gl != nullptr) free(gl);
    return nullptr;
}

void glfonsDelete(struct FONScontext* ctx)
{
    fonsDeleteInternal(ctx);
}

#endif
