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
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

class Material;
class ShaderProgram;
class SceneGraphNode;
class RenderingComponent : public SGNComponent {
   public:
    RenderingComponent(Material* const materialInstance,
                       SceneGraphNode& parentSGN);
    ~RenderingComponent();

    /// Draw the parent scene graph node
    void render(const SceneRenderState& sceneRenderState,
                const RenderStage& currentRenderStage);
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

    /// Called after the parent node was rendered
    void postDraw(const SceneRenderState& sceneRenderState,
                  RenderStage renderStage);

    U8 lodLevel() const;
    void lodLevel(U8 LoD);

    ShaderProgram* const getDrawShader(
        RenderStage renderStage = RenderStage::FINAL_STAGE);
    size_t getDrawStateHash(RenderStage renderStage);

    inline const vectorImpl<GenericDrawCommand>& getDrawCommands() const {
        return _drawCommandsCache;
    }
    inline void getMaterialColorMatrix(mat4<F32>& matOut) const {
        return matOut.set(_materialColorMatrix);
    }
    inline void getMaterialPropertyMatrix(mat4<F32>& matOut) const {
        return matOut.set(_materialPropertyMatrix);
    }
    inline Material* const getMaterialInstance() { return _materialInstance; }

    const vectorImpl<GenericDrawCommand>& getDrawCommands(
        SceneRenderState& sceneRenderState, RenderStage renderStage);

#ifdef _DEBUG
    void drawDebugAxis();
#endif
   protected:
    friend class SceneGraphNode;
    void inViewCallback();

   protected:
    Material* _materialInstance;
    /// LOD level is updated at every visibility check
    /// (SceneNode::isInView(...));
    U8 _lodLevel;  ///<Relative to camera distance
    bool _castsShadows;
    bool _receiveShadows;
    bool _renderWireframe;
    bool _renderBoundingBox;
    bool _renderSkeleton;
    mat4<F32> _materialColorMatrix;
    mat4<F32> _materialPropertyMatrix;
    vectorImpl<GenericDrawCommand> _drawCommandsCache;

#ifdef _DEBUG
    vectorImpl<Line> _axisLines;
    IMPrimitive* _axisGizmo;
#endif
};

};  // namespace Divide
#endif