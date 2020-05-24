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
#ifndef _SCENE_ENVIRONMENT_PROBE_POOL_H_
#define _SCENE_ENVIRONMENT_PROBE_POOL_H_

#include "Scenes/Headers/SceneComponent.h"

namespace Divide {

namespace GFX {
    class CommandBuffer;
};

FWD_DECLARE_MANAGED_STRUCT(DebugView);

class Camera;
class EnvironmentProbeComponent;
using EnvironmentProbeList = vectorEASTL<EnvironmentProbeComponent*>;

class SceneRenderState;
class SceneEnvironmentProbePool final : public SceneComponent {
public:
    SceneEnvironmentProbePool(Scene& parentScene);
    ~SceneEnvironmentProbePool();

    const EnvironmentProbeList& sortAndGetLocked(const vec3<F32>& position);
    const EnvironmentProbeList& getLocked();

    EnvironmentProbeComponent* registerProbe(EnvironmentProbeComponent* probe);
    void unregisterProbe(EnvironmentProbeComponent* probe);

    void prepare(GFX::CommandBuffer& bufferInOut);

    void lockProbeList();
    void unlockProbeList();

    void debugProbe(EnvironmentProbeComponent* probe);
    POINTER_R(EnvironmentProbeComponent, debugProbe, nullptr);

    static vectorEASTL<Camera*>& probeCameras() noexcept { return s_probeCameras; }

protected:
    SharedMutex _probeLock;
    EnvironmentProbeList _envProbes;

    static vectorEASTL<DebugView_ptr> s_debugViews;
    static vectorEASTL<Camera*> s_probeCameras;
};

}; //namespace Divide

#endif //_SCENE_ENVIRONMENT_PROBE_POOL_H_
