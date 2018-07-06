#include "Headers/RenderStateBlock.h"
#include "Core/Math/Headers/MathHelper.h"

namespace Divide {

RenderStateBlockDescriptor::RenderStateBlockDescriptor()
    : GUIDWrapper(), 
    _cachedHash(0),
    _lockHash(false)
{
    setDefaultValues();
}

RenderStateBlockDescriptor::RenderStateBlockDescriptor(const RenderStateBlockDescriptor& other)
    : GUIDWrapper(other),
     _lockHash(false),
     _colorWrite(other._colorWrite),
     _blendEnable(other._blendEnable),
     _blendSrc(other._blendSrc),
     _blendDest(other._blendDest),
     _blendOp(other._blendOp),
     _cullMode(other._cullMode),
     _cullEnabled(other._cullEnabled),
     _lineWidth(other._lineWidth),
     _zEnable(other._zEnable),
     _zWriteEnable(other._zWriteEnable),
     _zFunc(other._zFunc),
     _zBias(other._zBias),
     _zUnits(other._zUnits),
     _stencilEnable(other._stencilEnable),
     _stencilRef(other._stencilRef),
     _stencilMask(other._stencilMask),
     _stencilWriteMask(other._stencilWriteMask),
     _stencilFailOp(other._stencilFailOp),
     _stencilZFailOp(other._stencilZFailOp),
     _stencilPassOp(other._stencilPassOp),
     _stencilFunc(other._stencilFunc),
     _fillMode(other._fillMode),
     _cachedHash(other._cachedHash)
{
}

void RenderStateBlockDescriptor::flipCullMode() {
    if (_cullMode == CullMode::NONE) {
        _cullMode = CullMode::ALL;
    }
    if (_cullMode == CullMode::ALL) {
        _cullMode = CullMode::NONE;
    }
    if (_cullMode == CullMode::CW) {
        _cullMode = CullMode::CCW;
    }
    if (_cullMode == CullMode::CCW) {
        _cullMode = CullMode::CW;
    }
    clean();
}

void RenderStateBlockDescriptor::setCullMode(CullMode mode) {
    _cullMode = mode;
    _cullEnabled = _cullMode == CullMode::NONE ? false : true;
    clean();
}

void RenderStateBlockDescriptor::setZEnable(const bool enable) {
    _zEnable = enable;

    clean();
}

void RenderStateBlockDescriptor::setZReadWrite(bool read, bool write) {
    _zEnable = read;
    _zWriteEnable = write;

    clean();
}

void RenderStateBlockDescriptor::setLineWidth(F32 width) {
    _lineWidth = width;

    clean();
}

void RenderStateBlockDescriptor::setBlend(bool enable,
                                          BlendProperty src,
                                          BlendProperty dest,
                                          BlendOperation op) {
    _blendEnable = enable;
    _blendSrc = src;
    _blendDest = dest;
    _blendOp = op;

    clean();
}

void RenderStateBlockDescriptor::setColorWrites(bool red,
                                                bool green,
                                                bool blue,
                                                bool alpha) {
    _colorWrite.b[0] = red ? 1 : 0;
    _colorWrite.b[1] = green ? 1 : 0;
    _colorWrite.b[2] = blue ? 1 : 0;
    _colorWrite.b[3] = alpha ? 1 : 0;

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
    setBlend(false, BlendProperty::ONE,
             BlendProperty::ONE,
             BlendOperation::ADD);
    setZReadWrite(true, true);
    setCullMode(CullMode::CW);
    setFillMode(FillMode::SOLID);
    setStencilReadWriteMask(0xFFFFFFFF, 0xFFFFFFFF);
    setStencil(false);
    setLineWidth(1.0f);
    _lockHash = false;

    clean();
}

void RenderStateBlockDescriptor::clean() {
    if (_lockHash) {
        return;
    }

    _cachedHash = 0;
    Util::Hash_combine(_cachedHash, _colorWrite.i);
    Util::Hash_combine(_cachedHash, _blendEnable);
    Util::Hash_combine(_cachedHash, _blendSrc);
    Util::Hash_combine(_cachedHash, _blendDest);
    Util::Hash_combine(_cachedHash, _blendOp);
    Util::Hash_combine(_cachedHash, _lineWidth);
    Util::Hash_combine(_cachedHash, _cullMode);
    Util::Hash_combine(_cachedHash, _cullEnabled);
    Util::Hash_combine(_cachedHash, _zEnable);
    Util::Hash_combine(_cachedHash, _zWriteEnable);
    Util::Hash_combine(_cachedHash, _zFunc);
    Util::Hash_combine(_cachedHash, _zBias);
    Util::Hash_combine(_cachedHash, _zUnits);
    Util::Hash_combine(_cachedHash, _stencilEnable);
    Util::Hash_combine(_cachedHash, _stencilRef);
    Util::Hash_combine(_cachedHash, _stencilMask);
    Util::Hash_combine(_cachedHash, _stencilWriteMask);
    Util::Hash_combine(_cachedHash, _stencilFailOp);
    Util::Hash_combine(_cachedHash, _stencilZFailOp);
    Util::Hash_combine(_cachedHash, _stencilPassOp);
    Util::Hash_combine(_cachedHash, _stencilFunc);
    Util::Hash_combine(_cachedHash, _fillMode);
}
};