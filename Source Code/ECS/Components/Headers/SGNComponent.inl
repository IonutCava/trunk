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

#ifndef _SGN_COMPONENT_INL_
#define _SGN_COMPONENT_INL_

#include "ECS/Events/Headers/EntityEvents.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    inline const char* getComponentTypeName(ComponentType type) {
        switch (type) {
            case ComponentType::ANIMATION: return "ANIMATION";
            case ComponentType::INVERSE_KINEMATICS: return "INVERSE_KINEMATICS";
            case ComponentType::RAGDOLL: return "RAGDOLL";
            case ComponentType::NAVIGATION: return "NAVIGATION";
            case ComponentType::TRANSFORM: return "TRANSFORM";
            case ComponentType::BOUNDS: return "BOUNDS";
            case ComponentType::RENDERING: return "RENDERING";
            case ComponentType::NETWORKING: return "NETWORKING";
            case ComponentType::UNIT: return "UNIT";
            case ComponentType::RIGID_BODY: return "RIGID_BODY";
            case ComponentType::SELECTION: return "SELECTION";
        };

        return "";
    }

    inline ComponentType getComponentTypeByName(const char* name) {
        if (Util::CompareIgnoreCase(name, "ANIMATION")) {
            return ComponentType::ANIMATION;
        } else if (Util::CompareIgnoreCase(name, "INVERSE_KINEMATICS")) {
            return ComponentType::INVERSE_KINEMATICS;
        } else if (Util::CompareIgnoreCase(name, "RAGDOLL")) {
            return ComponentType::RAGDOLL;
        } else if (Util::CompareIgnoreCase(name, "NAVIGATION")) {
            return ComponentType::NAVIGATION;
        } else if (Util::CompareIgnoreCase(name, "TRANSFORM")) {
            return ComponentType::TRANSFORM;
        } else if (Util::CompareIgnoreCase(name, "BOUNDS")) {
            return ComponentType::BOUNDS;
        } else if (Util::CompareIgnoreCase(name, "RENDERING")) {
            return ComponentType::RENDERING;
        } else if (Util::CompareIgnoreCase(name, "NETWORKING")) {
            return ComponentType::NETWORKING;
        } else if (Util::CompareIgnoreCase(name, "UNIT")) {
            return ComponentType::UNIT;
        } else if(Util::CompareIgnoreCase(name, "RIGID_BODY")) {
            return ComponentType::RIGID_BODY;
        } else if (Util::CompareIgnoreCase(name, "SELECTION")) {
            return ComponentType::SELECTION;
        }

        return ComponentType::COUNT;
    }

    template<typename T>
    SGNComponent<T>::SGNComponent(SceneGraphNode& parentSGN, ComponentType type)
        : ECS::Event::IEventListener(&parentSGN.GetECSEngine()),
        _parentSGN(parentSGN),
        _editorComponent(getComponentTypeName(type)),
        _enabled(true),
        _hasChanged(false)
    {
        RegisterEventCallbacks();
    }

    template<typename T>
    SGNComponent<T>::~SGNComponent()
    {
        UnregisterAllEventCallbacks();
    }

    template<typename T>
    void SGNComponent<T>::RegisterEventCallbacks() {
    }

    template<typename T>
    bool SGNComponent<T>::save(ByteBuffer& outputBuffer) const {
        ACKNOWLEDGE_UNUSED(outputBuffer);
        return true;
    }

    template<typename T>
    bool SGNComponent<T>::load(ByteBuffer& inputBuffer) {
        ACKNOWLEDGE_UNUSED(inputBuffer);
        return true;
    }

    template<typename T>
    I64 SGNComponent<T>::uniqueID() const {
        return _ID((_parentSGN.name() + "_" + _editorComponent.name().c_str()).c_str());
    }

    template<typename T>
    void SGNComponent<T>::PreUpdate(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    template<typename T>
    bool SGNComponent<T>::enabled() const {
        return _enabled;
    }

    template<typename T>
    void SGNComponent<T>::enabled(const bool state) {
        _enabled = state;
    }

    template<typename T>
    void SGNComponent<T>::Update(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    template<typename T>
    void SGNComponent<T>::PostUpdate(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    template<typename T>
    void SGNComponent<T>::OnUpdateLoop() {

    }
};//namespace Divide

#endif