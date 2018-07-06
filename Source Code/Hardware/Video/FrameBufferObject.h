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

#include "Hardware/Platform/PlatformDefines.h"

class FrameBufferObject {

public:
	enum FBO_TYPE {FBO_PLACEHOLDER, FBO_2D_COLOR, FBO_CUBE_COLOR, FBO_2D_DEPTH, FBO_2D_DEFERRED };
	enum TEXTURE_FORMAT {RGB, RGBA, BGRA, LUMINANCE, LUMINANCE_ALPHA};
	enum TEXTURE_FORMAT_INTERNAL {RGBA32F,RGBA16F,RGBA8I, RGBA8, RGB16F, RGB8I, RGB16, RGB8};

	virtual bool Create(FBO_TYPE type, U16 width, U16 height, TEXTURE_FORMAT_INTERNAL internalFormatEnum = RGBA8, TEXTURE_FORMAT formatEnum = RGBA) = 0;
	virtual void Destroy() = 0;

	virtual void Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;		

	virtual void Bind(U8 unit=0, U8 texture = 0) = 0;		
	virtual void Unbind(U8 unit=0) = 0;	

	inline U16 getWidth() const			{return _width;}
	inline U16 getHeight() const		{return _height;}
	virtual ~FrameBufferObject(){};
	FrameBufferObject() : _frameBufferHandle(0),
						  _depthBufferHandle(0),
					      _diffuseBufferHandle(0),
						  _positionBufferHandle(0),
						  _normalBufferHandle(0),
					      _width(0),
						  _height(0), 
						  _textureType(0),
						  _fboType(FBO_PLACEHOLDER),
						  _attachment(0),
					      _useFBO(true),
						  _bound(false),
						  _useDepthBuffer(false)
	{
	    _textureId.reserve(1);
	}

	FrameBufferObject(const FrameBufferObject& c){
		_useFBO = c._useFBO;
		_useDepthBuffer = c._useDepthBuffer;
		_bound = c._bound;
		_width = c._width,
		_height = c._height;
		_frameBufferHandle = c._frameBufferHandle;
		_depthBufferHandle = c._depthBufferHandle;
		_diffuseBufferHandle = c._diffuseBufferHandle;
		_positionBufferHandle = c._positionBufferHandle;
		_normalBufferHandle = c._normalBufferHandle;
		_textureType = c._textureType;
		_fboType = c._fboType;
	    _attachment = c._attachment;
		for_each(U32 v, c._textureId){
			_textureId.push_back(v);
		}
	}
	FrameBufferObject& operator=(const FrameBufferObject& c){
		_useFBO = c._useFBO;
		_useDepthBuffer = c._useDepthBuffer;
		_bound = c._bound;
		_width = c._width,
		_height = c._height;
		_frameBufferHandle = c._frameBufferHandle;
		_depthBufferHandle = c._depthBufferHandle;
		_diffuseBufferHandle = c._diffuseBufferHandle;
		_positionBufferHandle = c._positionBufferHandle;
		_normalBufferHandle = c._normalBufferHandle;
		_textureType = c._textureType;
		_fboType = c._fboType;
	    _attachment = c._attachment;
		for_each(U32 v, c._textureId){
			_textureId.push_back(v);
		}
		return *this;
	}

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

