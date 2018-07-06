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

#ifndef _DEF_SHADE_RENDERER_H_
#define _DEF_SHADE_RENDERER_H_

#include "Renderer.h"

namespace Divide {

class Quad3D;
class Framebuffer;
class PixelBuffer;
class ShaderProgram;

///This class implements a full  deferred renderer based on the 2 pass, huge G-buffer model
class DeferredShadingRenderer : public Renderer {
public:
	DeferredShadingRenderer();
	~DeferredShadingRenderer();
    void processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes, const GFXDevice::GPUBlock& gpuBlock);
	void render(const DELEGATE_CBK<>& renderCallback, const SceneRenderState& sceneRenderState);
	void toggleDebugView();
    void updateResolution(U16 width, U16 height);

private:
	void firstPass(const DELEGATE_CBK<>& renderCallback, const SceneRenderState& sceneRenderState);
	void secondPass(const SceneRenderState& sceneRenderState);

private:
	U16 _cachedLightCount;
	vectorImpl<Quad3D* >  _renderQuads;
	Framebuffer*    _deferredBuffer;
	ShaderProgram*  _deferredShader;
	ShaderProgram*  _previewDeferredShader;
	PixelBuffer*    _lightTexture;
};

}; //namespace Divide

#endif