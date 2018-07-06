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

#ifndef _D3D_FRAME_BUFFER_OBJECT_H_
#define _D3D_FRAME_BUFFER_OBJECT_H_

#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

class d3dFrameBufferObject : public FrameBufferObject
{
public:

	d3dFrameBufferObject() : FrameBufferObject() {}
	virtual ~d3dFrameBufferObject() {}

	virtual bool Create(U16 width, U16 height, IMAGE_FORMATS internalFormatEnum = RGBA8,
											   IMAGE_FORMATS formatEnum = RGBA) = 0;
				
	virtual void Destroy() = 0;

	virtual void Begin(U8 nFace=0) const = 0;
	virtual void End(U8 nFace=0) const  = 0;

	virtual void Bind(U8 unit=0, U8 texture = 0)  = 0;
	virtual void Unbind(U8 unit=0) = 0;

protected:
	bool checkStatus();

};

#endif