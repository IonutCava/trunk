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

	virtual void Bind(U8 unit=0, U8 texture = 0) const = 0;		
	virtual void Unbind(U8 unit=0) const = 0;	

	inline std::vector<U32> getTextureHandle() const	{return _textureId;} 
	inline U16 getWidth() const			{return _width;}
	inline U16 getHeight() const		{return _height;}

	virtual ~FrameBufferObject(){};

protected:
	virtual bool checkStatus() = 0;

protected:
	bool		_useFBO;
	bool		_useDepthBuffer;
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

