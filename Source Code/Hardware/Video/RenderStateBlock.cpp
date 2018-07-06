#include "Headers/RenderStateBlock.h"

RenderStateBlockDescriptor::RenderStateBlockDescriptor() : GUIDWrapper(),
                                                           _dirty(true),
                                                           _cachedHash(0)
{
    setDefaultValues();
}

void RenderStateBlockDescriptor::fromDescriptor(const RenderStateBlockDescriptor& descriptor){
   setBlend(descriptor._blendEnable,descriptor._blendSrc,descriptor._blendDest,descriptor._blendOp);
   setColorWrites(descriptor._writeRedChannel,descriptor._writeGreenChannel,descriptor._writeBlueChannel,descriptor._writeAlphaChannel);
   setCullMode(descriptor._cullMode);
   setZReadWrite(descriptor._zEnable, descriptor._zWriteEnable);

   _zFunc = descriptor._zFunc;
   _zBias = descriptor._zBias;
   _zUnits = descriptor._zUnits;
  
   _stencilEnable = descriptor._stencilEnable;
   _stencilFailOp = descriptor._stencilFailOp;
   _stencilZFailOp = descriptor._stencilZFailOp;
   _stencilPassOp = descriptor._stencilPassOp;
   _stencilFunc = descriptor._stencilFunc;
   _stencilRef = descriptor._stencilRef;
   _stencilMask = descriptor._stencilMask;
   _stencilWriteMask = descriptor._stencilWriteMask;

   _fillMode = descriptor._fillMode;

   _dirty = true;
}

void RenderStateBlockDescriptor::flipCullMode(){
    if (_cullMode == CULL_MODE_NONE) _cullMode = CULL_MODE_ALL;
    if (_cullMode == CULL_MODE_ALL)  _cullMode = CULL_MODE_NONE;
    if (_cullMode == CULL_MODE_CW)   _cullMode = CULL_MODE_CCW;
    if (_cullMode == CULL_MODE_CCW)  _cullMode = CULL_MODE_CW;

    _dirty = true;
}

void RenderStateBlockDescriptor::setCullMode( CullMode mode ) {
   _cullMode = mode;

   _dirty = true;
}

void RenderStateBlockDescriptor::setZEnable(const bool enable){
    _zEnable = enable;

    _dirty = true;
}

void RenderStateBlockDescriptor::setZReadWrite( bool read, bool write ) {
   _zEnable = read;
   _zWriteEnable = write;

   _dirty = true;
}

void RenderStateBlockDescriptor::setBlend( bool enable, BlendProperty src, BlendProperty dest, BlendOperation op ) {
   _blendEnable = enable;
   _blendSrc = src;
   _blendDest = dest;
   _blendOp = op;

   _dirty = true;
}

void RenderStateBlockDescriptor::setColorWrites( bool red, bool green, bool blue, bool alpha ) {
   _writeRedChannel = red;
   _writeGreenChannel = green;
   _writeBlueChannel = blue;
   _writeAlphaChannel = alpha;

   _dirty = true;
}

void RenderStateBlockDescriptor::setZBias(F32 zBias, F32 zUnits, ComparisonFunction zFunc){
    _zBias = zBias;
    _zUnits = zUnits;
    _zFunc = zFunc;

    _dirty = true;
}

void RenderStateBlockDescriptor::setFillMode(FillMode mode)  { 
    _fillMode = mode; 

    _dirty = true;
}

void RenderStateBlockDescriptor::setStencilReadWriteMask(U32 read, U32 write) {
    _stencilMask = read;
    _stencilWriteMask = write;

    _dirty = true;
}

void RenderStateBlockDescriptor::setStencil(bool enable, U32 stencilRef, StencilOperation stencilFailOp, StencilOperation stencilZFailOp, 
                                                                         StencilOperation stencilPassOp, ComparisonFunction stencilFunc) {
    _stencilEnable = enable;
    _stencilRef = stencilRef;
    _stencilFailOp = stencilFailOp;
    _stencilZFailOp = stencilZFailOp;
    _stencilPassOp = stencilPassOp;
    _stencilFunc = stencilFunc;

    _dirty = true;
}

void RenderStateBlockDescriptor::setDefaultValues(){
    setZBias(0.0f, 2.0f);
    setColorWrites(true, true, true, true);
    setBlend(false, BLEND_PROPERTY_ONE, BLEND_PROPERTY_ONE, BLEND_OPERATION_ADD);
    setZReadWrite(true, true);
    setCullMode(CULL_MODE_CW);
    setFillMode(FILL_MODE_SOLID);
    setStencilReadWriteMask(0xFFFFFFFF, 0xFFFFFFFF);
    setStencil(false);
}

void RenderStateBlockDescriptor::clean(){
    if (!_dirty)
        return;

    _cachedHash =  Util::CRC32(this, sizeof(RenderStateBlockDescriptor));
    _dirty = false;
}