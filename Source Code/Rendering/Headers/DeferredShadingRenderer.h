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

#ifndef _DEF_SHADE_RENDERER_H_
#define _DEF_SHADE_RENDERER_H_
#include "Renderer.h"
#include "Utility/Headers/vector.h"
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
	void render(boost::function0<void> renderCallback, SceneRenderState* const sceneRenderState);
	void toggleDebugView();
private:
	void firstPass(boost::function0<void> renderCallback);
	void secondPass(SceneRenderState* const sceneRenderState);

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