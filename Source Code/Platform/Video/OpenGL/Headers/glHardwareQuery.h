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
#ifndef _GL_HARDWARE_QUERY_H_
#define _GL_HARDWARE_QUERY_H_

#include "glResources.h"

#include "Core/Headers/RingBuffer.h"
#include "Core/TemplateLibraries/Headers/Vector.h"

namespace Divide {

class glHardwareQuery : public glObject {
public:
    explicit glHardwareQuery(GFXDevice& context);
    ~glHardwareQuery();

    void create();
    void destroy();

    inline U32 getID() const { return _queryID; }
    inline bool enabled() const { return _enabled; }
    inline void enabled(bool state) { _enabled = state; }

protected:
    bool _enabled;
    U32 _queryID;
};

class glHardwareQueryRing : public RingBuffer {

public:
    glHardwareQueryRing(GFXDevice& context, U32 queueLength, U32 id = 0);
    ~glHardwareQueryRing();

    glHardwareQuery& readQuery();
    glHardwareQuery& writeQuery();

    void initQueries();

    void resize(U32 queueLength) override;

    inline U32 id() const { return _id; }

protected:
    U32 _id;
    bool _needRefresh;
    GFXDevice& _context;
    vectorImpl<std::shared_ptr<glHardwareQuery>> _queries;
};

};
#endif //_GL_HARDWARE_QUERY_H_