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

#ifndef _C_SM_H_
#define _C_SM_H_

#include "ShadowMap.h"
class Quad3D;
class Camera;
class IMPrimitive;
class ShaderProgram;
class SceneGraphNode;
///Directional lights can't deliver good quality shadows using a single shadow map. This tehnique offers an implementation of the CSM method
class CascadedShadowMaps : public ShadowMap {
public:
    CascadedShadowMaps(Light* light, F32 numSplits, F32 splitLogFactor);
    ~CascadedShadowMaps();
    void render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction);
    void postRender();
    ///Update depth maps
    void resolution(U16 resolution, F32 resolutionFactor);
    void previewShadowMaps();
    void togglePreviewShadowMaps(bool state);
    void init(ShadowMapInfo* const smi);

protected:
    ///Simple frustum representation (no ratio or fov needed for now)
    struct frustum {
        vec3<F32> wsPoints[8];
        vec3<F32> lsPoints[8];
    };

#if defined(CSM_USE_LAYERED_RENDERING)
    void extractShadowCastersAndReceivers(const SceneRenderState& renderState);
#endif

    void prepareDebugView();
    void releaseDebugView();
    void drawFrustum(bool lightFrustum);

    void CalculateSplitDepths(const Camera& cam, const vec2<F32>& zPlanes);
    void ApplyFrustumSplit(U8 pass, const vec2<F32>& zPlanes);

protected:
    U8  _numSplits;
    U8  _splitPadding; //<Avoid artifacts;
    F32 _splitLogFactor;
    bool _updateFrustum;
    mat4<F32> _viewMatrixCache;
    mat4<F32> _viewInvMatrixCache;
    Quad3D* _renderQuad;
    ShaderProgram*  _previewDepthMapShader;
    ShaderProgram*  _blurDepthMapShader;
    vectorImpl<frustum >   _frustum;
    vectorImpl<const SceneGraphNode* > _casters;
    vectorImpl<const SceneGraphNode* > _receivers;
    ///The blur buffer
    FrameBuffer*        _blurBuffer;
    ///For frusta preview
    IMPrimitive*        _primitive;

    vectorImpl<vec3<F32> > _frustumCornersVS;
    vectorImpl<vec3<F32> > _frustumCornersWS;
    vectorImpl<vec3<F32> > _frustumCornersLS;
    vectorImpl<vec3<F32> > _farFrustumCornersVS;
    vectorImpl<vec3<F32> > _splitFrustumCornersVS;
    vectorImpl<F32 >       _splitDepths;


};
#endif 