/* Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _SGN_COMPONENT_H_
#define _SGN_COMPONENT_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

/// A generic component for the SceneGraphNode class
enum class RenderStage : U32;
class SceneGraphNode;
class RenderStagePass;
class SceneRenderState;

class SGNComponent : private NonCopyable {
   public:
    enum class ComponentType : U32 {
        ANIMATION = toBit(1),
        INVERSE_KINEMATICS = toBit(2),
        RAGDOLL = toBit(3),
        NAVIGATION = toBit(4),
        PHYSICS = toBit(5),
        BOUNDS = toBit(6),
        RENDERING = toBit(7),
        NETWORKING = toBit(8),
        UNIT = toBit(9),
        COUNT = 10
    };
    
    SGNComponent(ComponentType type, SceneGraphNode& parentSGN);
    virtual ~SGNComponent();

    virtual bool onRender(const SceneRenderState& sceneRenderState,
                          const RenderStagePass& renderStagePass) {
        ACKNOWLEDGE_UNUSED(sceneRenderState);
        ACKNOWLEDGE_UNUSED(renderStagePass);
        return true;
    }

    virtual void update(const U64 deltaTimeUS) {
        _deltaTimeUS = deltaTimeUS;
        _elapsedTimeUS += deltaTimeUS;
    }

    virtual void resetTimers() {
        _deltaTimeUS = 0UL;
        _elapsedTimeUS = 0UL;
    }

    virtual void setActive(const bool state) {
        _parentNodeActive = state;
    }

    virtual void postLoad() {
    }

    inline ComponentType getType() const { return _type; }
    inline SceneGraphNode& getSGN() const { return _parentSGN; }
    
   protected:
    std::atomic_bool _parentNodeActive;
    /// Pointer to the SGN owning this instance of AnimationComponent
    SceneGraphNode& _parentSGN;
    ComponentType _type;
    U64 _elapsedTimeUS;
    U64 _deltaTimeUS;
};

};  // namespace Divide
#endif
