/*
Copyright (c) 2015 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _RENDERING_COMPONENT_H_
#define _RENDERING_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

class Material;
class ShaderProgram;
class SceneGraphNode;
class RenderingComponent : public SGNComponent {
    friend class RenderingCompGFXDeviceAttorney;
    friend class RenderingCompSceneGraphAttorney;
    friend class RenderingCompRenderBinAttorney;
    friend class RenderingCompPassCullerAttorney;

   public:
    RenderingComponent(Material* const materialInstance,
                       SceneGraphNode& parentSGN);
    ~RenderingComponent();

    bool onDraw(RenderStage currentStage);
    void update(const U64 deltaTime);

    void renderWireframe(const bool state);
    void renderBoundingBox(const bool state);
    void renderSkeleton(const bool state);

    inline bool renderWireframe() const { return _renderWireframe; }
    inline bool renderBoundingBox() const { return _renderBoundingBox; }
    inline bool renderSkeleton() const { return _renderSkeleton; }

    void castsShadows(const bool state);
    void receivesShadows(const bool state);

    bool castsShadows() const;
    bool receivesShadows() const;

    U8 lodLevel() const;
    void lodLevel(U8 LoD);

    inline U32 drawOrder() const { return _drawOrder; }

    ShaderProgram* const getDrawShader(
        RenderStage renderStage = RenderStage::DISPLAY_STAGE);

    size_t getDrawStateHash(RenderStage renderStage);

    inline void getMaterialColorMatrix(mat4<F32>& matOut) const {
        return matOut.set(_materialColorMatrix);
    }

    inline void getMaterialPropertyMatrix(mat4<F32>& matOut) const {
        return matOut.set(_materialPropertyMatrix);
    }

    inline Material* const getMaterialInstance() { return _materialInstance; }

    void makeTextureResident(const Texture& texture, U8 slot);

    void registerShaderBuffer(ShaderBufferLocation slot,
                              ShaderBuffer& shaderBuffer);

    void unregisterShaderBuffer(ShaderBufferLocation slot);

    inline const GFXDevice::RenderPackage& getRenderData() const {
        return _renderData;
    }

#ifdef _DEBUG
    void drawDebugAxis();
#endif

   protected:
    void inViewCallback();
    bool canDraw(const SceneRenderState& sceneRenderState,
                 RenderStage renderStage);
    /// Called after the parent node was rendered
    void postDraw(const SceneRenderState& sceneRenderState,
                  RenderStage renderStage);
    vectorImpl<GenericDrawCommand>& getDrawCommands(
        SceneRenderState& sceneRenderState,
        RenderStage renderStage);

    inline void drawOrder(U32 index) { _drawOrder = index; }

   protected:
    Material* _materialInstance;
    /// LOD level is updated at every visibility check
    /// (SceneNode::isInView(...));
    U8 _lodLevel;  ///<Relative to camera distance
    U32 _drawOrder;
    bool _castsShadows;
    bool _receiveShadows;
    bool _renderWireframe;
    bool _renderBoundingBox;
    bool _renderSkeleton;
    bool _renderingLocked;
    mat4<F32> _materialColorMatrix;
    mat4<F32> _materialPropertyMatrix;

    GFXDevice::RenderPackage _renderData;

#ifdef _DEBUG
    vectorImpl<Line> _axisLines;
    IMPrimitive* _axisGizmo;
#endif
};

class RenderingCompGFXDeviceAttorney {
   private:
    static void lockRendering(RenderingComponent& renderable) {
        renderable._renderingLocked = true;
    }
    static void unlockRendering(RenderingComponent& renderable) {
        renderable._renderingLocked = false;
    }

    static vectorImpl<GenericDrawCommand>& getDrawCommands(
        RenderingComponent& renderable,
        SceneRenderState& sceneRenderState,
        RenderStage renderStage) {
        return renderable.getDrawCommands(sceneRenderState, renderStage);
    }

    static bool canDraw(RenderingComponent& renderable,
                        const SceneRenderState& sceneRenderState,
                        RenderStage renderStage) {
        return renderable.canDraw(sceneRenderState, renderStage);
    }
    friend class GFXDevice;
};

class RenderingCompSceneGraphAttorney {
   private:
    static void inViewCallback(RenderingComponent& renderable) {
        renderable.inViewCallback();
    }

    friend class SceneGraphNode;
};

class RenderingCompRenderBinAttorney {
   private:
    static void postDraw(RenderingComponent& renderable,
                         const SceneRenderState& sceneRenderState,
                         RenderStage renderStage) {
        renderable.postDraw(sceneRenderState, renderStage);
    }

    static void drawOrder(RenderingComponent& renderable, U32 index) {
        renderable.drawOrder(index);
    }

    friend class RenderBin;
};

class RenderingCompPassCullerAttorney {
   private:
    static bool canDraw(RenderingComponent& renderable,
                        const SceneRenderState& sceneRenderState,
                        RenderStage renderStage) {
        return renderable.canDraw(sceneRenderState, renderStage);
    }

    friend class RenderPassCuller;
};

};  // namespace Divide
#endif