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

#include "PreRenderBatch.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class Framebuffer;
class ShaderProgram;
class Texture;

DEFINE_SINGLETON(PostFX)
   private:
      enum class TexOperatorBindPoint : U32 {
          TEX_BIND_POINT_SCREEN = 0,
          TEX_BIND_POINT_BORDER = 1,
          TEX_BIND_POINT_NOISE = 2,
          TEX_BIND_POINT_UNDERWATER = 3,
          TEX_BIND_POINT_LEFT_EYE = 4,
          TEX_BIND_POINT_RIGHT_EYE = 5,
          COUNT
      };

  private:
    ~PostFX();
    PostFX();

  public:
    void displayScene();

    void init(const vec2<U16>& resolution);
    void idle();
    void updateResolution(U16 newWidth, U16 newHeight);

  private:
    bool _enableNoise;
    bool _enableVignette;
    bool _underwater;

    U32 _filterMask;

    PreRenderBatch _preRenderBatch;
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
