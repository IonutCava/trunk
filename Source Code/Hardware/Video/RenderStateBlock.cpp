#include "RenderStateBlock.h"
#include "Utility/Headers/CRC.h"
U32 RenderStateBlockDescriptor::getHash() const{   
	return Util::CRC32( this, sizeof(RenderStateBlockDescriptor) );
}

RenderStateBlockDescriptor::RenderStateBlockDescriptor() : _hash(0),
														   _blendDefined(false),
														   _blendEnable(false),
														   _blendSrc(BLEND_PROPERTY_One),
														   _blendDest(BLEND_PROPERTY_One),
														   _blendOp(BLEND_OPERATION_Add),
														   _alphaBlendDefined(false),
														   _alphaBlendEnable(false),
														   _alphaBlendSrc(BLEND_PROPERTY_One),
														   _alphaBlendDest(BLEND_PROPERTY_Zero),
														   _alphaBlendOp(BLEND_OPERATION_Add),
														   _alphaDefined(false),
														   _alphaTestEnable(false),
														   _alphaTestRef(0),
														   _alphaTestFunc(COMPARE_FUNC_GreaterEqual),
														   _enableColorWrite(false),
														   _writeRedChannel(true),
														   _writeBlueChannel(true),
														   _writeGreenChannel(true),
														   _writeAlphaChannel(true),
														   _cullDefined(false),
														   _cullMode(CULL_MODE_CW),
														   _zDefined(false),
														   _zEnable(true),
														   _zWriteEnable(true),
														   _zFunc(COMPARE_FUNC_LessEqual),
														   _zBias(0),
														   _zSlopeBias(0),
														   _stencilDefined(false),
														   _stencilEnable(false),
														   _stencilFailOp(STENCIL_OPERATION_Keep),
														   _stencilZFailOp(STENCIL_OPERATION_Keep),
														   _stencilPassOp(STENCIL_OPERATION_Keep),
														   _stencilFunc(COMPARE_FUNC_Never),
														   _stencilRef(0),
														   _stencilMask(0xFFFFFFFF),
														   _stencilWriteMask(0xFFFFFFFF),
														   _fixedLighting(false),
														   _vertexColorEnable(false),
														   _fillMode(FILL_MODE_Solid)
{
}

void RenderStateBlockDescriptor::fromDescriptor(const RenderStateBlockDescriptor& descriptor){

   if (descriptor._blendDefined)  {
	  setBlend(descriptor._blendEnable,descriptor._blendSrc,descriptor._blendDest,descriptor._blendOp);
   }

   if ( descriptor._alphaBlendDefined )  {
	  setAlphaBlend(descriptor._alphaBlendEnable,descriptor._alphaBlendSrc,descriptor._alphaBlendDest,descriptor._alphaBlendOp);
   }

   if (descriptor._alphaDefined)  {
	   setAlphaTest(descriptor._alphaTestEnable,descriptor._alphaTestFunc,descriptor._alphaTestRef);
   }

   if (descriptor._enableColorWrite)     {
	   setColorWrites(descriptor._writeRedChannel,descriptor._writeGreenChannel,descriptor._writeBlueChannel,descriptor._writeAlphaChannel);
   }

   if (descriptor._cullDefined)  {   
	   setCullMode(descriptor._cullMode);
   }

   if (descriptor._zDefined)  {   
	   setZReadWrite(descriptor._zEnable,descriptor._zWriteEnable);
      _zFunc = descriptor._zFunc;
      _zBias = descriptor._zBias;
      _zSlopeBias = descriptor._zSlopeBias;
   }

   if (descriptor._stencilDefined)  {   
      _stencilDefined = true;
      _stencilEnable = descriptor._stencilEnable;
      _stencilFailOp = descriptor._stencilFailOp;
      _stencilZFailOp = descriptor._stencilZFailOp;
      _stencilPassOp = descriptor._stencilPassOp;
      _stencilFunc = descriptor._stencilFunc;
      _stencilRef = descriptor._stencilRef;
      _stencilMask = descriptor._stencilMask;
      _stencilWriteMask = descriptor._stencilWriteMask;
   }

   _vertexColorEnable = descriptor._vertexColorEnable;
   _fillMode = descriptor._fillMode;
}

void RenderStateBlockDescriptor::setCullMode( CULL_MODE mode ) { 
   _cullDefined = true; 
   _cullMode = mode; 
}

void RenderStateBlockDescriptor::setZEnable(const bool enable){
	_zDefined = true;
	_zEnable = enable;
}

void RenderStateBlockDescriptor::setZReadWrite( bool read, bool write ) { 
   _zDefined = true; 
   _zEnable = read; 
   _zWriteEnable = write;
}

void RenderStateBlockDescriptor::setAlphaTest( bool enable, COMPARE_FUNC function, I32 alphaRef ) { 
   _alphaDefined = true; 
   _alphaTestEnable = enable; 
   _alphaTestFunc = function; 
   _alphaTestRef = alphaRef; 
}

void RenderStateBlockDescriptor::setBlend( bool enable, BLEND_PROPERTY src, BLEND_PROPERTY dest, BLEND_OPERATION op ) { 
   _blendDefined = true; 
   _blendEnable = enable; 
   _blendSrc = src; 
   _blendDest = dest; 
   _blendOp = op;
}

void RenderStateBlockDescriptor::setAlphaBlend( bool enable, BLEND_PROPERTY src, BLEND_PROPERTY dest, BLEND_OPERATION op ) { 
   _alphaBlendDefined = true; 
   _alphaBlendEnable = enable; 
   _alphaBlendSrc = src; 
   _alphaBlendDest = dest; 
   _alphaBlendOp = op;
}

void RenderStateBlockDescriptor::setColorWrites( bool red, bool green, bool blue, bool alpha ) {
   _enableColorWrite = true;
   _writeRedChannel = red;
   _writeGreenChannel = green;
   _writeBlueChannel = blue;
   _writeAlphaChannel = alpha;
}