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

#ifndef _SCENE_ENVIRONMENT_PROBE_POOL_H_
#define _SCENE_ENVIRONMENT_PROBE_POOL_H_

#include "Rendering/Headers/EnvironmentProbe.h"
#include "Scenes/Headers/SceneComponent.h"

namespace Divide {

class SceneRenderState;
class SceneEnvironmentProbePool : public SceneComponent {
public:
    SceneEnvironmentProbePool(Scene& parentScene);
    ~SceneEnvironmentProbePool();

    const EnvironmentProbeList& getNearestSorted(const vec3<F32>& position);

    EnvironmentProbe_wptr addInfiniteProbe(const vec3<F32>& position);
    EnvironmentProbe_wptr addLocalProbe(const vec3<F32>& bbMin, const vec3<F32>& bbMax);
    void removeProbe(EnvironmentProbe_wptr probe);

    void debugDraw(GFX::CommandBuffer& bufferInOut);

protected:
    bool _isSorted;
    EnvironmentProbeList _envProbes;
    EnvironmentProbeList _sortedProbes;
};

}; //namespace Divide

#endif //_SCENE_ENVIRONMENT_PROBE_POOL_H_
