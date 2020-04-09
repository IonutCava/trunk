/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _RENDER_STAGE_PASS_H_
#define _RENDER_STAGE_PASS_H_

#include "RenderAPIEnums.h"

namespace Divide {

struct RenderStagePass {
    RenderStage _stage = RenderStage::COUNT;
    RenderPassType _passType = RenderPassType::COUNT;
    U8 _variant = 0;

    union {
        U32 _passIndex = 0;
        struct {
            U16 _indexA; //usually some kind of type info (reflector/refractor index, light type, etc)
            U16 _indexB; //usually some kind of actual pass index (eg. cube face we are rendering into)
        };
    };

    inline bool isDepthPass() const noexcept {
        return _stage == RenderStage::SHADOW || _passType == RenderPassType::PRE_PASS;
    }

    /// This ignores the variant and pass index flags!
    inline U8 baseIndex() const noexcept {
        return baseIndex(_stage, _passType);
    }

    static U8 baseIndex(RenderStage stage, RenderPassType passType) noexcept {
        return static_cast<U8>(to_base(stage) + to_base(passType) * to_base(RenderStage::COUNT));
    }

    static RenderStagePass fromBaseIndex(U8 baseIndex) noexcept {
        const RenderStage stage = static_cast<RenderStage>(baseIndex % to_base(RenderStage::COUNT));
        const RenderPassType pass = static_cast<RenderPassType>(baseIndex / to_base(RenderStage::COUNT));
        return { stage, pass };
    }

    inline bool operator==(const RenderStagePass& other) const noexcept {
        return _variant == other._variant &&
               _passIndex == other._passIndex &&
               _stage == other._stage &&
               _passType == other._passType;
    }

    inline bool operator!=(const RenderStagePass& other) const noexcept {
        return _variant != other._variant ||
               _passIndex != other._passIndex ||
               _stage != other._stage ||
               _passType != other._passType;
    }

};

}; //namespace Divide

#endif //_RENDER_STAGE_PASS_H_