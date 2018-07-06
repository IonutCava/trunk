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

#ifndef _TRANSFORM_EVENTS_H_
#define _TRANSFORM_EVENTS_H_

#include <ECS.h>

namespace Divide {

    struct TransformDirty: public ECS::Event::Event<TransformDirty>
    {
        ECS::EntityId ownerID;
        U32 typeMask;

        TransformDirty(ECS::ECSEngine* engine, ECS::EntityId id, U32 transformTypeMask) : Event(engine), ownerID(id), typeMask(transformTypeMask)
        {}
    };

    struct ParentTransformDirty : public ECS::Event::Event<ParentTransformDirty>
    {
        ECS::EntityId ownerID;
        U32 typeMask;

        ParentTransformDirty(ECS::ECSEngine* engine, ECS::EntityId id, U32 transformTypeMask) : Event(engine), ownerID(id), typeMask(transformTypeMask)
        {}
    };
};

#endif //_TRANSFORM_EVENTS_H_