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
#ifndef _ENTITY_EVENTS_H_
#define _ENTITY_EVENTS_H_

namespace Divide {

    struct EntityPostLoad : public ECS::Event::Event<EntityPostLoad>
    {
        ECS::EntityId ownerID;

        EntityPostLoad(ECS::ECSEngine* engine, ECS::EntityId id) : Event(engine), ownerID(id)
        {}
    };

    struct EntityActiveStateChange : public ECS::Event::Event<EntityActiveStateChange>
    {
        ECS::EntityId ownerID;
        bool _state = false;

        EntityActiveStateChange(ECS::ECSEngine* engine, ECS::EntityId id, bool state) : Event(engine), ownerID(id), _state(state)
        {}
    };

}; //namespace Divide

#endif //_ENTITY_EVENTS_H_