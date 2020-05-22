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

namespace Divide {

/// A generic component for the SceneGraphNode class
enum class RenderStage : U8;
class SceneGraphNode;
class SceneRenderState;
struct RenderStagePass;

}; //namespace Divide 

namespace ECS {
    struct CustomEvent {
        enum class Type : Divide::U8 {
            TransformUpdated = 0,
            RelationshipCacheInvalidated,
            BoundsUpdated,
            EntityPostLoad,
            EntityFlagChanged,
            COUNT
        };

        Type _type = Type::COUNT;
        std::any _userData;
        Divide::U32 _flag = 0u;
    };
};

namespace Divide {

//ref: http://www.nirfriedman.com/2018/04/29/unforgettable-factory/
template <typename Base, typename... Args>
struct Factory {
    using ConstructFunc = DELEGATE<void, SceneGraphNode&, Args...>;
    using DestructFunc = DELEGATE<void, SceneGraphNode&>;
    using FactoryContainerConstruct = ska::bytell_hash_map<ComponentType::_integral, ConstructFunc>;
    using FactoryContainerDestruct = ska::bytell_hash_map<ComponentType::_integral, DestructFunc>;

    template <typename... ConstructArgs>
    static void construct(ComponentType type, SceneGraphNode& node, ConstructArgs&&... args) {
        constructData().at(type)(node, std::forward<ConstructArgs>(args)...);
    }

    static void destruct(ComponentType type, SceneGraphNode& node) {
        destructData().at(type)(node);
    }

    template <typename T, ComponentType::_enumerated C>
    struct Registrar : public ECS::Component<T>,
                       public Base
    {
        template<typename... InnerArgs>
        Registrar(InnerArgs&&... args)
            : Base(Key{ s_registered }, C, std::forward<InnerArgs>(args)...)
        {
            ACKNOWLEDGE_UNUSED(s_registered);
        }

        void OnData(const ECS::CustomEvent& data) override {
            ACKNOWLEDGE_UNUSED(data);
        }

        static bool RegisterComponentType() {
            Factory::constructData().emplace(C, [](SceneGraphNode& node, Args... args) -> void {
                node.AddSGNComponent<T>(std::forward<Args>(args)...);
            });

            Factory::destructData().emplace(C, [](SceneGraphNode& node) -> void {
                node.RemoveSGNComponent<T>();
            });

            return true;
        }

        static bool s_registered;

        friend T;
    };

    friend Base;

private:
    struct Key {
        Key(bool registered) : _registered(registered) {}

      private:
        bool _registered = false;

        template <typename T, ComponentType::_enumerated C>
        friend struct Registrar;
    };

    Factory() = default;

    static FactoryContainerConstruct& constructData() {
        static FactoryContainerConstruct container;
        return container;
    }
    static FactoryContainerDestruct& destructData() {
        static FactoryContainerDestruct container;
        return container;
    }
};

template <typename Base, typename... Args>
template <typename T, ComponentType::_enumerated C>
bool Factory<Base, Args...>::Registrar<T, C>::s_registered = Factory<Base, Args...>::template Registrar<T, C>::RegisterComponentType();

struct EntityOnUpdate;

class SGNComponent : protected PlatformContextComponent,
                     public Factory<SGNComponent>
{
    public:
        explicit SGNComponent(Key key, ComponentType type, SceneGraphNode& parentSGN, PlatformContext& context);
        virtual ~SGNComponent();

        virtual void PreUpdate(const U64 deltaTime);
        virtual void Update(const U64 deltaTime);
        virtual void PostUpdate(const U64 deltaTime);

        virtual void OnData(const ECS::CustomEvent& data);

        inline SceneGraphNode& getSGN() const noexcept { return _parentSGN; }
        inline ComponentType type() const noexcept { return _type; }

        EditorComponent& getEditorComponent() noexcept { return _editorComponent; }
        const EditorComponent& getEditorComponent() const noexcept { return _editorComponent; }

        virtual bool saveCache(ByteBuffer& outputBuffer) const;
        virtual bool loadCache(ByteBuffer& inputBuffer);

        U64 uniqueID() const;

        virtual bool enabled() const;
        virtual void enabled(const bool state);

    protected:
        EditorComponent _editorComponent;
        SceneGraphNode& _parentSGN;
        ComponentType _type = ComponentType::COUNT;
        std::atomic_bool _enabled;
        mutable std::atomic_bool _hasChanged;
};

template<typename T, ComponentType::_enumerated C>
using BaseComponentType = SGNComponent::Registrar<T, C>;

#define INIT_COMPONENT(X) static bool X##_registered = X::s_registered
};  // namespace Divide
#endif //_SGN_COMPONENT_H_

#include "SGNComponent.inl"
