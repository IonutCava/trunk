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
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"
#include <boost/noncopyable.hpp>

class FrameBufferObject : private boost::noncopyable{

public:
	bool AddAttachment(const TextureDescriptor& decriptor, TextureDescriptor::AttachmentType slot);

	virtual bool Create(U16 width, U16 height, U8 imageLayers = 0) = 0;

	virtual void Destroy() = 0;
	virtual void DrawToLayer(U8 face, U8 layer) {} ///<Use by multilayerd FBO's
	virtual void Begin(U8 nFace=0) const = 0;	
	virtual void End(U8 nFace=0) const = 0;	

	virtual void Bind(U8 unit = 0, U8 texture = 0);
	virtual void Unbind(U8 unit=0);

	virtual void BlitFrom(FrameBufferObject* inputFBO) = 0;
	///A FBO with color writting disabled is a depth only buffer. But it is not the same as a depthBufferObject(!!) as we still need textures
	virtual void toggleColorWrites(bool state) {_disableColorWrites = !state;}
	virtual void toggleDepthWrites(bool state) {_useDepthBuffer = state;}
	inline U16 getWidth()  const	{return _width;}
	inline U16 getHeight() const	{return _height;}
	inline U8  getType()   const	{return _fboType;}
	inline U8  getHandle() const	{return _frameBufferHandle;}
	///Support function. Same as "Bind(..)" but sets a flag to enable internal api calls to bind the image without shader support (e.g. OGL: glEnable(textureType))
	inline void BindFixed(U8 unit=0, U8 texture = 0) {_fixedPipeline = true; Bind(unit,texture);}

	FrameBufferObject(FBOType type);
	virtual ~FrameBufferObject();

protected:
	virtual bool checkStatus() = 0;

protected:
	typedef Unordered_map<TextureDescriptor::AttachmentType, TextureDescriptor >  TextureAttachements;

	bool		_useDepthBuffer;
	bool        _disableColorWrites;
	bool        _bound;
	bool        _fixedPipeline;
	U16		    _width, _height;
	U32		    _frameBufferHandle;
	U32		    _depthBufferHandle;
	U32		    _textureType;
	U8          _fboType;
	U8          _prevTexture;
	U8          _prevUnit;

	TextureAttachements _attachement;	
	Unordered_map<TextureDescriptor::AttachmentType, bool > _attachementDirty;
};


#endif

