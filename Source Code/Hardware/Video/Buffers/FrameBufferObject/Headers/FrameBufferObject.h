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

#ifndef _FRAME_BUFFER_OBJECT_H
#define _FRAME_BUFFER_OBJECT_H

#include "core.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include <boost/noncopyable.hpp>

class FrameBufferObject : private boost::noncopyable{

public:

	virtual bool Create(U16 width, U16 height, IMAGE_FORMATS internalFormatEnum = RGBA8, IMAGE_FORMATS formatEnum = RGBA) = 0;
	virtual void Destroy() = 0;

	virtual void Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;		

	virtual void Bind(U8 unit=0, U8 texture = 0) = 0;		
	virtual void Unbind(U8 unit=0) = 0;	

	inline U16 getWidth()  const	{return _width;}
	inline U16 getHeight() const	{return _height;}
	inline U8  getType()   const	{return _fboType;}

	virtual ~FrameBufferObject(){};
	FrameBufferObject() : _frameBufferHandle(0),
						  _depthBufferHandle(0),
					      _width(0),
						  _height(0), 
						  _textureType(0),
						  _bound(false),
						  _useDepthBuffer(false){}

protected:
	virtual bool checkStatus() = 0;

protected:
	bool		_useDepthBuffer;
	bool        _bound;
	U16		    _width, _height;
	U32		    _frameBufferHandle;
	U32		    _depthBufferHandle;
	U32		    _textureType;
	U8          _fboType;
};


#endif

