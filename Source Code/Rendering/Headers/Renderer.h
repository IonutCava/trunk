/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

enum class RendererType : U32 {
    RENDERER_TILED_FORWARD_SHADING = 0,
    RENDERER_DEFERRED_SHADING = 1,
    COUNT
};

class LightPool;
class ResourceCache;
class PlatformContext;
/// An abstract renderer used to switch between different rendering techniques:
/// TiledForwardShading, Deferred Shading, etc
class NOINITVTABLE Renderer {
   public:
    Renderer(PlatformContext& context, ResourceCache& cache, RendererType type);
    virtual ~Renderer();

    virtual void preRender(RenderTarget& target, LightPool& lightPool);

    virtual void render(const DELEGATE_CBK<>& renderCallback,
                        const SceneRenderState& sceneRenderState) = 0;

    virtual void updateResolution(U16 width, U16 height) = 0;

    inline RendererType getType() const { return _type; }
    inline void toggleDebugView() { _debugView = !_debugView; }

    inline U32 getFlag() const { return _flag; }

   protected:
    PlatformContext& _context;
    ResourceCache& _resCache;
    // General purpose flag
    U32 _flag;
    bool _debugView;
    RendererType _type;
};

};  // namespace Divide

#endif