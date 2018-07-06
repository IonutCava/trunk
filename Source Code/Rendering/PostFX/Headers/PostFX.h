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

#ifndef _POST_EFFECTS_H
#define _POST_EFFECTS_H

#include "core.h"

class GFXDevice;
class Quad3D;
class Camera;
class ShaderProgram;
class FrameBufferObject;
class PreRenderOperator;

class Texture;
typedef Texture Texture2D;

DEFINE_SINGLETON( PostFX )

private:
    ~PostFX();
    PostFX();
    void createOperators();

public:

    void displaySceneWithoutAnaglyph();
    void displaySceneWithAnaglyph();

    void init(const vec2<U16>& resolution);
    void idle();
    void reshapeFBO(I32 newwidth , I32 newheight);

    inline void toggleDepthPreview(const bool state) {_depthPreview = state;}

private:
    bool _enableBloom;
    bool _enableDOF;
    bool _enableNoise;
    bool _enableSSAO;
    bool _enableFXAA;
    bool _underwater;
    bool _depthPreview;
    /// Bloom
    FrameBufferObject* _bloomFBO;
    PreRenderOperator* _bloomOP;

    /// SSAO
    FrameBufferObject* _SSAO_FBO;

    /// FXAA
    PreRenderOperator* _fxaaOP;

    /// DoF
    PreRenderOperator* _dofOP;

    /// Screen Border
    Texture2D*	_screenBorder;

    /// Noise
    Texture2D*	_noise;

    F32 _randomNoiseCoefficient, _randomFlashCoefficient;
    D32 _timer, _tickInterval;

    Quad3D*	_renderQuad;
    ShaderProgram* _anaglyphShader;
    ShaderProgram* _postProcessingShader;
    Texture2D* _underwaterTexture;
    GFXDevice& _gfx;

END_SINGLETON

#endif