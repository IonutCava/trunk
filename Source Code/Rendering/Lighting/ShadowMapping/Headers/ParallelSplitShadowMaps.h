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
#ifndef _PS_SM_H_
#define _PS_SM_H_

#include "ShadowMap.h"
class Quad3D;
class ShaderProgram;
class PixelBufferObject;
///Directional lights can't deliver good quality shadows using a single shadow map. This tehnique offers an implementation of the PSSM method
class PSShadowMaps : public ShadowMap {
public:
	PSShadowMaps(Light* light);
	~PSShadowMaps();
	void render(SceneRenderState* sceneRenderState, boost::function0<void> sceneRenderFunction);
	///Get the current shadow mapping tehnique
	ShadowType getShadowMapType() const {return SHADOW_TYPE_PSSM;}
	///Update depth maps
	void resolution(U16 resolution,SceneRenderState* sceneRenderState);
	void previewShadowMaps();
protected:
	void renderInternal(SceneRenderState* renderState) const;
	void createJitterTexture(I32 size, I32 samples_u, I32 samples_v);
    //OGRE! I know .... sorry -Ionut
    void calculateSplitPoints(U8 splitCount, F32 nearDist, F32 farDist, F32 lambda = 0.95);
    //
    void setOptimalAdjustFactor(U8 index, F32 value);

protected:
	U8  _numSplits;
    U8  _splitPadding; //<Avoid artifacts;
	Quad3D* _renderQuad;
	ShaderProgram*  _previewDepthMapShader;
    vectorImpl<F32> _splitPoints;
    vectorImpl<F32> _optAdjustFactor;
    vectorImpl<F32> _orthoPerPass;
	PixelBufferObject* _jitterTexture; ///<For blurring
};
#endif 