#include "Headers/RenderStateBlock.h"

RenderStateBlockDescriptor::RenderStateBlockDescriptor() : GUIDWrapper(),
														   _blendDefined(false),
														   _blendEnable(false),
														   _blendSrc(BLEND_PROPERTY_ONE),
														   _blendDest(BLEND_PROPERTY_ONE),
														   _blendOp(BLEND_OPERATION_ADD),
														   _alphaBlendDefined(false),
														   _alphaBlendEnable(false),
														   _alphaBlendSrc(BLEND_PROPERTY_ONE),
														   _alphaBlendDest(BLEND_PROPERTY_ZERO),
														   _alphaBlendOp(BLEND_OPERATION_ADD),
														   _enableColorWrite(true),
														   _writeRedChannel(true),
														   _writeBlueChannel(true),
														   _writeGreenChannel(true),
														   _writeAlphaChannel(true),
														   _cullDefined(false),
														   _cullMode(CULL_MODE_CW),
														   _zDefined(false),
														   _zEnable(true),
														   _zWriteEnable(true),
														   _zFunc(CMP_FUNC_LEQUAL),
														   _zBias(0),
														   _zUnits(4096.0f),
														   _stencilDefined(false),
														   _stencilEnable(false),
														   _stencilFailOp(STENCIL_OPERATION_KEEP),
														   _stencilZFailOp(STENCIL_OPERATION_KEEP),
														   _stencilPassOp(STENCIL_OPERATION_KEEP),
														   _stencilFunc(CMP_FUNC_NEVER),
														   _stencilRef(0),
														   _stencilMask(0xFFFFFFFF),
														   _stencilWriteMask(0xFFFFFFFF),
														   _vertexColorEnable(false),
														   _fillMode(FILL_MODE_SOLID)
{
}

void RenderStateBlockDescriptor::fromDescriptor(const RenderStateBlockDescriptor& descriptor){
   if (descriptor._blendDefined)  {
	  setBlend(descriptor._blendEnable,descriptor._blendSrc,descriptor._blendDest,descriptor._blendOp);
   }

   if ( descriptor._alphaBlendDefined )  {
	  setAlphaBlend(descriptor._alphaBlendEnable,descriptor._alphaBlendSrc,descriptor._alphaBlendDest,descriptor._alphaBlendOp);
   }

   if (descriptor._enableColorWrite)     {
	   setColorWrites(descriptor._writeRedChannel,descriptor._writeGreenChannel,descriptor._writeBlueChannel,descriptor._writeAlphaChannel);
   }

   if (descriptor._cullDefined)  {
	   setCullMode(descriptor._cullMode);
   }

   if (descriptor._zDefined)  {
	   setZReadWrite(descriptor._zEnable, descriptor._zWriteEnable);
      _zFunc = descriptor._zFunc;
      _zBias = descriptor._zBias;
      _zUnits = descriptor._zUnits;
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

void RenderStateBlockDescriptor::setCullMode( CullMode mode ) {
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

void RenderStateBlockDescriptor::setBlend( bool enable, BlendProperty src, BlendProperty dest, BlendOperation op ) {
   _blendDefined = true;
   _blendEnable = enable;
   _blendSrc = src;
   _blendDest = dest;
   _blendOp = op;
}

void RenderStateBlockDescriptor::setAlphaBlend( bool enable, BlendProperty src, BlendProperty dest, BlendOperation op ) {
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