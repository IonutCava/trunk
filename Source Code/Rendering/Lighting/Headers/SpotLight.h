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
#ifndef _SPOT_LIGHT_H_
#define _SPOT_LIGHT_H_

#include "Light.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

class SpotLight : public Light{
public:
	SpotLight(U8 slot, F32 range = 2);

	void setCameraToLightView(const vec3<F32>& eyePos);
	void renderFromLightView(const U8 depthPass,const F32 sceneHalfExtent = 1);
};

#endif