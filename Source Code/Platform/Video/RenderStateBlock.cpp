#include "stdafx.h"

#include "Headers/RenderStateBlock.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderStateBlock::RenderStateMap RenderStateBlock::s_stateBlockMap;
SharedLock RenderStateBlock::s_stateBlockMapMutex;
size_t RenderStateBlock::s_defaultCacheValue = 0;

RenderStateBlock::RenderStateBlock()
    : GUIDWrapper(), 
     _cachedHash(0)
{
    setDefaultValues();
    clean();
    if (s_defaultCacheValue == 0) {
        s_defaultCacheValue = _cachedHash;
    }
}

RenderStateBlock::RenderStateBlock(const RenderStateBlock& other)
    : GUIDWrapper(other),
     _colourWrite(other._colourWrite),
     _blendEnable(other._blendEnable),
     _blendSrc(other._blendSrc),
     _blendDest(other._blendDest),
     _blendOp(other._blendOp),
     _cullMode(other._cullMode),
     _cullEnabled(other._cullEnabled),
     _zEnable(other._zEnable),
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

void RenderStateBlock::flipCullMode() {
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

void RenderStateBlock::setCullMode(CullMode mode) {
    _cullMode = mode;
    _cullEnabled = _cullMode == CullMode::NONE ? false : true;
    clean();
}

void RenderStateBlock::setZRead(const bool enable) {
    _zEnable = enable;

    clean();
}

void RenderStateBlock::setBlend(bool enable,
                                BlendProperty src,
                                BlendProperty dest,
                                BlendOperation op) {
    _blendEnable = enable;
    _blendSrc = src;
    _blendDest = dest;
    _blendOp = op;

    clean();
}

void RenderStateBlock::setColourWrites(bool red,
                                       bool green,
                                       bool blue,
                                       bool alpha) {
    _colourWrite.b[0] = red ? 1 : 0;
    _colourWrite.b[1] = green ? 1 : 0;
    _colourWrite.b[2] = blue ? 1 : 0;
    _colourWrite.b[3] = alpha ? 1 : 0;

    clean();
}

void RenderStateBlock::setZBias(F32 zBias, F32 zUnits) {
    _zBias = zBias;
    _zUnits = zUnits;

    clean();
}

void RenderStateBlock::setZFunc(ComparisonFunction zFunc) {
    _zFunc = zFunc;

    clean();
}

void RenderStateBlock::setFillMode(FillMode mode) {
    _fillMode = mode;

    clean();
}

void RenderStateBlock::setStencilReadWriteMask(U32 read, U32 write) {
    _stencilMask = read;
    _stencilWriteMask = write;

    clean();
}

void RenderStateBlock::setStencil(bool enable,
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

void RenderStateBlock::setDefaultValues() {
    _zBias = 0.0f;
    _zUnits = 1.0f;
    _zFunc = ComparisonFunction::LEQUAL;
    _colourWrite.b[0] = _colourWrite.b[1] = _colourWrite.b[2] = _colourWrite.b[3] = 1;
    _blendEnable = false;
    _blendSrc = BlendProperty::ONE;
    _blendDest = BlendProperty::ONE;
    _blendOp = BlendOperation::ADD;
    _zEnable = true;
    _cullMode = CullMode::CW;
    _cullEnabled = true;
    _fillMode = FillMode::SOLID;
    _stencilMask = 0xFFFFFFFF;
    _stencilWriteMask = 0xFFFFFFFF;
    _stencilEnable = false;
    _stencilRef = 0;
    _stencilFailOp = StencilOperation::KEEP;
    _stencilZFailOp = StencilOperation::KEEP;
    _stencilPassOp = StencilOperation::KEEP;
    _stencilFunc = ComparisonFunction::NEVER;
    _cachedHash = s_defaultCacheValue;
}

void RenderStateBlock::clean() {
    size_t previousCache = _cachedHash;

    // Avoid small float rounding errors offsetting the general hash value
    U32 zBias = to_U32(std::floor((_zBias * 1000.0f) + 0.5f));
    U32 zUnits = to_U32(std::floor((_zUnits * 1000.0f) + 0.5f));

    _cachedHash = 0;
    Util::Hash_combine(_cachedHash, _colourWrite.i);
    Util::Hash_combine(_cachedHash, _blendEnable);
    Util::Hash_combine(_cachedHash, to_U32(_blendSrc));
    Util::Hash_combine(_cachedHash, to_U32(_blendDest));
    Util::Hash_combine(_cachedHash, to_U32(_blendOp));
    Util::Hash_combine(_cachedHash, to_U32(_cullMode));
    Util::Hash_combine(_cachedHash, _cullEnabled);
    Util::Hash_combine(_cachedHash, _zEnable);
    Util::Hash_combine(_cachedHash, to_U32(_zFunc));
    Util::Hash_combine(_cachedHash, zBias);
    Util::Hash_combine(_cachedHash, zUnits);
    Util::Hash_combine(_cachedHash, _stencilEnable);
    Util::Hash_combine(_cachedHash, _stencilRef);
    Util::Hash_combine(_cachedHash, _stencilMask);
    Util::Hash_combine(_cachedHash, _stencilWriteMask);
    Util::Hash_combine(_cachedHash, to_U32(_stencilFailOp));
    Util::Hash_combine(_cachedHash, to_U32(_stencilZFailOp));
    Util::Hash_combine(_cachedHash, to_U32(_stencilPassOp));
    Util::Hash_combine(_cachedHash, to_U32(_stencilFunc));
    Util::Hash_combine(_cachedHash, to_U32(_fillMode));

    if (previousCache != _cachedHash) {
        WriteLock w_lock(s_stateBlockMapMutex);
        hashAlg::emplace(s_stateBlockMap, _cachedHash, *this);
    }
}

void RenderStateBlock::init() {
}

void RenderStateBlock::clear() {
    WriteLock w_lock(s_stateBlockMapMutex);
    s_stateBlockMap.clear();
}

/// Return the render state block defined by the specified hash value.
const RenderStateBlock& RenderStateBlock::get(size_t renderStateBlockHash) {
    bool blockFound = false;
    const RenderStateBlock& block = get(renderStateBlockHash, blockFound);
    // Assert if it doesn't exist. Avoids programming errors.
    DIVIDE_ASSERT(blockFound, "RenderStateBlock error: Invalid render state block hash specified for getRenderStateBlock!");
    // Return the state block's descriptor
    return block;
}

const RenderStateBlock& RenderStateBlock::get(size_t renderStateBlockHash, bool& blockFound) {
    blockFound = false;

    ReadLock r_lock(s_stateBlockMapMutex);
    // Find the render state block associated with the received hash value
    RenderStateMap::const_iterator it = s_stateBlockMap.find(renderStateBlockHash);
    if(it != std::cend(s_stateBlockMap) ) {
        blockFound = true;
        return it->second;
    }

    return s_stateBlockMap.find(s_defaultCacheValue)->second;
}

};
