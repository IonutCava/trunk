/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
    IMPrimitive();
    virtual ~IMPrimitive();

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