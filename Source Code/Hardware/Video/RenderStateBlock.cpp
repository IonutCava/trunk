#include "Headers/RenderStateBlock.h"

RenderStateBlockDescriptor::RenderStateBlockDescriptor() : GUIDWrapper(),
                                                           _blendDefined(false),
                                                           _blendEnable(false),
                                                           _blendSrc(BLEND_PROPERTY_ONE),
                                                           _blendDest(BLEND_PROPERTY_ONE),
                                                           _blendOp(BLEND_OPERATION_ADD),
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
                                                           _zBias(0.0f),
                                                           _zUnits(2.0f),
                                                           _stencilDefined(false),
                                                           _stencilEnable(false),
                                                           _stencilFailOp(STENCIL_OPERATION_KEEP),
                                                           _stencilZFailOp(STENCIL_OPERATION_KEEP),
                                                           _stencilPassOp(STENCIL_OPERATION_KEEP),
                                                           _stencilFunc(CMP_FUNC_NEVER),
                                                           _stencilRef(0),
                                                           _stencilMask(0xFFFFFFFF),
                                                           _stencilWriteMask(0xFFFFFFFF),
                                                           _fillMode(FILL_MODE_SOLID)
{
}

void RenderStateBlockDescriptor::fromDescriptor(const RenderStateBlockDescriptor& descriptor){
   if (descriptor._blendDefined)  {
      setBlend(descriptor._blendEnable,descriptor._blendSrc,descriptor._blendDest,descriptor._blendOp);
   }

   if (descriptor._enableColorWrite) {
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

   _fillMode = descriptor._fillMode;
}

void RenderStateBlockDescriptor::flipCullMode(){
    _cullDefined = true;

    if (_cullMode == CULL_MODE_NONE) _cullMode = CULL_MODE_ALL;
    if (_cullMode == CULL_MODE_ALL)  _cullMode = CULL_MODE_NONE;
    if (_cullMode == CULL_MODE_CW)   _cullMode = CULL_MODE_CCW;
    if (_cullMode == CULL_MODE_CCW)  _cullMode = CULL_MODE_CW;
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

void RenderStateBlockDescriptor::setColorWrites( bool red, bool green, bool blue, bool alpha ) {
   _enableColorWrite = true;
   _writeRedChannel = red;
   _writeGreenChannel = green;
   _writeBlueChannel = blue;
   _writeAlphaChannel = alpha;
}