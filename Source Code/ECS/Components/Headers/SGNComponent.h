/* Copyright (c) 2018 DIVIDE-Studio
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
#ifndef _SGN_COMPONENT_H_
#define _SGN_COMPONENT_H_

#include "EditorComponent.h"

#include <ECS.h>

namespace Divide {

/// A generic component for the SceneGraphNode class
enum class RenderStage : U8;
class SceneGraphNode;
class SceneRenderState;
struct RenderStagePass;

enum class ComponentType : U32 {
    ANIMATION = toBit(1),
    INVERSE_KINEMATICS = toBit(2),
    RAGDOLL = toBit(3),
    NAVIGATION = toBit(4),
    TRANSFORM = toBit(5),
    BOUNDS = toBit(6),
    RENDERING = toBit(7),
    NETWORKING = toBit(8),
    UNIT = toBit(9),
    RIGID_BODY = toBit(10),
    SELECTION = toBit(11),
    COUNT = 12
};

inline const char* getComponentTypeName(ComponentType type);
inline ComponentType getComponentTypeByName(const char* name);

struct EntityOnUpdate;
struct EntityActiveStateChange;

template <typename T>
class SGNComponent : private NonCopyable,
                     public ECS::Component<T>,
                     protected ECS::Event::IEventListener
{
   public:

    SGNComponent(SceneGraphNode& parentSGN, ComponentType type);
    virtual ~SGNComponent();

    virtual void PreUpdate(const U64 deltaTime);
    virtual void Update(const U64 deltaTime);
    virtual void PostUpdate(const U64 deltaTime);
    virtual void OnUpdateLoop();

    inline SceneGraphNode& getSGN() const { return _parentSGN; }
    
    EditorComponent& getEditorComponent() { return _editorComponent; }
    const EditorComponent& getEditorComponent() const { return _editorComponent; }

    virtual bool save(ByteBuffer& outputBuffer) const;
    virtual bool load(ByteBuffer& inputBuffer);

    I64 uniqueID() const;

    virtual bool enabled() const;
    virtual void enabled(const bool state);
    
   protected:
    void RegisterEventCallbacks();

   protected:
    std::atomic_bool _enabled;
    mutable std::atomic_bool _hasChanged;
    /// Pointer to the SGN owning this instance of AnimationComponent
    SceneGraphNode& _parentSGN;
    EditorComponent _editorComponent;
};

};  // namespace Divide
#endif //_SGN_COMPONENT_H_

#include "SGNComponent.inl"
