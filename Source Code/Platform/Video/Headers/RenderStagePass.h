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

static constexpr U8  g_AllVariantsID = std::numeric_limits<U8>::max();
static constexpr U16 g_AllPassID = std::numeric_limits<U16>::max();
static constexpr U16 g_AllIndicesID = g_AllPassID;

struct RenderStagePass {
    RenderStagePass() = default;
    ~RenderStagePass() = default;

    explicit RenderStagePass(const RenderStage stage, const RenderPassType passType, const U8 variant = 0u, const U16 index = 0u, const U16 pass = 0u)
        : _stage(stage),
          _passType(passType),
          _variant(variant),
          _index(index),
          _pass(pass)
    {}

    RenderStage _stage = RenderStage::COUNT;
    RenderPassType _passType = RenderPassType::COUNT;
    U8 _variant = 0;
    U16 _index = 0u; //usually some kind of type info (reflector/refractor index, light type, etc)
    U16 _pass = 0u; //usually some kind of actual pass index (eg. cube face we are rendering into)

    [[nodiscard]] bool isDepthPass() const noexcept {
        return _stage == RenderStage::SHADOW || _passType == RenderPassType::PRE_PASS;
    }

    /// This ignores the variant and pass index flags!
    [[nodiscard]] U8 baseIndex() const noexcept {
        return baseIndex(_stage, _passType);
    }

    static constexpr U8 baseIndex(const RenderStage stage, const RenderPassType passType) noexcept {
        return static_cast<U8>(to_base(stage) + to_base(passType) * to_base(RenderStage::COUNT));
    }

    static RenderStagePass fromBaseIndex(const U8 baseIndex) noexcept {
        const RenderStage stage = static_cast<RenderStage>(baseIndex % to_base(RenderStage::COUNT));
        const RenderPassType pass = static_cast<RenderPassType>(baseIndex / to_base(RenderStage::COUNT));
        return RenderStagePass(stage, pass);
    }

    static U16 indexForStage(const RenderStagePass& renderStagePass) noexcept {
        if (renderStagePass._stage == RenderStage::SHADOW) {
            constexpr U16 offsetDir = 0u;
            constexpr U16 offsetPoint = offsetDir + Config::Lighting::MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS * Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT;
            constexpr U16 offsetSpot = offsetPoint + Config::Lighting::MAX_SHADOW_CASTING_POINT_LIGHTS * 6;
            
            if (renderStagePass._variant == to_base(LightType::DIRECTIONAL)) {
                return offsetDir + renderStagePass._index * Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT + renderStagePass._pass;
            } else if (renderStagePass._variant == to_base(LightType::POINT)) {
                return offsetPoint + renderStagePass._index * 6 + renderStagePass._pass;
            } else /*if (renderStagePass._variant == to_base(LightType::SPOT)*/{
                return offsetSpot + renderStagePass._index;
            }
        } else if (renderStagePass._stage == RenderStage::REFLECTION) {
            if (renderStagePass._variant == to_base(ReflectorType::PLANAR)) {
                return renderStagePass._index;
            } else {
                return Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + (renderStagePass._index * 6 + renderStagePass._pass);

            }
        } else {
            assert(renderStagePass._pass == 0u);
            return renderStagePass._index;
        }
    }

    static U8 passCountForStage(const RenderStage renderStage) noexcept {
        if (renderStage == RenderStage::REFLECTION) {
            return Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + //Worst case, all nodes need cubemaps
                   Config::MAX_REFLECTIVE_PROBES_PER_PASS * 6;
        } else if (renderStage == RenderStage::SHADOW) {
            return Config::Lighting::MAX_SHADOW_PASSES;
        }

        return to_U8(1u);
    }

    bool operator==(const RenderStagePass& other) const noexcept {
        return _variant == other._variant &&
               _pass == other._pass &&
               _index == other._index &&
               _stage == other._stage &&
               _passType == other._passType;
    }

    bool operator!=(const RenderStagePass& other) const noexcept {
        return _variant != other._variant ||
               _pass != other._pass ||
               _index != other._index ||
               _stage != other._stage ||
               _passType != other._passType;
    }

};

}; //namespace Divide

#endif //_RENDER_STAGE_PASS_H_