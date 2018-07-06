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

#ifndef _FORWARD_PLUS_RENDERER_H_
#define _FORWARD_PLUS_RENDERER_H_

#include "Renderer.h"
#include "Rendering/Lighting/Headers/LightGrid.h"
/// This class implements the forward plus renderer that creates a list of all important lights in screen space 
/// Details: http://mynameismjp.wordpress.com/2012/03/31/light-indexed-deferred-rendering/

namespace Divide {

class ForwardPlusRenderer : public Renderer {

public:
	ForwardPlusRenderer();
	~ForwardPlusRenderer();
    void processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes, const GFXDevice::GPUBlock& gpuBlock);
	void render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState);
    void updateResolution(U16 width, U16 height);

protected:
    bool buildLightGrid(const GFXDevice::GPUBlock& gpuBlock);
    void downSampleDepthBuffer(vectorImpl<vec2<F32>> &depthRanges);

private:
    LightGrid* _opaqueGrid;
    LightGrid* _transparentGrid;
    Framebuffer* _depthRanges;
    ShaderProgram* _depthRangesConstructProgram;
    vectorImpl<vec2<F32> > _depthRangesCache;
    vectorImpl<LightGrid::LightInternal > _omniLightList;
};

}; //namespace Divide

#endif