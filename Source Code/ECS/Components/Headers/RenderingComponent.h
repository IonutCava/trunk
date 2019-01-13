/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _RENDERING_COMPONENT_H_
#define _RENDERING_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Rendering/Headers/EnvironmentProbe.h"

namespace Divide {

class Sky;
class SubMesh;
class Material;
class GFXDevice;
class RenderBin;
class WaterPlane;
class ImpostorBox;
class SceneGraphNode;
class ParticleEmitter;

TYPEDEF_SMART_POINTERS_FOR_TYPE(Material);

namespace Attorney {
    class RenderingCompRenderPass;
    class RenderingCompGFXDevice;
    class RenderingCompRenderBin;
};

struct RenderParams {
    GenericDrawCommand _cmd;
    Pipeline _pipeline;
};

struct RefreshNodeDataParams {
    explicit RefreshNodeDataParams(vectorEASTL<IndirectDrawCommand>& commands, GFX::CommandBuffer& bufferInOut)
        : _drawCommandsInOut(commands),
        _bufferInOut(bufferInOut)
    {

    }

    RenderStagePass _stagePass = {};
    U32 _nodeCount = 0;

    vectorEASTL<IndirectDrawCommand>& _drawCommandsInOut;
    GFX::CommandBuffer& _bufferInOut;

};

struct RenderCbkParams {
    explicit RenderCbkParams(GFXDevice& context,
                             const SceneGraphNode& sgn,
                             const SceneRenderState& sceneRenderState,
                             const RenderTargetID& renderTarget,
                             U32 passIndex,
                             Camera* camera)
        : _context(context),
          _sgn(sgn),
          _sceneRenderState(sceneRenderState),
          _renderTarget(renderTarget),
          _passIndex(passIndex),
          _camera(camera)
    {
    }

    GFXDevice& _context;
    const SceneGraphNode& _sgn;
    const SceneRenderState& _sceneRenderState;
    const RenderTargetID& _renderTarget;
    U32 _passIndex;
    Camera* _camera;
};


enum class ReflectorType : U8 {
    PLANAR_REFLECTOR = 0,
    CUBE_REFLECTOR = 1,
    COUNT
};

typedef DELEGATE_CBK<void, RenderCbkParams&, GFX::CommandBuffer&> RenderCallback;

class RenderingComponent : public BaseComponentType<RenderingComponent, ComponentType::RENDERING> {
    friend class Attorney::RenderingCompRenderPass;
    friend class Attorney::RenderingCompGFXDevice;
    friend class Attorney::RenderingCompRenderBin;

   public:
       enum class RenderOptions : U16 {
           RENDER_GEOMETRY = toBit(1),
           RENDER_WIREFRAME = toBit(2),
           RENDER_BOUNDS_AABB = toBit(3),
           RENDER_BOUNDS_SPHERE = toBit(4),
           RENDER_SKELETON = toBit(5),
           CAST_SHADOWS = toBit(6),
           RECEIVE_SHADOWS = toBit(7),
           IS_VISIBLE = toBit(8),
           IS_OCCLUSION_CULLABLE = toBit(9)
       };

   public:
    explicit RenderingComponent(SceneGraphNode& parentSGN,
                                PlatformContext& context);
    ~RenderingComponent();

    void Update(const U64 deltaTimeUS) override;

    inline PushConstants& pushConstants() { return _globalPushConstants; }
    inline const PushConstants& pushConstants() const { return _globalPushConstants; }

    void toggleRenderOption(RenderOptions option, bool state);
    bool renderOptionEnabled(RenderOptions option) const;
    bool renderOptionsEnabled(U32 mask) const;

    inline void lockLoD(bool state) { _lodLocked = state; }
    inline bool lodLocked() const { return _lodLocked; }

    void getMaterialColourMatrix(mat4<F32>& matOut) const;

    void getRenderingProperties(RenderStagePass& stagePass, vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const;

    RenderPackage& getDrawPackage(RenderStagePass renderStagePass);
    const RenderPackage& getDrawPackage(RenderStagePass renderStagePass) const;

    size_t getSortKeyHash(RenderStagePass renderStagePass) const;

    inline const Material_ptr& getMaterialInstance() const { return _materialInstance; }

    void rebuildMaterial();

    inline void setReflectionAndRefractionType(ReflectorType type) { _reflectorType = type; }
    inline void setReflectionCallback(RenderCallback cbk) { _reflectionCallback = cbk; }
    inline void setRefractionCallback(RenderCallback cbk) { _refractionCallback = cbk; }

    void drawDebugAxis();
    void onRender(RenderStagePass renderStagePass);

   protected:    
    bool onRefreshNodeData(RefreshNodeDataParams& refreshParams);
    bool canDraw(RenderStagePass renderStagePass);
    U8 getLoDLevel(const Camera& camera, RenderStagePass renderStagePass, const vec4<U16>& lodThresholds);

    /// Called after the parent node was rendered
    void postRender(const SceneRenderState& sceneRenderState,
                    RenderStagePass renderStagePass,
                    GFX::CommandBuffer& bufferInOut);

    void rebuildDrawCommands(RenderStagePass stagePass);

    void prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass);

    // This returns false if the node is not reflective, otherwise it generates a new reflection cube map
    // and saves it in the appropriate material slot
    bool updateReflection(U32 reflectionIndex, 
                          Camera* camera,
                          const SceneRenderState& renderState,
                          GFX::CommandBuffer& bufferInOut);
    bool updateRefraction(U32 refractionIndex,
                          Camera* camera,
                          const SceneRenderState& renderState,
                          GFX::CommandBuffer& bufferInOut);
    bool clearReflection();
    bool clearRefraction();

    void updateEnvProbeList(const EnvironmentProbeList& probes);

   protected:
    GFXDevice& _context;
    Material_ptr _materialInstance;

    U32 _renderMask;
    bool _lodLocked;

    typedef std::array<std::unique_ptr<RenderPackage>, to_base(RenderPassType::COUNT)> RenderPackagesPerPassType;
    std::array<RenderPackagesPerPassType, to_base(RenderStage::COUNT)> _renderPackages;
    
    std::array<bool, RenderStagePass::count()> _renderPackagesDirty;
    PushConstants _globalPushConstants;

    IMPrimitive* _boundingBoxPrimitive[2];
    IMPrimitive* _boundingSpherePrimitive;
    IMPrimitive* _skeletonPrimitive;

    RenderCallback _reflectionCallback;
    RenderCallback _refractionCallback;

    EnvironmentProbeList _envProbes;
    vector<Line> _axisLines;
    IMPrimitive* _axisGizmo;

    ReflectorType _reflectorType;
    
    ShaderProgram_ptr _previewRenderTargetColour;
    ShaderProgram_ptr _previewRenderTargetDepth;

    static hashMap<U32, DebugView*> s_debugViews[2];
};

INIT_COMPONENT(RenderingComponent);

namespace Attorney {
class RenderingCompRenderPass {
    private:
        static bool updateReflection(RenderingComponent& renderable,
                                     U32 reflectionIndex,
                                     Camera* camera,
                                     const SceneRenderState& renderState,
                                     GFX::CommandBuffer& bufferInOut)
        {
            return renderable.updateReflection(reflectionIndex, camera, renderState, bufferInOut);
        }

        static bool updateRefraction(RenderingComponent& renderable,
                                     U32 refractionIndex,
                                     Camera* camera,
                                     const SceneRenderState& renderState,
                                     GFX::CommandBuffer& bufferInOut)
        {
            return renderable.updateRefraction(refractionIndex, camera, renderState, bufferInOut);
        }

        static bool clearReflection(RenderingComponent& renderable)
        {
            return renderable.clearReflection();
        }

        static bool clearRefraction(RenderingComponent& renderable)
        {
            return renderable.clearRefraction();
        }

        static void updateEnvProbeList(RenderingComponent& renderable, const EnvironmentProbeList& probes) {
            renderable.updateEnvProbeList(probes);
        }

        friend class Divide::RenderPass;

        static void prepareDrawPackage(RenderingComponent& renderable,
                                       const Camera& camera,
                                       const SceneRenderState& sceneRenderState,
                                       RenderStagePass renderStagePass) {
            renderable.prepareDrawPackage(camera, sceneRenderState, renderStagePass);
        }

        static bool onRefreshNodeData(RenderingComponent& renderable, RefreshNodeDataParams& refreshParams) {
            return renderable.onRefreshNodeData(refreshParams);
        }

    friend class Divide::RenderPass;
    friend class Divide::RenderPassManager;
};

class RenderingCompRenderBin {
   private:
    static size_t getSortKeyHash(RenderingComponent& renderable, RenderStagePass renderStagePass) {
        return renderable.getSortKeyHash(renderStagePass);
    }

    static void postRender(RenderingComponent& renderable,
                           const SceneRenderState& sceneRenderState,
                           RenderStagePass renderStagePass,
                           GFX::CommandBuffer& bufferInOut) {
        renderable.postRender(sceneRenderState, renderStagePass, bufferInOut);
    }

    friend class Divide::RenderBin;
    friend struct Divide::RenderBinItem;
};

};  // namespace Attorney
};  // namespace Divide
#endif