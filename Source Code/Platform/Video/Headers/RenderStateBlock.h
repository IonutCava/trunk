/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _RENDER_STATE_BLOCK_H
#define _RENDER_STATE_BLOCK_H

#include "RenderAPIEnums.h"
#include "Core/Headers/Hashable.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class RenderStateBlock : public GUIDWrapper, public Hashable {
    public:
        using RenderStateMap = hashMap<size_t, RenderStateBlock, NoHash<size_t>>;

       static void init();
       static void clear();
       /// Retrieve a state block by hash value.
       /// If the hash value doesn't exist in the state block map, return the default state block
       static const RenderStateBlock& get(size_t renderStateBlockHash);
       /// Returns false if the specified hash is not found in the map
       static const RenderStateBlock& get(size_t renderStateBlockHash, bool& blockFound);

    protected:
        static RenderStateMap s_stateBlockMap;
        static SharedMutex s_stateBlockMapMutex;
        static size_t s_defaultCacheValue;

    public:
        RenderStateBlock() noexcept;
        RenderStateBlock(const RenderStateBlock& b);

        inline bool operator==(const RenderStateBlock& rhs) const {
            return getHash() == rhs.getHash();
        }

        inline bool operator!=(const RenderStateBlock& rhs) const {
            return getHash() != rhs.getHash();
        }

        void setDefaultValues();

        void setFillMode(FillMode mode);
        void setTessControlPoints(U32 count);
        void setZBias(F32 zBias, F32 zUnits);
        void setZFunc(ComparisonFunction zFunc = ComparisonFunction::LEQUAL);
        void flipCullMode();
        void flipFrontFace();
        void setCullMode(CullMode mode);
        void setFrontFaceCCW(bool state);
        void depthTestEnabled(const bool enable);
        void setScissorTest(const bool enable);

        void setStencil(bool enable,
                        U32 stencilRef = 0,
                        StencilOperation stencilFailOp  = StencilOperation::KEEP,
                        StencilOperation stencilZFailOp = StencilOperation::KEEP,
                        StencilOperation stencilPassOp  = StencilOperation::KEEP,
                        ComparisonFunction stencilFunc = ComparisonFunction::NEVER);

        void setStencilReadWriteMask(U32 read, U32 write);

        void setColourWrites(bool red, bool green, bool blue, bool alpha);

        size_t getHash() const override;

    public:
        PROPERTY_R(P32, colourWrite);
        PROPERTY_R(F32, zBias, 0.0f);
        PROPERTY_R(F32, zUnits, 0.0f);
        PROPERTY_R(U32, tessControlPoints, 3);
        PROPERTY_R(U32, stencilRef, 0u);
        PROPERTY_R(U32, stencilMask, 0xFFFFFFFF);
        PROPERTY_R(U32, stencilWriteMask, 0xFFFFFFFF);

        PROPERTY_R(ComparisonFunction, zFunc, ComparisonFunction::LEQUAL);
        PROPERTY_R(StencilOperation, stencilFailOp, StencilOperation::KEEP);
        PROPERTY_R(StencilOperation, stencilZFailOp, StencilOperation::KEEP);
        PROPERTY_R(StencilOperation, stencilPassOp, StencilOperation::KEEP);
        PROPERTY_R(ComparisonFunction, stencilFunc, ComparisonFunction::NEVER);

        PROPERTY_R(CullMode, cullMode, CullMode::CW);
        PROPERTY_R(FillMode, fillMode, FillMode::SOLID);

        PROPERTY_R(bool, frontFaceCCW, true);
        PROPERTY_R(bool, cullEnabled, true);
        PROPERTY_R(bool, scissorTestEnabled, false);
        PROPERTY_R(bool, depthTestEnabled, true);
        PROPERTY_R(bool, stencilEnable, false);

        mutable bool _dirty = true;

    private:
        void operator=(const RenderStateBlock& b) = delete;
};

};  // namespace Divide
#endif
