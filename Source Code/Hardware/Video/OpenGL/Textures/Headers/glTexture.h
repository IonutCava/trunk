/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_TEXTURE_H_
#define _GL_TEXTURE_H_

#include "core.h"
#include "Hardware/Video/Textures/Headers/Texture.h"

class glTexture : public Texture {
public:
    glTexture(TextureType type, bool flipped = false);
    ~glTexture();

    bool unload();

    void Bind(GLushort unit);

    void setMipMapRange(GLushort base = 0, GLushort max = 1000);
    void updateMipMaps();

    void loadData(GLuint target, const GLubyte* const ptr, const vec2<GLushort>& dimensions, const vec2<GLushort>& mipLevels,
                  GFXImageFormat format, GFXImageFormat internalFormat, bool usePOW2 = false);

protected:
    bool generateHWResource(const std::string& name);
    void threadedLoad(const std::string& name);
    void reserveStorage();

private:
    GLenum _type;
    GLenum _format;
    GLenum _internalFormat;
    boost::atomic_bool _allocatedStorage;
    boost::atomic_bool _samplerCreated;    
    size_t _samplerHash;
    GLushort _mipMaxLevel;
    GLushort _mipMinLevel;
};

#endif