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
#include "Core/Headers/PlatformContextComponent.h"

#include <ECS.h>
#include <BetterEnums/include/enum.h>

namespace Divide {

/// A generic component for the SceneGraphNode class
enum class RenderStage : U8;
class SceneGraphNode;
class SceneRenderState;
struct RenderStagePass;

BETTER_ENUM(ComponentType, U16,
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
    DIRECTIONAL_LIGHT = toBit(12),
    POINT_LIGHT = toBit(13),
    SPOT_LIGHT = toBit(14),
    COUNT = 15
);

//ref: http://www.nirfriedman.com/2018/04/29/unforgettable-factory/
template <class Base, class... Args>
class Factory {
public:
    template <class ... T>
    static void make(ComponentType type, SceneGraphNode& node, T&&... args) {
        data().at(type)(node, std::forward<T>(args)...);
    }

    template <class T, ComponentType::_enumerated C>
    struct Registrar : public ECS::Component<T>,
                       Base
                       
    {
        friend T;

        static bool registerComponentType() {
            Factory::data()[C] = [](SceneGraphNode& node, Args... args) -> void {
                 AddSGNComponent<T>(node, std::forward<Args>(args)...);
            };
            return true;
        }

        static bool s_registered;

        template<class ...P>
        Registrar(P&&... param) : Base(Key{s_registered}, C, std::forward<P>(param)...) { 
            (void)s_registered;
        }
    };

    friend Base;

private:
    class Key {
        Key(bool registered)
            : _registered(registered)
        {
        };

    private:
        bool _registered = false;
        template <class T, ComponentType::_enumerated C> friend struct Registrar;
    };

    Factory() = default;

    static auto &data() {
        static ska::bytell_hash_map<ComponentType::_integral, DELEGATE_CBK<void, SceneGraphNode&, Args...>> s;
        return s;
    }
};

template <class Base, class... Args>
template <class T, ComponentType::_enumerated C>
bool Factory<Base, Args...>::Registrar<T, C>::s_registered = Factory<Base, Args...>::template Registrar<T, C>::registerComponentType();

struct EntityOnUpdate;
struct EntityActiveStateChange;

class SGNComponent : private PlatformContextComponent,
                     public Factory<SGNComponent>,
                     public ECS::Event::IEventListener
{
   public:

    explicit SGNComponent(Key key, ComponentType type, SceneGraphNode& parentSGN, PlatformContext& context);
    virtual ~SGNComponent();

    virtual void PreUpdate(const U64 deltaTime);
    virtual void Update(const U64 deltaTime);
    virtual void PostUpdate(const U64 deltaTime);
    virtual void FrameEnded();

    virtual void OnUpdateLoop();

    inline SceneGraphNode& getSGN() const { return _parentSGN; }
    inline ComponentType type() const { return _type; }

    EditorComponent& getEditorComponent() { return _editorComponent; }
    const EditorComponent& getEditorComponent() const { return _editorComponent; }

    virtual bool saveCache(ByteBuffer& outputBuffer) const;
    virtual bool loadCache(ByteBuffer& inputBuffer);

    I64 uniqueID() const;

    virtual bool enabled() const;
    virtual void enabled(const bool state);

   protected:
    void RegisterEventCallbacks();

   protected:
    EditorComponent _editorComponent;
    /// Pointer to the SGN owning this instance of AnimationComponent
    SceneGraphNode& _parentSGN;
    ComponentType _type;
    std::atomic_bool _enabled;
    mutable std::atomic_bool _hasChanged;

};

template<typename T, ComponentType::_enumerated C>
using BaseComponentType = SGNComponent::Registrar<T, C>;

#define INIT_COMPONENT(X) static bool X##_registered = X::s_registered
};  // namespace Divide
#endif //_SGN_COMPONENT_H_

#include "SGNComponent.inl"
