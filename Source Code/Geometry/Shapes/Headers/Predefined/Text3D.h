/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
/// For now, the name of the Text3D object is the text itself

namespace Divide {

class Text3D : public Object3D {
   public:
    explicit Text3D(const stringImpl& name, const stringImpl& font)
        : Object3D(name, ObjectType::TEXT_3D, ObjectFlag::OBJECT_FLAG_NONE),
          _font(font),
          _height(16),
          _width(1.0f)
    {
        /// Dummy
        getGeometryVB()->queueRefresh();
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline const stringImpl& getText() { return _text; }
    inline const stringImpl& getFont() { return _font; }
    inline F32 getWidth()  { return _width; }
    inline U32 getHeight() { return _height; }

    inline void setText(const stringImpl& text) { 
        _text = text;
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void setWidth(F32 width) {
        _width = width;
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void updateBoundsInternal(SceneGraphNode& sgn) override {
        vec3<F32> min(-_width * 2, 0, -_width * 0.5f);
        vec3<F32> max(_width * 1.5f * _text.length() * 10, _width * _text.length() * 1.5f, _width * 0.5f);
        _boundingBox.set(min, max);
        Object3D::updateBoundsInternal(sgn);
    }

   private:
    stringImpl _text;
    stringImpl _font;
    F32 _width;
    U32 _height;
};

};  // namespace Divide

#endif