/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _RENDER_STATE_BLOCK_H
#define _RENDER_STATE_BLOCK_H

#include "RenderAPIEnums.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class RenderStateBlock : public GUIDWrapper {
   public:
      typedef hashMapImpl<size_t, RenderStateBlock> RenderStateMap;
      static RenderStateMap s_stateBlockMap;
   private:
      static SharedLock s_stateBlockMapMutex;
   public:
       static void init();
       static void clear();
       /// Retrieve a state block by hash value.
       /// If the hash value doesn't exist in the state block map, return the default state block
       static const RenderStateBlock& get(size_t renderStateBlockHash);
       /// Returns false if the specified hash is not found in the map
       static const RenderStateBlock& get(size_t renderStateBlockHash, bool& blockFound);

   protected:
    /// Colour Writes
    P32 _colourWrite;

    // Blending
    bool _blendEnable;
    BlendProperty _blendSrc;
    BlendProperty _blendDest;
    BlendOperation _blendOp;

    /// Rasterizer
    CullMode _cullMode;
    bool _cullEnabled;

    /// Depth
    bool _zEnable;
    ComparisonFunction _zFunc;
    F32 _zBias;
    F32 _zUnits;

    /// Stencil
    bool _stencilEnable;
    U32 _stencilRef;
    U32 _stencilMask;
    U32 _stencilWriteMask;
    StencilOperation _stencilFailOp;
    StencilOperation _stencilZFailOp;
    StencilOperation _stencilPassOp;
    ComparisonFunction _stencilFunc;

    FillMode _fillMode;

    size_t _cachedHash;

    static size_t s_defaultCacheValue;

  private:
    void operator=(const RenderStateBlock& b) = delete;
    void clean();

   public:
    RenderStateBlock();
    RenderStateBlock(const RenderStateBlock& b);

    void setDefaultValues();

    void setFillMode(FillMode mode);
    void setZBias(F32 zBias, F32 zUnits);
    void setZFunc(ComparisonFunction zFunc = ComparisonFunction::LEQUAL);
    void flipCullMode();
    void setCullMode(CullMode mode);
    void setZRead(const bool enable);

    void setBlend(
        bool enable,
        BlendProperty src = BlendProperty::SRC_ALPHA,
        BlendProperty dest = BlendProperty::INV_SRC_ALPHA,
        BlendOperation op = BlendOperation::ADD);

    void setStencil(
        bool enable, U32 stencilRef = 0,
        StencilOperation stencilFailOp =
            StencilOperation::KEEP,
        StencilOperation stencilZFailOp =
            StencilOperation::KEEP,
        StencilOperation stencilPassOp =
            StencilOperation::KEEP,
        ComparisonFunction stencilFunc = ComparisonFunction::NEVER);

    void setStencilReadWriteMask(U32 read, U32 write);

    void setColourWrites(bool red, bool green, bool blue, bool alpha);

    inline P32 colourWrite() const {
        return _colourWrite;
    }
    inline bool blendEnable() const {
        return _blendEnable;
    }
    inline BlendProperty blendSrc() const {
        return _blendSrc;
    }
    inline BlendProperty blendDest() const {
        return _blendDest;
    }
    inline BlendOperation blendOp() const {
        return _blendOp;
    }
    inline CullMode cullMode() const {
        return _cullMode;
    }
    inline bool cullEnabled() const {
        return _cullEnabled;
    }
    inline bool zEnable() const {
        return _zEnable;
    }
    inline ComparisonFunction zFunc() const {
        return _zFunc;
    }
    inline F32 zBias() const {
        return _zBias;
    }
    inline F32 zUnits() const {
        return _zUnits;
    }
    inline bool stencilEnable() const {
        return _stencilEnable;
    }
    inline U32 stencilRef() const {
        return _stencilRef;
    }
    inline U32 stencilMask() const {
        return _stencilMask;
    }
    inline U32 stencilWriteMask() const {
        return _stencilWriteMask;
    }
    inline StencilOperation stencilFailOp() const {
        return _stencilFailOp;
    }
    inline StencilOperation stencilZFailOp() const {
        return _stencilZFailOp;
    }
    inline StencilOperation stencilPassOp() const {
        return _stencilPassOp;
    }
    inline ComparisonFunction stencilFunc() const {
        return _stencilFunc;
    }
    inline FillMode fillMode() const {
        return _fillMode;
    }
    inline size_t getHash() const {
        return _cachedHash;
    }
    bool operator==(const RenderStateBlock& RSBD) const {
        return _cachedHash == RSBD._cachedHash;
    }
    bool operator!=(const RenderStateBlock& RSBD) const {
        return _cachedHash != RSBD._cachedHash;
    }
};

};  // namespace Divide
#endif
