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

TYPEDEF_SMART_POINTERS_FOR_CLASS(Material);

namespace Attorney {
    class RenderingCompRenderPass;
    class RenderingCompGFXDevice;
    class RenderingCompRenderBin;
};

struct RenderParams {
    GenericDrawCommand _cmd;
    Pipeline _pipeline;
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


enum class ReflectorType : U32 {
    PLANAR_REFLECTOR = 0,
    CUBE_REFLECTOR = 1,
    COUNT
};

typedef DELEGATE_CBK<void, RenderCbkParams&, GFX::CommandBuffer&> RenderCallback;

class RenderingComponent : public SGNComponent<RenderingComponent> {
    friend class Attorney::RenderingCompRenderPass;
    friend class Attorney::RenderingCompGFXDevice;
    friend class Attorney::RenderingCompRenderBin;

   public:
       enum class RenderOptions : U32 {
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
    explicit RenderingComponent(GFXDevice& context,
                                Material_ptr materialInstance,
                                SceneGraphNode& parentSGN);
    ~RenderingComponent();

    void onRender(const SceneRenderState& sceneRenderState,
                  const RenderStagePass& renderStagePass);

    void Update(const U64 deltaTimeUS) override;

    inline PushConstants& pushConstants() { return _globalPushConstants; }
    inline const PushConstants& pushConstants() const { return _globalPushConstants; }

    void toggleRenderOption(RenderOptions option, bool state);
    bool renderOptionEnabled(RenderOptions option) const;
    bool renderOptionsEnabled(U32 mask) const;

    inline U32 commandIndex() const { return _commandIndex; }

    inline U32 commandOffset() const { return _commandOffset; }

    ShaderProgram_ptr getDrawShader(const RenderStagePass& renderStagePass);

    void getMaterialColourMatrix(mat4<F32>& matOut) const;

    void getRenderingProperties(vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const;

    RenderPackage& getDrawPackage(const RenderStagePass& renderStagePass);
    const RenderPackage& getDrawPackage(const RenderStagePass& renderStagePass) const;

    size_t getSortKeyHash(const RenderStagePass& renderStagePass) const;

    inline const Material_ptr& getMaterialInstance() const { return _materialInstance; }

    void registerShaderBuffer(ShaderBufferLocation slot,
                              vec2<U32> bindRange,
                              ShaderBuffer& shaderBuffer);

    void unregisterShaderBuffer(ShaderBufferLocation slot);

    void rebuildMaterial();

    void registerTextureDependency(const TextureData& additionalTexture, U8 binding);
    void removeTextureDependency(U8 binding);
    void removeTextureDependency(const TextureData& additionalTexture);

    inline void setReflectionAndRefractionType(ReflectorType type) { _reflectorType = type; }
    inline void setReflectionCallback(RenderCallback cbk) { _reflectionCallback = cbk; }
    inline void setRefractionCallback(RenderCallback cbk) { _refractionCallback = cbk; }

    void drawDebugAxis();

   protected:
    bool canDraw(const RenderStagePass& renderStagePass);
    void updateLoDLevel(const Camera& camera, const RenderStagePass& renderStagePass);
    size_t getMaterialStateHash(const RenderStagePass& renderStagePass);

    /// Called after the parent node was rendered
    void postRender(const SceneRenderState& sceneRenderState,
                    const RenderStagePass& renderStagePass,
                    GFX::CommandBuffer& bufferInOut);

    void rebuildDrawCommands(const RenderStagePass& stagePass);

    void prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass);

    void setDrawIDs(const RenderStagePass& renderStagePass,
                    U32 cmdOffset,
                    U32 cmdIndex);

    inline void commandIndex(U32 index) { _commandIndex = index; }

    inline void commandOffset(U32 offset) { _commandOffset = offset; }

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
    std::unique_ptr<RenderPackage>& renderData(const RenderStagePass& stagePass);
    const std::unique_ptr<RenderPackage>& renderData(const RenderStagePass& stagePass) const;

   protected:
    GFXDevice& _context;
    Material_ptr _materialInstance;

    /// LOD level is updated at every visibility check
    U8  _lodLevel;  ///<Relative to camera distance
    U32 _commandIndex;
    U32 _commandOffset;
    U32 _renderMask;
    TextureDataContainer _textureDependencies;
    std::array<std::unique_ptr<RenderPackage>, to_base(RenderStage::COUNT)> _renderData[to_base(RenderPassType::COUNT)];
    
    bool _renderPackagesDirty;
    PushConstants _globalPushConstants;

    IMPrimitive* _boundingBoxPrimitive[2];
    IMPrimitive* _boundingSpherePrimitive;
    IMPrimitive* _skeletonPrimitive;

    RenderCallback _reflectionCallback;
    RenderCallback _refractionCallback;

    EnvironmentProbeList _envProbes;
    vectorImpl<Line> _axisLines;
    IMPrimitive* _axisGizmo;

    size_t _depthStateBlockHash;
    size_t _shadowStateBlockHash;

    ReflectorType _reflectorType;
    TextureDataContainer _textureCache;
    ShaderBufferList _shaderBuffersCache;

    ShaderProgram_ptr _previewRenderTargetColour;
    ShaderProgram_ptr _previewRenderTargetDepth;

    static hashMapImpl<U32, GFXDevice::DebugView*> s_debugViews[2];
};

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
};

class RenderingCompGFXDevice {
   private:
    static void prepareDrawPackage(RenderingComponent& renderable,
                                   const Camera& camera,
                                   const SceneRenderState& sceneRenderState,
                                   const RenderStagePass& renderStagePass) {
        renderable.prepareDrawPackage(camera, sceneRenderState, renderStagePass);
    }

    static void setDrawIDs(RenderingComponent& renderable, 
                           const RenderStagePass& renderStagePass,
                           U32 cmdOffset,
                           U32 cmdIndex)
    {
        renderable.setDrawIDs(renderStagePass, cmdOffset, cmdIndex);
    }

    static const RenderPackage& getDrawPackage(RenderingComponent& renderable,
                                               const RenderStagePass& renderStagePass) {
        return renderable.getDrawPackage(renderStagePass);
    }


    friend class Divide::GFXDevice;
};

class RenderingCompRenderBin {
   private:
    static const RenderPackage& getRenderData(RenderingComponent& renderable, const RenderStagePass& renderStagePass) {
        return renderable.getDrawPackage(renderStagePass);
    }

    static size_t getSortKeyHash(RenderingComponent& renderable, const RenderStagePass& renderStagePass) {
        return renderable.getSortKeyHash(renderStagePass);
    }

    static void postRender(RenderingComponent& renderable,
                           const SceneRenderState& sceneRenderState,
                           const RenderStagePass& renderStagePass,
                           GFX::CommandBuffer& bufferInOut) {
        renderable.postRender(sceneRenderState, renderStagePass, bufferInOut);
    }

    friend class Divide::RenderBin;
    friend struct Divide::RenderBinItem;
};

};  // namespace Attorney
};  // namespace Divide
#endif