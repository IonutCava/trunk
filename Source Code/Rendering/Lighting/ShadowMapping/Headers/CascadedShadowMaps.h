/*
   Copyright (c) 2015 DIVIDE-Studio
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
namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class IMPrimitive;
class ShaderProgram;
class SceneGraphNode;

class DirectionalLight;
/// Directional lights can't deliver good quality shadows using a single shadow map. 
/// This technique offers an implementation of the CSM method
class CascadedShadowMaps : public ShadowMap {
public:
    CascadedShadowMaps(Light* light, Camera* shadowCamera, F32 numSplits);
    ~CascadedShadowMaps();
    void render(SceneRenderState& renderState, const DELEGATE_CBK<>& sceneRenderFunction);
    void postRender();
    ///Update depth maps
    void resolution(U16 resolution, U8 resolutionFactor);
    void previewShadowMaps();
    void togglePreviewShadowMaps(bool state);
    void init(ShadowMapInfo* const smi);

protected:
    bool BindInternal(U8 offset);
    void CalculateSplitDepths(const Camera& cam);
    void ApplyFrustumSplit(U8 pass);
    void updateResolution(I32 newWidth, I32 newHeight);

protected:
    U8  _numSplits;
    F32 _splitLogFactor;
    F32 _nearClipOffset;
    U32 _horizBlur;
    U32 _vertBlur;
    vec2<F32> _sceneZPlanes;
    vec3<F32> _lightPosition;
    mat4<F32> _viewInvMatrixCache;
    ShaderProgram*  _previewDepthMapShader;
    ShaderProgram*  _blurDepthMapShader;
    Framebuffer::FramebufferTarget* _renderPolicy;
    ///Shortcut for the owning directional light
    DirectionalLight*  _dirLight;
    Framebuffer*       _blurBuffer;
    vectorImpl<vec3<F32> > _frustumCornersVS;
    vectorImpl<vec3<F32> > _frustumCornersWS;
    vectorImpl<vec3<F32> > _frustumCornersLS;
    vectorImpl<vec3<F32> > _splitFrustumCornersVS;
    vectorImpl<F32 >       _splitDepths;
};

}; //namespace Divide
#endif 