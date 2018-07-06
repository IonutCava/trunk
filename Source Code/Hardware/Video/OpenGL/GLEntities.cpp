//Creation and management of new OpenGL related entities: GL fonts, FBO's, VBO's etc
#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"
#include "Core/Headers/ParamHandler.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glVertexArrayObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glTextureArrayBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDepthArrayBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glMSTextureBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glTextureBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDeferredBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDepthBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/PixelBufferObject/Headers/glPixelBufferObject.h"

#ifndef FTGL_LIBRARY_STATIC
#define FTGL_LIBRARY_STATIC
#endif
#include <FTGL/ftgl.h>

namespace IMPrimitiveValidation{
	inline bool isValid(glIMPrimitive* const priv){ return (priv && !priv->inUse()); }
}

IMPrimitive* GL_API::createPrimitive(bool allowPrimitiveRecycle) {
    return dynamic_cast<IMPrimitive*>(getOrCreateIMPrimitive(allowPrimitiveRecycle));
}

glIMPrimitive* GL_API::getOrCreateIMPrimitive(bool allowPrimitiveRecycle){
	glIMPrimitive* tempPriv = NULL;
	//Find a zombified primitive
	vectorImpl<glIMPrimitive* >::iterator it = std::find_if(_glimInterfaces.begin(),_glimInterfaces.end(),IMPrimitiveValidation::isValid);
	if(allowPrimitiveRecycle && it != _glimInterfaces.end()){//If we have one ...
		tempPriv = *it;
		//... resurrect it
		tempPriv->_zombieCounter = 0;
		tempPriv->clear();
	}else{//If we do not have a valid zombie, add a new element
		tempPriv = New glIMPrimitive();
		_glimInterfaces.push_back(tempPriv);
	}
	assert(tempPriv != NULL);
	tempPriv->_setupStates.clear();
	tempPriv->_resetStates.clear();
	tempPriv->_canZombify = allowPrimitiveRecycle;
	return tempPriv;
}

FTFont* GL_API::getFont(const std::string& fontName){
    FontCache::iterator itr = _fonts.find(fontName);
	if(itr == _fonts.end()){
		std::string fontPath = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/";
		fontPath += "GUI/";
		fontPath += "fonts/";
		fontPath += fontName;
		FTFont* tempFont = New FTTextureFont(fontPath.c_str());
		if (!tempFont) {
			ERROR_FN(Locale::get("ERROR_FONT_FILE"),fontName.c_str());
        }
		_fonts.insert(std::make_pair(fontName, tempFont));
		ftglSetAttributeLocations(Divide::GL::VERTEX_POSITION_LOCATION, Divide::GL::VERTEX_TEXCOORD_LOCATION);
	}
    return _fonts[fontName];
}

RenderStateBlock* GL_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor){
	return New glRenderStateBlock(descriptor);
}

FrameBufferObject* GL_API::newFBO(const FBOType& type)  {
	switch(type){
		case FBO_2D_DEFERRED:
			return New glDeferredBufferObject();
		case FBO_2D_DEPTH:
			return New glDepthBufferObject();
		case FBO_2D_ARRAY_DEPTH:
			return New glDepthArrayBufferObject();
		case FBO_CUBE_COLOR:
			return New glTextureBufferObject(true);
		case FBO_CUBE_DEPTH:
			return New glTextureBufferObject(true,true);
		case FBO_CUBE_COLOR_ARRAY:
			return New glTextureArrayBufferObject(true);
		case FBO_CUBE_DEPTH_ARRAY:
			return New glTextureArrayBufferObject(true,true);
		case FBO_2D_ARRAY_COLOR:
			return New glTextureArrayBufferObject();
		case FBO_2D_COLOR_MS:{
			if(_msaaSamples > 1 && _useMSAA)
				return New glMSTextureBufferObject(); ///<No MS cube support yet
			else
				return New glTextureBufferObject();
		}
		default:
		case FBO_2D_COLOR:
			return New glTextureBufferObject();
	}
}

VertexBufferObject* GL_API::newVBO(const PrimitiveType& type) {
	return New glVertexArrayObject(type);
}

PixelBufferObject* GL_API::newPBO(const PBOType& type) {
	return New glPixelBufferObject(type);
}