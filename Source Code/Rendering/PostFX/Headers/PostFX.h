/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _POST_EFFECTS_H
#define _POST_EFFECTS_H

#include "Core/Headers/Singleton.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class Framebuffer;
class ShaderProgram;
class PreRenderOperator;

class Texture;

DEFINE_SINGLETON(PostFX)
  private:
    ~PostFX();
    PostFX();
    void createOperators();

  public:
    void displayScene();

    void init(const vec2<U16>& resolution);
    void idle();
    void updateResolution(I32 newWidth, I32 newHeight);

  private:
    bool _enableBloom;
    bool _enableDOF;
    bool _enableNoise;
    bool _enableVignette;
    bool _enableSSAO;
    bool _enableFXAA;
    bool _underwater;
    bool _depthPreview;

    enum class TexOperatorBindPoint : U32 {
        TEX_BIND_POINT_SCREEN = 0,
        TEX_BIND_POINT_BLOOM = 1,
        TEX_BIND_POINT_SSAO = 2,
        TEX_BIND_POINT_BORDER = 3,
        TEX_BIND_POINT_NOISE = 4,
        TEX_BIND_POINT_UNDERWATER = 5,
        TEX_BIND_POINT_LEFT_EYE = 6,
        TEX_BIND_POINT_RIGHT_EYE = 7,
    };
    /// Bloom
    Framebuffer* _bloomFB;
    PreRenderOperator* _bloomOP;

    /// SSAO
    Framebuffer* _SSAO_FB;

    /// FXAA
    PreRenderOperator* _fxaaOP;

    /// DoF
    PreRenderOperator* _dofOP;

    /// Screen Border
    Texture* _screenBorder;

    /// Noise
    Texture* _noise;

    F32 _randomNoiseCoefficient, _randomFlashCoefficient;
    D32 _timer, _tickInterval;

    ShaderProgram* _anaglyphShader;
    ShaderProgram* _postProcessingShader;
    Texture* _underwaterTexture;
    GFXDevice* _gfx;
    vec2<U16> _resolutionCache;
    vectorImpl<U32> _shaderFunctionSelection;
    vectorImpl<I32> _shaderFunctionList;
END_SINGLETON

};  // namespace Divide

#endif
