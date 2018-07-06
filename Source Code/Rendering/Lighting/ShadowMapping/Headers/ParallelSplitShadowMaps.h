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
    void render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction);
    void postRender();
    ///Update depth maps
    void resolution(U16 resolution, const SceneRenderState& renderState);
    void previewShadowMaps();

protected:
    void renderInternal(const SceneRenderState& renderState) const;
    //OGRE! I know .... sorry -Ionut
    void calculateSplitPoints(U8 splitCount, F32 nearDist, F32 farDist, F32 lambda = 0.95);
    void setOptimalAdjustFactor(U8 index, F32 value);

protected:
    U8  _numSplits;
    U8  _splitPadding; //<Avoid artifacts;
    Quad3D* _renderQuad;
    ShaderProgram*  _previewDepthMapShader;
    ShaderProgram*  _blurDepthMapShader;
    vectorImpl<F32> _splitPoints;
    vectorImpl<F32> _optAdjustFactor;
    vectorImpl<F32> _orthoPerPass;
    ///The blur buffer
    FrameBufferObject*  _blurBuffer;
};
#endif 