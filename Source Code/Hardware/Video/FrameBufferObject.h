/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _FRAME_BUFFER_OBJECT_H
#define _FRAME_BUFFER_OBJECT_H

#include <iostream>
#include "Hardware/Platform/PlatformDefines.h"

class FrameBufferObject 
{
public:
	enum FBO_TYPE { FBO_2D_COLOR, FBO_CUBE_COLOR, FBO_2D_DEPTH, FBO_2D_DEFERRED };

	virtual bool Create(FBO_TYPE type, U16 width, U16 height) = 0;
	virtual void Destroy() = 0;

	virtual void Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;		

	virtual void Bind(U8 unit=0, U8 texture = 0) = 0;		
	virtual void Unbind(U8 unit=0) = 0;	

	inline U16 getWidth() const			{return _width;}
	inline U16 getHeight() const		{return _height;}
	virtual ~FrameBufferObject(){};

protected:
	virtual bool checkStatus() = 0;

protected:
	bool		_useFBO;
	bool		_useDepthBuffer;
	bool        _bound;
	std::vector<U32>   _textureId;
	U16		    _width, _height;
	U32		    _frameBufferHandle;
	U32		    _depthBufferHandle;
	U32         _diffuseBufferHandle;
	U32         _positionBufferHandle;
	U32         _normalBufferHandle;
	U32		    _textureType;
	U8          _fboType;
	U32		    _attachment;
	
};


#endif

