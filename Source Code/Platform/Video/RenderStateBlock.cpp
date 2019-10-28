#include "stdafx.h"

#include "Headers/RenderStateBlock.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderStateBlock::RenderStateMap RenderStateBlock::s_stateBlockMap;
SharedMutex RenderStateBlock::s_stateBlockMapMutex;
size_t RenderStateBlock::s_defaultCacheValue = 0;

RenderStateBlock::RenderStateBlock() noexcept
    : GUIDWrapper()
{
    setDefaultValues();
    if (s_defaultCacheValue == 0) {
        _dirty = true;
        s_defaultCacheValue = getHash();
        _hash = s_defaultCacheValue;
    }
}

RenderStateBlock::RenderStateBlock(const RenderStateBlock& other)
    : GUIDWrapper(other),
     _dirty(true),
     _colourWrite(other._colourWrite),
     _cullMode(other._cullMode),
     _cullEnabled(other._cullEnabled),
     _frontFaceCCW(other._frontFaceCCW),
     _depthTestEnabled(other._depthTestEnabled),
     _zFunc(other._zFunc),
     _zBias(other._zBias),
     _zUnits(other._zUnits),
     _scissorTestEnabled(other._scissorTestEnabled),
     _stencilEnable(other._stencilEnable),
     _stencilRef(other._stencilRef),
     _stencilMask(other._stencilMask),
     _stencilWriteMask(other._stencilWriteMask),
     _stencilFailOp(other._stencilFailOp),
     _stencilZFailOp(other._stencilZFailOp),
     _stencilPassOp(other._stencilPassOp),
     _stencilFunc(other._stencilFunc),
     _fillMode(other._fillMode),
     _tessControlPoints(other._tessControlPoints)
{

    _hash = other._hash;
}

void RenderStateBlock::flipCullMode() {
    if (cullMode() == CullMode::NONE) {
        _cullMode = CullMode::ALL;
    } else if (cullMode() == CullMode::ALL) {
        _cullMode = CullMode::NONE;
    } else if (cullMode() == CullMode::CW) {
        _cullMode = CullMode::CCW;
    } else if (cullMode() == CullMode::CCW) {
        _cullMode = CullMode::CW;
    }
    _dirty = true;
}

void RenderStateBlock::setCullMode(CullMode mode) {
    if (cullMode() != mode) {
        _cullMode = mode;
        _cullEnabled = (mode != CullMode::NONE);
        _dirty = true;
    }
}

void RenderStateBlock::flipFrontFace() {
    _frontFaceCCW = !frontFaceCCW();
    _dirty = true;
}

void RenderStateBlock::setFrontFaceCCW(bool state) {
    if (_frontFaceCCW != state) {
        _frontFaceCCW = state;
        _dirty = true;
    }
}
void RenderStateBlock::depthTestEnabled(const bool enable) {
    if (_depthTestEnabled != enable) {
        _depthTestEnabled = enable;
        _dirty = true;
    }
}

void RenderStateBlock::setColourWrites(bool red,
                                       bool green,
                                       bool blue,
                                       bool alpha) {
    P32 rgba = {};
    rgba.b[0] = (red ? 1 : 0);
    rgba.b[1] = (green ? 1 : 0);
    rgba.b[2] = (blue ? 1 : 0);
    rgba.b[3] = (alpha ? 1 : 0);

    if (_colourWrite != rgba) {
        _colourWrite = rgba;
        _dirty = true;
    }
}

void RenderStateBlock::setScissorTest(const bool enable) {
    if (_scissorTestEnabled != enable) {
        _scissorTestEnabled = enable;
        _dirty = true;
    }
}

void RenderStateBlock::setZBias(F32 zBias, F32 zUnits) {
    if (!COMPARE(_zBias, zBias) || !COMPARE(_zUnits, zUnits)) {
        _zBias = zBias;
        _zUnits = zUnits;
        _dirty = true;
    }
}

void RenderStateBlock::setZFunc(ComparisonFunction zFunc) {
    if (_zFunc != zFunc) {
        _zFunc = zFunc;
        _dirty = true;
    }
}

void RenderStateBlock::setFillMode(FillMode mode) {
    if (_fillMode != mode) {
        _fillMode = mode;
        _dirty = true;
    }
}

void RenderStateBlock::setTessControlPoints(U32 count) {
    if (_tessControlPoints != count) {
        _tessControlPoints = count;
        _dirty = true;
    }
}

void RenderStateBlock::setStencilReadWriteMask(U32 read, U32 write) {
    if (_stencilMask != read || _stencilWriteMask != write) {
        _stencilMask = read;
        _stencilWriteMask = write;
        _dirty = true;
    }
}

void RenderStateBlock::setStencil(bool enable,
                                  U32 stencilRef,
                                  StencilOperation stencilFailOp,
                                  StencilOperation stencilZFailOp,
                                  StencilOperation stencilPassOp,
                                  ComparisonFunction stencilFunc) {
    if (_stencilEnable != enable ||
        _stencilRef != stencilRef ||
        _stencilFailOp != stencilFailOp ||
        _stencilZFailOp != stencilZFailOp ||
        _stencilPassOp != stencilPassOp ||
        _stencilFunc != stencilFunc) 
    {
        _stencilEnable = enable;
        _stencilRef = stencilRef;
        _stencilFailOp = stencilFailOp;
        _stencilZFailOp = stencilZFailOp;
        _stencilPassOp = stencilPassOp;
        _stencilFunc = stencilFunc;
        _dirty = true;
    }
}

void RenderStateBlock::setDefaultValues() {
    _zBias = 0.0f;
    _zUnits = 0.0f;
    _zFunc = ComparisonFunction::LEQUAL;
    _colourWrite.b[0] = _colourWrite.b[1] = _colourWrite.b[2] = _colourWrite.b[3] = 1;
    _depthTestEnabled = true;
    _scissorTestEnabled = false;
    _cullMode = CullMode::CW;
    _cullEnabled = true;
    _frontFaceCCW = true;
    _fillMode = FillMode::SOLID;
    _tessControlPoints = 3;
    _stencilMask = 0xFFFFFFFF;
    _stencilWriteMask = 0xFFFFFFFF;
    _stencilEnable = false;
    _stencilRef = 0;
    _stencilFailOp = StencilOperation::KEEP;
    _stencilZFailOp = StencilOperation::KEEP;
    _stencilPassOp = StencilOperation::KEEP;
    _stencilFunc = ComparisonFunction::NEVER;
    _hash = s_defaultCacheValue;
}

void RenderStateBlock::init() {
}

void RenderStateBlock::clear() {
    UniqueLockShared w_lock(s_stateBlockMapMutex);
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

    SharedLock r_lock(s_stateBlockMapMutex);
    // Find the render state block associated with the received hash value
    RenderStateMap::const_iterator it = s_stateBlockMap.find(renderStateBlockHash);
    if(it != std::cend(s_stateBlockMap) ) {
        blockFound = true;
        return it->second;
    }

    return s_stateBlockMap.find(s_defaultCacheValue)->second;
}

size_t RenderStateBlock::getHash() const {
    if (_dirty) {
        const size_t previousCache = Hashable::getHash();

        // Avoid small float rounding errors offsetting the general hash value
        const U32 zBias = to_U32(std::floor((_zBias * 1000.0f) + 0.5f));
        const U32 zUnits = to_U32(std::floor((_zUnits * 1000.0f) + 0.5f));

        _hash = 59;
        Util::Hash_combine(_hash, _colourWrite.i);
        Util::Hash_combine(_hash, to_U32(_cullMode));
        Util::Hash_combine(_hash, _cullEnabled);
        Util::Hash_combine(_hash, _depthTestEnabled);
        Util::Hash_combine(_hash, to_U32(_zFunc));
        Util::Hash_combine(_hash, zBias);
        Util::Hash_combine(_hash, zUnits);
        Util::Hash_combine(_hash, _scissorTestEnabled);
        Util::Hash_combine(_hash, _stencilEnable);
        Util::Hash_combine(_hash, _stencilRef);
        Util::Hash_combine(_hash, _stencilMask);
        Util::Hash_combine(_hash, _stencilWriteMask);
        Util::Hash_combine(_hash, _frontFaceCCW);
        Util::Hash_combine(_hash, to_U32(_stencilFailOp));
        Util::Hash_combine(_hash, to_U32(_stencilZFailOp));
        Util::Hash_combine(_hash, to_U32(_stencilPassOp));
        Util::Hash_combine(_hash, to_U32(_stencilFunc));
        Util::Hash_combine(_hash, to_U32(_fillMode));
        Util::Hash_combine(_hash, to_U32(_tessControlPoints));

        if (previousCache != _hash) {
            UniqueLockShared w_lock(s_stateBlockMapMutex);
            hashAlg::insert(s_stateBlockMap, _hash, *this);
        }
        _dirty = false;
    }

    return Hashable::getHash();
}
};
