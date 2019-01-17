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

#ifndef _DIRECTIONAL_LIGHT_COMPONENT_H_
#define _DIRECTIONAL_LIGHT_COMPONENT_H_

#include "Rendering/Lighting/Headers/Light.h"

namespace Divide {

class DirectionalLightComponent : public BaseComponentType<DirectionalLightComponent, ComponentType::DIRECTIONAL_LIGHT>,
                                  public Light
{
   public:
    using Light::getSGN;

   public:

    explicit DirectionalLightComponent(SceneGraphNode& sgn, PlatformContext& context);
    ~DirectionalLightComponent();

    inline U8 csmSplitCount() const { return _csmSplitCount; }
    inline void csmSplitCount(U8 splitCount) { _csmSplitCount = splitCount; }

    inline F32 csmNearClipOffset() const { return _csmNearClipOffset; }
    inline void csmNearClipOffset(F32 offset) { _csmNearClipOffset = offset; }

   protected:
    /// CSM split count
    U8 _csmSplitCount;
    /// CSM extra back up distance for light position
    F32 _csmNearClipOffset;
};

INIT_COMPONENT(DirectionalLightComponent);

};  // namespace Divide

#endif //_DIRECTIONAL_LIGHT_COMPONENT_H_