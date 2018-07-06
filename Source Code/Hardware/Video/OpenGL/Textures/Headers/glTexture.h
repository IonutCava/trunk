/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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