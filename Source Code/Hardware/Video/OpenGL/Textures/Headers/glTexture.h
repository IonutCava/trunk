/*
   Copyright (c) 2013 DIVIDE-Studio
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
	glTexture(GLuint type, bool flipped = false);
	~glTexture();

	bool unload() {Destroy(); return true;}

	void Bind(GLushort unit);
	void Unbind(GLushort unit);

	void LoadData(GLuint target, GLubyte* ptr, GLushort& w, GLushort& h, GLubyte d);

protected:
	bool generateHWResource(const std::string& name);
	void threadedLoad(const std::string& name);
private:

	void Bind() const;
	void Unbind() const;
	void Destroy();
	void reserveStorage(GLint w, GLint h);
private:
	GLenum _format;
	GLenum _internalFormat;
	GLenum _type;
	bool  _reservedStorage;   ///<Used glTexStorage2D for this texture
	GLboolean  _canReserveStorage; ///<Can use glTexStorage2D
};

#endif