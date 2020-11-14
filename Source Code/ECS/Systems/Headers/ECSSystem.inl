/*
Copyright (c) 2018 DIVIDE-Studio
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
#ifndef _ECS_SYSTEM_INL_
#define _ECS_SYSTEM_INL_
namespace Divide {
    template<class T, class U>
    ECSSystem<T, U>::ECSSystem(ECS::ECSEngine& engine)
        : _engine(engine)
        , _compManager(engine.GetComponentManager())
        , _container(_compManager->GetComponentContainer<U>())
    {
        _componentCache.reserve(Config::MAX_VISIBLE_NODES);
    }

    template<class T, class U>
    bool ECSSystem<T, U>::saveCache(const SceneGraphNode* sgn, ByteBuffer& outputBuffer) {
        ACKNOWLEDGE_UNUSED(sgn);
        ACKNOWLEDGE_UNUSED(outputBuffer);
        return true;
    }

    template<class T, class U>
    bool ECSSystem<T, U>::loadCache(SceneGraphNode* sgn, ByteBuffer& inputBuffer) {
        ACKNOWLEDGE_UNUSED(sgn);
        ACKNOWLEDGE_UNUSED(inputBuffer);
        return true;
    }

    template<class T, class U>
    void ECSSystem<T, U>::PreUpdate(F32 dt) {
        OPTICK_EVENT();

        U64 microSec = Time::MillisecondsToMicroseconds(dt);
        const size_t compCount = _container->size();

        // Keep memory in order to avoid mid-frame allocs
        if (_componentCache.size() < compCount) {
            _componentCache.resize(compCount);
        }

        auto iterBegin = _container->begin();
        auto iterEnd = _container->end();
        for (size_t idx = 0; iterBegin != iterEnd; ++iterBegin, ++idx) {
            _componentCache[idx] = &*iterBegin;
        }

        for (U* comp : _componentCache) {
            comp->PreUpdate(microSec);
        }
    }

    template<class T, class U>
    void ECSSystem<T, U>::Update(F32 dt) {
        OPTICK_EVENT();

        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (U* comp : _componentCache) {
            comp->Update(microSec);
        }
    }

    template<class T, class U>
    void ECSSystem<T, U>::PostUpdate(F32 dt) {
        OPTICK_EVENT();

        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        for (U* comp : _componentCache) {
            comp->PostUpdate(microSec);
        }
    }
}
#endif //_ECS_SYSTEM_INL_