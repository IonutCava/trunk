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
#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
/// For now, the name of the Text3D object is the text itself

namespace Divide {

class Text3D : public Object3D {
  public:
    explicit Text3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& font);

    inline const stringImpl& getText() { return _text; }
    inline const stringImpl& getFont() { return _font; }
    inline F32 getWidth()  { return _width; }
    inline U32 getHeight() { return _height; }

    void setText(const stringImpl& text);

    void setWidth(F32 width);

    void updateBoundsInternal() override;

  private:
    stringImpl _text;
    stringImpl _font;
    F32 _width;
    U32 _height;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Text3D);

};  // namespace Divide

#endif