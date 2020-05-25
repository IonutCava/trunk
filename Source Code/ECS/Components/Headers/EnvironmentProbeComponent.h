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
#ifndef _ENVIRONMENT_PROBE_COMPONENT_H_
#define _ENVIRONMENT_PROBE_COMPONENT_H_

#include "SGNComponent.h"

#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Scene;
class Pipeline;
class GFXDevice;
class IMPrimitive;
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

namespace GFX {
    class CommandBuffer;
}; //namespace GFX


class EnvironmentProbeComponent final : public BaseComponentType<EnvironmentProbeComponent, ComponentType::ENVIRONMENT_PROBE>,
                                        public GUIDWrapper {
public:
    enum class ProbeType {
        TYPE_INFINITE = 0,
        TYPE_LOCAL,
        COUNT
    };
public:
    explicit EnvironmentProbeComponent(SceneGraphNode& sgn, PlatformContext& context);
    ~EnvironmentProbeComponent();

    void PreUpdate(const U64 deltaTime) final;

    void refresh(GFX::CommandBuffer& bufferInOut);
    void setUpdateRate(U8 rate);
    void setBounds(const vec3<F32>& min, const vec3<F32>& max);
    void setBounds(const vec3<F32>& center, F32 radius);

    F32 distanceSqTo(const vec3<F32>& pos) const;
    
    std::array<Camera*, 6> probeCameras() const noexcept;

    PROPERTY_R_IW(U16, rtLayerIndex, 0u);
    PROPERTY_RW(bool, showParallaxAABB, false);

protected:
    void OnData(const ECS::CustomEvent& data) final;

    SceneGraphNode* findNodeToIgnore() const noexcept;

protected:
    BoundingBox _aabb, _refaabb;
    U8 _updateRate = 1u;
    U8 _currentUpdateCall = 0u;
    ProbeType _type = ProbeType::TYPE_LOCAL;

private:
    bool _drawImpostor = false;
};

INIT_COMPONENT(EnvironmentProbeComponent);

}; //namespace Divide

#endif //_ENVIRONMENT_PROBE_COMPONENT_H_
