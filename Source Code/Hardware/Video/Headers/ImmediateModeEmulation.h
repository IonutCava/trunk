/*
   Copyright (c) 2013 DIVIDE-Studio
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

#ifndef _IM_EMULATION_H_
#define _IM_EMULATION_H_

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include "Core/Math/Headers/MathClasses.h"

class Texture;
typedef Texture Texture2D;

enum PrimitiveType;
///IMPrimitive replaces immediate mode calls to VBO based rendering
class IMPrimitive  : private boost::noncopyable{
public:
    inline void setRenderStates(boost::function0<void> setupStatesCallback, boost::function0<void> releaseStatesCallback){
        _setupStates = setupStatesCallback;
        _resetStates = releaseStatesCallback;
    }

    virtual void beginBatch() = 0;
    virtual void begin(PrimitiveType type) = 0;
    virtual void vertex(const vec3<F32>& vert) = 0;
    virtual void attribute4ub(const std::string& attribName, const vec4<U8>& value) = 0;
    virtual void attribute4f(const std::string& attribName, const vec4<F32>& value) = 0;
    virtual void end() = 0;
    virtual void endBatch() = 0;
    virtual void clear() = 0;

    inline void inUse(bool state)                  {_inUse = state;}
    inline bool inUse()                      const {return _inUse;}
    inline void zombieCounter(U8 count)            {_zombieCounter = count;}
    inline U8   zombieCounter()              const {return _zombieCounter;}
    inline void forceWireframe(bool state)         {_forceWireframe = state;}
    inline bool forceWireframe()             const {return _forceWireframe; }
    inline bool hasRenderStates()            const {return (!_setupStates.empty() && !_resetStates.empty());}

protected:
    IMPrimitive();
    virtual ~IMPrimitive();

public:

    Texture2D*           _texture;
    bool                 _hasLines;
    bool                 _canZombify;
    F32                  _lineWidth;

protected:
    ///after rendering an IMPrimitive, it's "inUse" flag is set to false.
    ///If OpenGL tries to render it again, we just increment the _zombieCounter
    ///If the _zombieCounter reaches 3, we remove it from the vector as it is not needed
    U8                   _zombieCounter;
    ///After rendering the primitive, we se "inUse" to false but leave it in the vector
    ///If a new primitive is to be rendered, it first looks for a zombie primitive
    ///If none are found, it adds a new one
    bool                 _inUse; //<For caching
    //render in wireframe mode
    bool                 _forceWireframe;
    ///2 functions used to setup or reset states
    boost::function0<void> _setupStates;
    boost::function0<void> _resetStates;
};

#endif