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

#ifndef _RENDER_INSTANCE_H_
#define _RENDER_INSTANCE_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <boost/noncopyable.hpp>

class Object3D;
class Transform;
#define NULL 0
///Wraps geometry with transform and additional rendering params. Pass to the rendering API for presenting to screen
class RenderInstance : private boost::noncopyable{
public:
	RenderInstance(Object3D* const geometry) : _geometry(geometry),
											   _transform(NULL),
											   _preDraw(false),
											   _draw2D(false)
	{
	}

	~RenderInstance()
	{
		_geometry = NULL;
	}

	///Model transform manipulation
	Transform* transform()                           const {return _transform;}
	void       transform(Transform* const transform)       {_transform = transform;}
	///Geometry manipulation
	Object3D* const object3D()                         const {return _geometry;}
	void            object3D(Object3D* const geometry)       {_geometry = geometry;}
	///PreDraw checks
	bool preDraw()                   const {return _preDraw;}
	void preDraw(const bool preDraw)       {_preDraw = preDraw;}
	///2D drawing
	bool draw2D()                  const {return _draw2D;}
	void draw2D(const bool draw2D)       {_draw2D = draw2D;}     

private:
	///The actual geometry wrapper
	Object3D*  _geometry;
	///The geometry's transformation information
	Transform* _transform;
	///Perform a preDraw operation on the model
	bool       _preDraw;
	///Use 2D drawing
	bool       _draw2D;
};

#endif