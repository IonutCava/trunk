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
#include "Utility/Headers/Vector.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

class Quad3D;
class ShaderProgram;
class FrameBufferObject;
class PixelBufferObject;

///This class implements a full  deferred renderer based on the 2 pass, huge G-buffer model
class DeferredShadingRenderer : public Renderer {
public:
	DeferredShadingRenderer();
	~DeferredShadingRenderer();
	void render(boost::function0<void> renderCallback, const SceneRenderState& sceneRenderState);
	void toggleDebugView();
private:
	void firstPass(boost::function0<void> renderCallback, const SceneRenderState& sceneRenderState);
	void secondPass(const SceneRenderState& sceneRenderState);

private:
	U16 _cachedLightCount;
	bool _debugView;
	vectorImpl<Quad3D* >  _renderQuads;
	FrameBufferObject*    _deferredBuffer;
	ShaderProgram*	      _deferredShader;
	ShaderProgram*        _previewDeferredShader;
	PixelBufferObject*    _lightTexture;
};

#endif