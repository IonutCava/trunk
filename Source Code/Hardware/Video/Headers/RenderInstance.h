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

#include <boost/noncopyable.hpp>
#include "Geometry/Shapes/Headers/Object3D.h"

///Wraps geometry with transform and additional rendering params. Pass to the rendering API for presenting to screen
class RenderInstance : private boost::noncopyable{
public:
    RenderInstance(Object3D* const geometry) : _model(geometry),
                                               _buffer(nullptr),
                                               _transform(nullptr),
                                               _prevTransform(nullptr),
                                               _preDraw(false),
                                               _stateHash(0)
    {
    }

    ~RenderInstance()
    {
        _model = nullptr;
        _buffer = nullptr;
    }

    ///Model transform manipulation
    Transform* transform()                               const {return _transform;}
    void       transform(Transform* const transform)           {_transform = transform;}
    Transform* prevTransform()                           const {return _prevTransform;}
    void       prevTransform(Transform* const transform)       {_prevTransform = transform;}
    ///Geometry manipulation
    Object3D* const object3D()                           const { return _model; }
    void            object3D(Object3D* const geometry)         { _model = geometry; }
    ///Buffer Data
    inline VertexBuffer* const buffer()                           const { return _buffer; }
    inline void                buffer(VertexBuffer* const buffer)       { _buffer = buffer; }
    inline void                deleteBuffer()                           { SAFE_DELETE(_buffer); }
    ///PreDraw checks
    bool preDraw()                   const {return _preDraw;}
    void preDraw(const bool preDraw)       {_preDraw = preDraw;}
    ///Draw Command
    inline void  addDrawCommand(const GenericDrawCommand& drawCommand)      { _drawCommands.push_back(drawCommand);}
    inline const vectorImpl<GenericDrawCommand >& drawCommands()      const { return _drawCommands; }
    inline void  clearDrawCommands()                                        { _drawCommands.clear(); }
    ///State management
    inline I64  stateHash()              const { return _stateHash; }
    inline void stateHash(I64 hashValue)       { _stateHash = hashValue; }


private:
    ///The actual geometry wrapper
    Object3D*  _model;
    ///A custom, override vertex buffer
    VertexBuffer* _buffer;
    ///The geometry's transformation information
    Transform* _transform;
    ///The geometry's previous transformation information. Used for interpolation purposes
    Transform* _prevTransform;
    ///Perform a preDraw operation on the model
    bool       _preDraw;
    ///The draw command associated with this render instance
    vectorImpl<GenericDrawCommand > _drawCommands;
    ///The state hash associated with this render instance
    I64       _stateHash;
};

#endif