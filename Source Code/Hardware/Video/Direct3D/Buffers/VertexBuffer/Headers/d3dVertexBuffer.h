/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _D3D_VERTEX_BUFFER_OBJECT_H
#define _D3D_VERTEX_BUFFER_OBJECT_H

#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class d3dVertexBuffer : public VertexBuffer {
public:
    bool Create(bool staticDraw = true) {return true;}

    void Destroy() {}

    bool SetActive() {return true;}
    void Draw(const GenericDrawCommand& command, bool skipBind = false) {};
    void Draw(const vectorImpl<GenericDrawCommand>& commands, bool skipBind = false) {};

    bool queueRefresh() {return Refresh();}

    d3dVertexBuffer() : VertexBuffer() {}
    ~d3dVertexBuffer() {Destroy();}

private:
    bool CreateInternal() {return true;}
    bool Refresh() {return true;}
    void checkStatus() {}
};

}; //namespace Divide


#endif
