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
#ifndef _BOX_3D_H_
#define _BOX_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"

namespace Divide {

class Box3D : public Object3D {
   public:
   explicit Box3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const vec3<F32>& size);

   void setHalfExtent(const vec3<F32>& halfExtent);

   const vec3<F32>& getHalfExtent() const;

   void fromPoints(const std::initializer_list<vec3<F32>>& points,
                   const vec3<F32>& halfExtent);

   void updateBoundsInternal() override;

   void saveToXML(boost::property_tree::ptree& pt) const override;
   void loadFromXML(const boost::property_tree::ptree& pt)  override;

   private:
      vec3<F32> _halfExtent;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Box3D);

};  // namespace Divide

#endif