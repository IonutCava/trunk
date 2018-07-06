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
#ifndef _DIRECTIONAL_LIGHT_H_
#define _DIRECTIONAL_LIGHT_H_

#include "Light.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

struct frustum;
class DirectionalLight : public Light {
public:
	DirectionalLight(U8 slot);
	~DirectionalLight();
	void setCameraToLightView(const vec3<F32>& eyePos);
	void renderFromLightView(const U8 depthPass,const F32 sceneHalfExtent = 1);

private:
	void updateFrustumPoints(frustum &f, vec3<F32> &center, vec3<F32> &viewDir);
	void updateSplitDist(frustum f[3], F32 nearPlane, F32 farPlane);

private:
	F32 _splitWeight;
	I8  _splitCount;
	D32 _lightOrtho[3];
};

#endif