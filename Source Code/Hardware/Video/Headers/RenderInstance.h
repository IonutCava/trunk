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

#ifndef _RENDER_INSTANCE_H_
#define _RENDER_INSTANCE_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <boost/noncopyable.hpp>

class Object3D;
class Transform;

///Wraps geometry with transform and additional rendering params. Pass to the rendering API for presenting to screen
class RenderInstance : private boost::noncopyable{
public:
    RenderInstance(Object3D* const geometry) : _geometry(geometry),
                                               _transform(nullptr),
                                               _prevTransform(nullptr),
                                               _preDraw(false)
    {
    }

    ~RenderInstance()
    {
        _geometry = nullptr;
    }

    ///Model transform manipulation
    Transform* transform()                               const {return _transform;}
    void       transform(Transform* const transform)           {_transform = transform;}
    Transform* prevTransform()                           const {return _prevTransform;}
    void       prevTransform(Transform* const transform)       {_prevTransform = transform;}
    ///Geometry manipulation
    Object3D* const object3D()                         const {return _geometry;}
    void            object3D(Object3D* const geometry)       {_geometry = geometry;}
    ///PreDraw checks
    bool preDraw()                   const {return _preDraw;}
    void preDraw(const bool preDraw)       {_preDraw = preDraw;}

private:
    ///The actual geometry wrapper
    Object3D*  _geometry;
    ///The geometry's transformation information
    Transform* _transform;
    ///The geoemtry's previous transformation information. Used for interpolation purposes
    Transform* _prevTransform;
    ///Perform a preDraw operation on the model
    bool       _preDraw;
};

#endif