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
#ifndef _POINT_LIGHT_H_
#define _POINT_LIGHT_H_

#include "Light.h"

class PointLight : public Light{
public:
	PointLight(U8 slot,F32 range = 2);

	///These 2 functions are not needed as we generate a cubemap based only on position.
	void setCameraToLightView(const vec3<F32>& eyePos) {}
	void renderFromLightView(const U8 depthPass,const F32 sceneHalfExtent = 1) {}
};

#endif