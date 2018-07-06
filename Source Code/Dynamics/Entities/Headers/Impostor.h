/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _IMPOSTOR_H_
#define _IMPOSTOR_H_

#include "core.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

class SceneGraphNode;

/// Renders a sphere at the parent node's position using the desired radius;
class Impostor{
public:
	Impostor(const std::string& name, F32 radius = 1.0f);
	~Impostor();

	/// Render the impostor using SceneGraphData
	void render(SceneGraphNode* const node);
	/// Render the impostor using target transform
	void render(Transform* const transform);

	inline void setRadius(F32 radius) {_dummy->setRadius(radius);}
	inline Sphere3D* const getDummy() {return _dummy;}

private:
	bool      _visible;
	Sphere3D* _dummy;
	RenderStateBlock* _dummyStateBlock;
};

#endif

