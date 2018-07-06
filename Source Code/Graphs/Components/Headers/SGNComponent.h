/* Copyright (c) 2015 DIVIDE-Studio
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

class SGNComponent : private NonCopyable {
   public:
    enum class ComponentType : U32 {
        ANIMATION = 0,
        NAVIGATION = 1,
        PHYSICS = 2,
        RENDERING = 3,
        COUNT
    };
    
    SGNComponent(ComponentType type, SceneGraphNode& parentSGN);
    virtual ~SGNComponent();

    virtual bool onRender(RenderStage currentStage) { return true; }
    virtual void update(const U64 deltaTime) {
        _deltaTime = deltaTime;
        _elapsedTime += deltaTime;
    }

    virtual void resetTimers() {
        _deltaTime = 0UL;
        _elapsedTime = 0UL;
    }

    virtual void setActive(const bool state) {
        _parentNodeActive = state;
    }

    inline ComponentType getType() const { return _type; }
    inline SceneGraphNode& getSGN() const { return _parentSGN; }
    
   protected:
    std::atomic_bool _parentNodeActive;
    /// Pointer to the SGN owning this instance of AnimationComponent
    SceneGraphNode& _parentSGN;
    ComponentType _type;
    U64 _elapsedTime;
    U64 _deltaTime;
};

};  // namespace Divide
#endif
