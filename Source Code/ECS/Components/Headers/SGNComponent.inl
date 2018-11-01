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
    template<typename T>
    SGNComponent<T>::SGNComponent(SceneGraphNode& parentSGN, ComponentType type)
        : ECS::Event::IEventListener(&parentSGN.GetECSEngine()),
         _type(type),
         _parentSGN(parentSGN),
         _editorComponent(type._to_string()),
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