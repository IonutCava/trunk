#include "Headers/RenderStateBlock.h"

namespace Divide {

RenderStateBlockDescriptor::RenderStateBlockDescriptor() : GUIDWrapper(),
                                                           _cachedHash(0),
                                                           _lockHash(false)
{
    setDefaultValues();
}

void RenderStateBlockDescriptor::fromDescriptor(const RenderStateBlockDescriptor& descriptor) {
   setCullMode(descriptor._cullMode);

   setBlend(descriptor._blendEnable,
            descriptor._blendSrc,
            descriptor._blendDest,
            descriptor._blendOp);

   setColorWrites(descriptor._colorWrite.b.b0 > 0,
                  descriptor._colorWrite.b.b1 > 0,
                  descriptor._colorWrite.b.b2 > 0,
                  descriptor._colorWrite.b.b3 > 0);

   setZReadWrite(descriptor._zEnable,
                 descriptor._zWriteEnable);

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

   clean();
}

void RenderStateBlockDescriptor::flipCullMode() {
    if (_cullMode == CULL_MODE_NONE) {
        _cullMode = CULL_MODE_ALL;
    }
    if (_cullMode == CULL_MODE_ALL)  {
        _cullMode = CULL_MODE_NONE;
    }
    if (_cullMode == CULL_MODE_CW) {
        _cullMode = CULL_MODE_CCW;
    }
    if (_cullMode == CULL_MODE_CCW) {
        _cullMode = CULL_MODE_CW;
    }
    clean();
}

void RenderStateBlockDescriptor::setCullMode(CullMode mode) {
   _cullMode = mode;
   _cullEnabled = _cullMode == CULL_MODE_NONE ? false : true;
   clean();
}

void RenderStateBlockDescriptor::setZEnable(const bool enable) {
    _zEnable = enable;

    clean();
}

void RenderStateBlockDescriptor::setZReadWrite( bool read, bool write ) {
   _zEnable = read;
   _zWriteEnable = write;

   clean();
}

void RenderStateBlockDescriptor::setBlend( bool enable, BlendProperty src, BlendProperty dest, BlendOperation op ) {
   _blendEnable = enable;
   _blendSrc = src;
   _blendDest = dest;
   _blendOp = op;

   clean();
}

void RenderStateBlockDescriptor::setColorWrites( bool red, bool green, bool blue, bool alpha ) {
    _colorWrite.b.b0 = red ? 1 : 0;
    _colorWrite.b.b1 = green ? 1 : 0;
    _colorWrite.b.b2 = blue ? 1 : 0;
    _colorWrite.b.b3 = alpha ? 1 : 0;

    clean();
}

void RenderStateBlockDescriptor::setZBias(F32 zBias, F32 zUnits) {
    _zBias = zBias;
    _zUnits = zUnits;

    clean();
}

void RenderStateBlockDescriptor::setZFunc(ComparisonFunction zFunc) {
    _zFunc = zFunc;

    clean();
}

void RenderStateBlockDescriptor::setFillMode(FillMode mode) { 
    _fillMode = mode; 

    clean();
}

void RenderStateBlockDescriptor::setStencilReadWriteMask(U32 read, U32 write) {
    _stencilMask = read;
    _stencilWriteMask = write;

    clean();
}

void RenderStateBlockDescriptor::setStencil(bool enable,
                                            U32 stencilRef, 
                                            StencilOperation stencilFailOp,
                                            StencilOperation stencilZFailOp, 
                                            StencilOperation stencilPassOp, 
                                            ComparisonFunction stencilFunc) {
    _stencilEnable = enable;
    _stencilRef = stencilRef;
    _stencilFailOp = stencilFailOp;
    _stencilZFailOp = stencilZFailOp;
    _stencilPassOp = stencilPassOp;
    _stencilFunc = stencilFunc;

    clean();
}

void RenderStateBlockDescriptor::setDefaultValues() {
    _lockHash = true;
    setZBias(0.0f, 1.0f);
    setZFunc();
    setColorWrites(true, true, true, true);
    setBlend(false, BLEND_PROPERTY_ONE, BLEND_PROPERTY_ONE, BLEND_OPERATION_ADD);
    setZReadWrite(true, true);
    setCullMode(CULL_MODE_CW);
    setFillMode(FILL_MODE_SOLID);
    setStencilReadWriteMask(0xFFFFFFFF, 0xFFFFFFFF);
    setStencil(false);
    _lockHash = false;

    clean();
}

void RenderStateBlockDescriptor::clean() {
    if (_lockHash) {
        return;
    }

    _cachedHash = 0;
    hash_combine(_cachedHash, _colorWrite.i);
    hash_combine(_cachedHash, _blendEnable);
    hash_combine(_cachedHash, _blendSrc);
    hash_combine(_cachedHash, _blendDest);
    hash_combine(_cachedHash, _blendOp);
    hash_combine(_cachedHash, _cullMode);
    hash_combine(_cachedHash, _cullEnabled);
    hash_combine(_cachedHash, _zEnable);
    hash_combine(_cachedHash, _zWriteEnable);
    hash_combine(_cachedHash, _zFunc);
    hash_combine(_cachedHash, _zBias);
    hash_combine(_cachedHash, _zUnits);
    hash_combine(_cachedHash, _stencilEnable);
    hash_combine(_cachedHash, _stencilRef);
    hash_combine(_cachedHash, _stencilMask);
    hash_combine(_cachedHash, _stencilWriteMask);
    hash_combine(_cachedHash, _stencilFailOp);
    hash_combine(_cachedHash, _stencilZFailOp);
    hash_combine(_cachedHash, _stencilPassOp);
    hash_combine(_cachedHash, _stencilFunc);
    hash_combine(_cachedHash, _fillMode);
}

};