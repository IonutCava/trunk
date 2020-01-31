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
#ifndef _BLENDING_PROPERTIES_H_
#define _BLENDING_PROPERTIES_H_

#include "RenderAPIEnums.h"

namespace Divide {

struct BlendingProperties {
    BlendProperty  _blendSrc = BlendProperty::ONE;
    BlendProperty  _blendDest = BlendProperty::ZERO;
    BlendOperation _blendOp = BlendOperation::ADD;

    BlendProperty  _blendSrcAlpha = BlendProperty::ONE;
    BlendProperty  _blendDestAlpha = BlendProperty::ZERO;
    BlendOperation _blendOpAlpha = BlendOperation::COUNT;

    bool _enabled = false;

    inline bool operator==(const BlendingProperties& rhs) const noexcept {
        return _enabled == rhs._enabled &&
               _blendSrc == rhs._blendSrc &&
               _blendDest == rhs._blendDest &&
               _blendOp == rhs._blendOp &&
               _blendSrcAlpha == rhs._blendSrcAlpha &&
               _blendDestAlpha == rhs._blendDestAlpha &&
               _blendOpAlpha == rhs._blendOpAlpha;
    }

    inline bool operator!=(const BlendingProperties& rhs) const noexcept {
        return _enabled != rhs._enabled ||
               _blendSrc != rhs._blendSrc ||
               _blendDest != rhs._blendDest ||
               _blendOp != rhs._blendOp ||
               _blendSrcAlpha != rhs._blendSrcAlpha ||
               _blendDestAlpha != rhs._blendDestAlpha ||
               _blendOpAlpha != rhs._blendOpAlpha;
    }

    inline bool blendEnabled() const noexcept {
        return _enabled;
    }

    inline void reset() noexcept {
        _enabled = false;

        _blendSrc = BlendProperty::ONE;
        _blendDest = BlendProperty::ZERO;
        _blendOp = BlendOperation::ADD;

        _blendSrcAlpha = BlendProperty::ONE;
        _blendDestAlpha = BlendProperty::ZERO;
        _blendOpAlpha = BlendOperation::ADD;
    }
};

}; //namespace Divide

#endif //_BLENDING_PROPERTIES_H_