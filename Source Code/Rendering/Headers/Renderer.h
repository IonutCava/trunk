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

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
class SceneRenderState;

enum RendererType{
	RENDERER_FORWARD = 0,
	RENDERER_DEFERRED_SHADING,
	RENDERER_DEFERRED_LIGHTING,
	RENDERER_PLACEHOLDER
};
///An abstract renderer used to switch between different rendering tehniques: Forward, Deferred Shading or Deferred Lighting
class Renderer {
public:
	Renderer(RendererType type) : _type(type) {}
	virtual ~Renderer() {}
	virtual void render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState) = 0;
	virtual void toggleDebugView() = 0;
	inline RendererType getType() {return _type;}

protected:
	RendererType _type;
};

#endif