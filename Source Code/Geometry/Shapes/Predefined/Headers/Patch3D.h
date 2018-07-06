/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _PATCH_3D_H_
#define _PATCH_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"

namespace Divide {

// Class used only for tessellated  geometry
class Patch3D : public Object3D {
public:
    explicit Patch3D(GFXDevice& context,
                     ResourceCache& parentCache,
                     size_t descriptorHash,
                     const stringImpl& name,
                     vec2<U16> dim);

    void build(const DELEGATE_CBK<void, const vec2<U16>&, VertexBuffer*>& buildFunction);

    void build();

    void updateBoundsInternal() override;

protected:
    vec2<U16> _dimensions;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Patch3D);

};  // namespace Divide

#endif //_PATCH_3D_H_
