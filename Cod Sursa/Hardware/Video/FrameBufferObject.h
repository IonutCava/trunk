#ifndef _FRAME_BUFFER_OBJECT_H
#define _FRAME_BUFFER_OBJECT_H

#include <iostream>
#include "Utility/Headers/DataTypes.h"

class FrameBufferObject 
{
public:
	enum FBO_TYPE { FBO_2D_COLOR, FBO_CUBE_COLOR, FBO_2D_DEPTH };

	virtual bool Create(FBO_TYPE type, U32 width, U32 height) = 0;
	virtual void Destroy() = 0;

	virtual void Begin(U32 nFace=0) const = 0;	
	virtual void End(U32 nFace=0) const = 0;		

	virtual void Bind(int unit=0) const = 0;		
	virtual void Unbind(int unit=0) const = 0;	

	inline U32 getTextureHandle() const	{return _textureId;} 
	inline U32 getWidth() const			{return _width;}
	inline U32 getHeight() const		{return _height;}

	virtual ~FrameBufferObject(){};

protected:
	virtual bool checkStatus() = 0;

protected:
	bool		_useFBO;
	bool		_useDepthBuffer;
	U32		    _textureId;
	U32		    _width, _height;
	U32		    _frameBufferHandle;
	U32		    _depthBufferHandle;
	U32		    _textureType;
	U32		    _attachment;
	
};


#endif

