/*�Copyright 2009-2011 DIVIDE-Studio�*/
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

#ifndef _GL_VERTEX_BUFFER_OBJECT_H
#define _GL_VERTEX_BUFFER_OBJECT_H

#include "../VertexBufferObject.h"

class glVertexBufferObject : public VertexBufferObject
{

public:
	bool        Create();
	bool		Create(U32 usage);

	void		Destroy();

	
	void		Enable();
	void		Disable();

	
	glVertexBufferObject();
	~glVertexBufferObject() {Destroy();}

private:
	void Enable_VA();	
	void Enable_VBO();	
	void Disable_VA();	
	void Disable_VBO();

private:
	bool _created; //VBO's can be auto-created as GL_STATIC_DRAW if Enable() is called before Create();
				   //This helps with multi-threaded asset loading without creating separate GL contexts for each thread

};

#endif
