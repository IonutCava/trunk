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
#include "Geometry/Material/Headers/MaterialEnums.h"
#include "Rendering/Headers/EnvironmentProbe.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"

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
    class RenderingComponentSGN;
};

struct RenderParams {
    GenericDrawCommand _cmd;
    Pipeline _pipeline;
};

using DrawCommandContainer = eastl::fixed_vector<IndirectDrawCommand, Config::MAX_VISIBLE_NODES, false>;

struct RefreshNodeDataParams {
    DrawCommandContainer* _drawCommandsInOut = nullptr;
    GFX::CommandBuffer* _bufferInOut = nullptr;
    const RenderStagePass* _stagePass = nullptr;
    const Camera* _camera = nullptr;
};

struct RenderCbkParams {
    explicit RenderCbkParams(GFXDevice& context,
                             const SceneGraphNode& sgn,
                             const SceneRenderState& sceneRenderState,
                             const RenderTargetID& renderTarget,
                             U32 passIndex,
                             U8 passVariant,
                             Camera* camera) noexcept
        : _context(context),
          _sgn(sgn),
          _sceneRenderState(sceneRenderState),
          _renderTarget(renderTarget),
          _passIndex(passIndex),
          _passVariant(passVariant),
          _camera(camera)
    {
    }

    GFXDevice& _context;
    const SceneGraphNode& _sgn;
    const SceneRenderState& _sceneRenderState;
    const RenderTargetID& _renderTarget;
    U32 _passIndex;
    U8  _passVariant;
    Camera* _camera;
};

using RenderCallback = DELEGATE<void, RenderCbkParams&, GFX::CommandBuffer&>;

constexpr std::pair<RenderTargetUsage, TextureUsage> g_texUsage[] = {
    { RenderTargetUsage::REFLECTION_PLANAR, TextureUsage::REFLECTION_PLANAR},
    { RenderTargetUsage::REFRACTION_PLANAR, TextureUsage::REFRACTION_PLANAR},
    { RenderTargetUsage::REFLECTION_CUBE, TextureUsage::REFLECTION_CUBE }
};

class RenderingComponent final : public BaseComponentType<RenderingComponent, ComponentType::RENDERING> {
    friend class Attorney::RenderingCompRenderPass;
    friend class Attorney::RenderingCompGFXDevice;
    friend class Attorney::RenderingCompRenderBin;
    friend class Attorney::RenderingComponentSGN;

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

       struct NodeRenderingProperties {
           U32 _reflectionIndex = 0u;
           U32 _refractionIndex = 0u;
           F32 _cullFlagValue = 1.0f;
           U8 _lod = 0u;
           TextureOperation _texOperation = TextureOperation::REPLACE;
           BumpMethod _bumpMethod = BumpMethod::NONE;
           bool _receivesShadows = false;
           bool _isHovered = false;
           bool _isSelected = false;
       };

   public:
    explicit RenderingComponent(SceneGraphNode& parentSGN,
                                PlatformContext& context);
    ~RenderingComponent();

    void Update(const U64 deltaTimeUS) final;

    void toggleRenderOption(RenderOptions option, bool state);
    bool renderOptionEnabled(RenderOptions option) const;
    bool renderOptionsEnabled(U32 mask) const;

    void setMinRenderRange(F32 minRange);
    void setMaxRenderRange(F32 maxRange);
    inline void setRenderRange(F32 minRange, F32 maxRange) { setMinRenderRange(minRange); setMaxRenderRange(maxRange); }
    inline const vec2<F32>& renderRange() const noexcept { return _renderRange; }

    // If the new value is negative, this disables occlusion culling!
    void cullFlagValue(F32 newValue);

    inline void lockLoD(U8 level) noexcept { _lodLockLevels.fill({ true, level }); }
    inline void unlockLoD() noexcept { _lodLockLevels.fill({ false, to_U8(0u) }); }
    inline void lockLoD(RenderStage stage, U8 level) noexcept { _lodLockLevels[to_base(stage)] = { true, level }; }
    inline void unlockLoD(RenderStage stage, U8 level) noexcept { _lodLockLevels[to_base(stage)] = { false, to_U8(0u) }; }
    inline bool lodLocked(RenderStage stage) const noexcept { return _lodLockLevels[to_base(stage)].first; }

    void useUniqueMaterialInstance();
    void setMaterialTpl(const Material_ptr& material);

    void getMaterialColourMatrix(mat4<F32>& matOut) const;

    void getRenderingProperties(const RenderStagePass& stagePass, NodeRenderingProperties& propertiesOut) const;

    RenderPackage& getDrawPackage(const RenderStagePass& renderStagePass);
    const RenderPackage& getDrawPackage(const RenderStagePass& renderStagePass) const;

    size_t getSortKeyHash(const RenderStagePass& renderStagePass) const;

    inline const Material_ptr& getMaterialInstance() const noexcept { return _materialInstance; }

    void rebuildMaterial();

    inline void setReflectionAndRefractionType(ReflectorType reflectType, RefractorType refractType) noexcept { _reflectorType = reflectType;  _refractorType = refractType; }
    inline void setReflectionCallback(RenderCallback cbk) { _reflectionCallback = cbk; }
    inline void setRefractionCallback(RenderCallback cbk) { _refractionCallback = cbk; }

    void drawDebugAxis();
    void onRender(const RenderStagePass& renderStagePass);

    U8 getLoDLevel(const BoundsComponent& bComp, const vec3<F32>& cameraEye, RenderStage renderStage, const vec4<U16>& lodThresholds);

    inline void addShaderBuffer(const ShaderBufferBinding& binding) { _externalBufferBindings.push_back(binding); }
    inline const vectorEASTL<ShaderBufferBinding>& getShaderBuffers() const noexcept { return _externalBufferBindings; }

  protected:
    bool onRefreshNodeData(RefreshNodeDataParams& refreshParams, const U32 dataIndex);
    bool onQuickRefreshNodeData(RefreshNodeDataParams& refreshParams);

    bool canDraw(const RenderStagePass& renderStagePass, U8 LoD, bool refreshData);

    /// Called after the parent node was rendered
    void postRender(const SceneRenderState& sceneRenderState,
                    const RenderStagePass& renderStagePass,
                    GFX::CommandBuffer& bufferInOut);

    void rebuildDrawCommands(const RenderStagePass& stagePass, RenderPackage& pkg);

    bool prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, bool refreshData);

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
    void onParentUsageChanged(NodeUsageContext context);

   protected:
    using PackagesPerIndex = vectorEASTLFast<RenderPackage>;
    using PackagesPerPassType = std::array<PackagesPerIndex, to_base(RenderPassType::COUNT)>;
    using PackagesPerStage = std::array<PackagesPerPassType, to_base(RenderStage::COUNT)>;
    PackagesPerStage _renderPackages;

    RenderCallback _reflectionCallback;
    RenderCallback _refractionCallback;
    std::array<Texture_ptr, (sizeof(g_texUsage) / sizeof(g_texUsage[0]))> _externalTextures;

    EnvironmentProbeList _envProbes;
    vectorEASTL<ShaderBufferBinding> _externalBufferBindings;

    std::pair<Texture_ptr, U32> _defaultReflection;
    std::pair<Texture_ptr, U32> _defaultRefraction;
    Material_ptr _materialInstance;
    Material* _materialInstanceCache;
    GFXDevice& _context;
    const Configuration& _config;

    vec2<F32> _renderRange;

    Pipeline*    _primitivePipeline[3];
    IMPrimitive* _boundingBoxPrimitive[2];
    IMPrimitive* _boundingSpherePrimitive[2];
    IMPrimitive* _skeletonPrimitive;
    IMPrimitive* _axisGizmo;

    /// used to keep track of what GFXDevice::reflectionTarget we are using for this rendering pass
    I32 _reflectionIndex;
    I32 _refractionIndex;
    F32 _cullFlagValue;
    U32 _renderMask;
    std::array<U8, to_base(RenderStage::COUNT)> _lodLevels;
    ReflectorType _reflectorType;
    RefractorType _refractorType;

    std::array<std::pair<bool, U8>, to_base(RenderStage::COUNT)> _lodLockLevels;

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

        static bool prepareDrawPackage(RenderingComponent& renderable,
                                       const Camera& camera,
                                       const SceneRenderState& sceneRenderState,
                                       RenderStagePass renderStagePass,
                                       bool refreshData) {
            return renderable.prepareDrawPackage(camera, sceneRenderState, renderStagePass, refreshData);
        }

        static bool onRefreshNodeData(RenderingComponent& renderable, RefreshNodeDataParams& refreshParams, const U32 dataIndex) {
            return renderable.onRefreshNodeData(refreshParams, dataIndex);
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

class RenderingComponentSGN {
    static void onParentUsageChanged(RenderingComponent& comp, NodeUsageContext context) {
        comp.onParentUsageChanged(context);
    }
    friend class Divide::SceneGraphNode;
};
};  // namespace Attorney
};  // namespace Divide
#endif