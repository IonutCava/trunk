/* Copyright (c) 2014 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _SGN_COMPONENT_H_
#define _SGN_COMPONENT_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <boost/noncopyable.hpp>

/// A generic component for the SceneGraphNode class
enum RenderStage;
class SceneGraphNode;
class SGNComponent : private boost::noncopyable {
public:
    enum ComponentType {
        SGN_COMP_ANIMATION = 0,
        SGN_COMP_NAVIGATION = 1,
        SGN_COMP_PHYSICS = 2
    };

    SGNComponent(ComponentType type, SceneGraphNode* const parentSGN);
    ~SGNComponent();

    virtual void onDraw(RenderStage currentStage) {}
    virtual void update(const U64 deltaTime) {
        _deltaTime = deltaTime;
        _elapsedTime += deltaTime;
    }

    virtual void reset() {
        _deltaTime = 0UL;
        _elapsedTime = 0UL;
    }

    inline ComponentType         getType() const { return _type; }
    inline SceneGraphNode* const getSGN()  const { return _parentSGN; }

protected:
    /// The current instance using this component
    U32 _instanceID;
    /// Pointer to the SGN owning this instance of AnimationComponent
    SceneGraphNode* _parentSGN;
    ComponentType _type;
    U64 _elapsedTime;
    U64 _deltaTime;
};

#endif