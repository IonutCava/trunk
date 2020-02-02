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
#include "Platform/Video/Headers/RenderPackage.h"
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

using DrawCommandContainer = eastl::fixed_vector<IndirectDrawCommand, Config::MAX_VISIBLE_NODES, false>;

struct RefreshNodeDataParams {
    explicit RefreshNodeDataParams(DrawCommandContainer& commands, GFX::CommandBuffer& bufferInOut)
        : _drawCommandsInOut(commands),
          _bufferInOut(bufferInOut)
    {

    }

    RenderStagePass _stagePass = {};
    U32 _dataIdx = 0;
    const Camera* _camera = nullptr;
    DrawCommandContainer& _drawCommandsInOut;
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

constexpr std::pair<RenderTargetUsage, ShaderProgram::TextureUsage> g_texUsage[] = {
    { RenderTargetUsage::REFLECTION_PLANAR, ShaderProgram::TextureUsage::REFLECTION_PLANAR},
    { RenderTargetUsage::REFRACTION_PLANAR, ShaderProgram::TextureUsage::REFRACTION_PLANAR},
    { RenderTargetUsage::REFLECTION_CUBE, ShaderProgram::TextureUsage::REFLECTION_CUBE }
};

class RenderingComponent final : public BaseComponentType<RenderingComponent, ComponentType::RENDERING> {
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
           IS_VISIBLE = toBit(8)
       };

   public:
    explicit RenderingComponent(SceneGraphNode& parentSGN,
                                PlatformContext& context);
    ~RenderingComponent();

    void Update(const U64 deltaTimeUS) final;

    inline PushConstants& pushConstants() { return _globalPushConstants; }
    inline const PushConstants& pushConstants() const { return _globalPushConstants; }

    void toggleRenderOption(RenderOptions option, bool state);
    bool renderOptionEnabled(RenderOptions option) const;
    bool renderOptionsEnabled(U32 mask) const;

    void setMinRenderRange(F32 minRange);
    void setMaxRenderRange(F32 maxRange);
    inline void setRenderRange(F32 minRange, F32 maxRange) { setMinRenderRange(minRange); setMaxRenderRange(maxRange); }
    inline const vec2<F32>& renderRange() const { return _renderRange; }

    // If the new value is negative, this disables occlusion culling!
    void cullFlagValue(F32 newValue);

    inline void lockLoD(bool state) { _lodLocked = state; }
    inline bool lodLocked() const { return _lodLocked; }

    void useUniqueMaterialInstance();
    void setMaterialTpl(const Material_ptr& material);

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

    U8 getLoDLevel(const BoundsComponent& bComp, const vec3<F32>& cameraEye, RenderStage renderStage, const vec4<U16>& lodThresholds);

    inline void addShaderBuffer(const ShaderBufferBinding& binding) { _externalBufferBindings.push_back(binding); }
    inline const vectorEASTL<ShaderBufferBinding>& getShaderBuffers() const { return _externalBufferBindings; }

    void queueRebuildCommands(RenderStagePass renderStagePass);

    bool getDataIndex(U32& idxOut);
    void setDataIndex(U32 idx);

    PROPERTY_RW(bool, useDataIndexAsUniform, false);

  protected:
    bool onRefreshNodeData(RefreshNodeDataParams& refreshParams);
    bool onQuickRefreshNodeData(RefreshNodeDataParams& refreshParams);
    void uploadDataIndexAsUniform(RenderStagePass stagePass);

    bool canDraw(RenderStagePass renderStagePass, U8 LoD, bool refreshData);

    /// Called after the parent node was rendered
    void postRender(const SceneRenderState& sceneRenderState,
                    RenderStagePass renderStagePass,
                    GFX::CommandBuffer& bufferInOut);

    void rebuildDrawCommands(RenderStagePass stagePass);

    void prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass, bool refreshData);

    // This returns false if the node is not reflective, otherwise it generates a new reflection cube map
    // and saves it in the appropriate material slot
    bool updateReflection(U16 reflectionIndex, 
                          Camera* camera,
                          const SceneRenderState& renderState,
                          GFX::CommandBuffer& bufferInOut);
    bool updateRefraction(U16 refractionIndex,
                          Camera* camera,
                          const SceneRenderState& renderState,
                          GFX::CommandBuffer& bufferInOut);
    bool clearReflection();
    bool clearRefraction();

    void updateEnvProbeList(const EnvironmentProbeList& probes);

    void defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);
    void defaultRefractionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);

    U32 defaultReflectionTextureIndex() const;
    U32 defaultRefractionTextureIndex() const;

   protected:
    void updateReflectionIndex(ReflectorType type, I32 index);
    void updateRefractionIndex(ReflectorType type, I32 index);

    void onMaterialChanged();

   protected:
    GFXDevice& _context;
    const Configuration& _config;
    Material_ptr _materialInstance;
    Material* _materialInstanceCache;

    std::pair<U32, bool> _dataIndex;
    F32 _cullFlagValue;
    U32 _renderMask;
    bool _lodLocked;
    vec2<F32> _renderRange;

    std::array<U8, to_base(RenderStage::COUNT)> _lodLevels;
    std::array<U32, to_base(RenderStage::COUNT)> _drawDataIdx;

    using RenderPackagesPerPassType = std::array<RenderPackage, to_base(RenderPassType::COUNT)>;
    std::array<RenderPackagesPerPassType, to_base(RenderStage::COUNT) - 1> _renderPackagesNormal;

    using RenderPacakgesPerSplit = std::array<RenderPackage, 6>;
    std::array<RenderPacakgesPerSplit, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS> _renderPackagesShadow;

    PushConstants _globalPushConstants;

    Pipeline*    _primitivePipeline[3];
    IMPrimitive* _boundingBoxPrimitive[2];
    IMPrimitive* _boundingSpherePrimitive;
    IMPrimitive* _skeletonPrimitive;
    IMPrimitive* _axisGizmo;

    RenderCallback _reflectionCallback;
    RenderCallback _refractionCallback;

    EnvironmentProbeList _envProbes;

    ReflectorType _reflectorType;
    
    /// used to keep track of what GFXDevice::reflectionTarget we are using for this rendering pass
    I32 _reflectionIndex;
    I32 _refractionIndex;
    std::pair<Texture_ptr, U32> _defaultReflection;
    std::pair<Texture_ptr, U32> _defaultRefraction;

    vectorEASTL<ShaderBufferBinding> _externalBufferBindings;

    std::array<Texture_ptr, (sizeof(g_texUsage) / sizeof(g_texUsage[0]))> _externalTextures;

    static hashMap<U32, DebugView*> s_debugViews[2];
};

INIT_COMPONENT(RenderingComponent);

namespace Attorney {
class RenderingCompRenderPass {
    private:
        static bool updateReflection(RenderingComponent& renderable,
                                     U16 reflectionIndex,
                                     Camera* camera,
                                     const SceneRenderState& renderState,
                                     GFX::CommandBuffer& bufferInOut)
        {
            return renderable.updateReflection(reflectionIndex, camera, renderState, bufferInOut);
        }

        static bool updateRefraction(RenderingComponent& renderable,
                                     U16 refractionIndex,
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
                                       RenderStagePass renderStagePass,
                                       bool refreshData) {
            renderable.prepareDrawPackage(camera, sceneRenderState, renderStagePass, refreshData);
        }

        static bool onRefreshNodeData(RenderingComponent& renderable, RefreshNodeDataParams& refreshParams) {
            return renderable.onRefreshNodeData(refreshParams);
        }

        static bool onQuickRefreshNodeData(RenderingComponent& renderable, RefreshNodeDataParams& refreshParams) {
            return renderable.onQuickRefreshNodeData(refreshParams);
        }


    friend class Divide::RenderPass;
    friend class Divide::RenderPassManager;
};

class RenderingCompRenderBin {
   private:
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