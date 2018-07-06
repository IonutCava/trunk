/*
Copyright (c) 2017 DIVIDE-Studio
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
class ShaderProgram;
class SceneGraphNode;
class ParticleEmitter;

TYPEDEF_SMART_POINTERS_FOR_CLASS(Material);

namespace Attorney {
    class RenderingCompRenderPass;
    class RenderingCompGFXDevice;
    class RenderingCompRenderBin;
    class RenderingCompSceneNode;
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

typedef DELEGATE_CBK<void, RenderCbkParams&> RenderCallback;

class RenderingComponent : public SGNComponent {
    friend class Attorney::RenderingCompRenderPass;
    friend class Attorney::RenderingCompGFXDevice;
    friend class Attorney::RenderingCompRenderBin;
    friend class Attorney::RenderingCompSceneNode;

   public:
    bool onRender(const RenderStagePass& renderStagePass) override;
    void update(const U64 deltaTime) override;

    void setActive(const bool state) override;
    void postLoad() override;

    void renderGeometry(const bool state);
    void renderWireframe(const bool state);
    void renderBoundingBox(const bool state);
    void renderBoundingSphere(const bool state);
    void renderSkeleton(const bool state);

    inline bool renderGeometry() const { return _renderGeometry; }
    inline bool renderWireframe() const { return _renderWireframe; }
    inline bool renderBoundingBox() const { return _renderBoundingBox; }
    inline bool renderBoundingSphere() const { return _renderBoundingSphere; }
    inline bool renderSkeleton() const { return _renderSkeleton; }


    inline bool isVisible() const { return _isVisible; }
    inline void setVisible(bool state) { _isVisible = state; }

    U32 renderMask(U32 baseMask) const;

    void castsShadows(const bool state);
    void receivesShadows(const bool state);

    bool castsShadows() const;
    bool receivesShadows() const;
    
    inline U32 drawOrder() const { return _drawOrder; }

    inline U32 commandIndex() const { return _commandIndex; }

    inline U32 commandOffset() const { return _commandOffset; }

    ShaderProgram_ptr getDrawShader(const RenderStagePass& renderStagePass);

    size_t getDrawStateHash(const RenderStagePass& renderStagePass);

    void getMaterialColourMatrix(mat4<F32>& matOut) const;

    void getRenderingProperties(vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const;

    inline const Material_ptr& getMaterialInstance() const { return _materialInstance; }

    void registerShaderBuffer(ShaderBufferLocation slot,
                              vec2<U32> bindRange,
                              ShaderBuffer& shaderBuffer);

    void unregisterShaderBuffer(ShaderBufferLocation slot);

    void rebuildMaterial();

    void registerTextureDependency(const TextureData& additionalTexture);
    void removeTextureDependency(const TextureData& additionalTexture);

    inline void setReflectionAndRefractionType(ReflectorType type) { _reflectorType = type; }
    inline void setReflectionCallback(RenderCallback cbk) { _reflectionCallback = cbk; }
    inline void setRefractionCallback(RenderCallback cbk) { _refractionCallback = cbk; }

    void drawDebugAxis();

   protected:
    friend class SceneGraphNode;
    explicit RenderingComponent(GFXDevice& context,
                                Material_ptr materialInstance,
                                SceneGraphNode& parentSGN);
    ~RenderingComponent();

   protected:
    bool canDraw(const RenderStagePass& renderStagePass);
    void updateLoDLevel(const Camera& camera, const RenderStagePass& renderStagePass);

    /// Called after the parent node was rendered
    void postRender(const SceneRenderState& sceneRenderState,
                    const RenderStagePass& renderStagePass,
                    RenderSubPassCmds& subPassesInOut);

    void prepareDrawPackage(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass);

    RenderPackage& getDrawPackage(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass);

    RenderPackage& getDrawPackage(const RenderStagePass& renderStagePass);


    inline void drawOrder(U32 index) { _drawOrder = index; }

    inline void commandIndex(U32 index) { _commandIndex = index; }

    inline void commandOffset(U32 offset) { _commandOffset = offset; }

    // This returns false if the node is not reflective, otherwise it generates a new reflection cube map
    // and saves it in the appropriate material slot
    bool updateReflection(U32 reflectionIndex, 
                          Camera* camera,
                          const SceneRenderState& renderState);
    bool updateRefraction(U32 refractionIndex,
                          Camera* camera,
                          const SceneRenderState& renderState);
    bool clearReflection();
    bool clearRefraction();

    void updateEnvProbeList(const EnvironmentProbeList& probes);

    inline RenderPackage& renderData(const RenderStagePass& stagePass) {
        return _renderData[to_U32(stagePass.pass())][to_U32(stagePass.stage())];
    }

   protected:
    GFXDevice& _context;
    Material_ptr _materialInstance;

    /// LOD level is updated at every visibility check
    U8  _lodLevel;  ///<Relative to camera distance
    U32 _drawOrder;
    U32 _commandIndex;
    U32 _commandOffset;
    bool _preDrawPass;
    bool _castsShadows;
    bool _receiveShadows;
    bool _renderGeometry;
    bool _renderWireframe;
    bool _renderBoundingBox;
    bool _renderBoundingSphere;
    bool _renderSkeleton;
    bool _isVisible;
    TextureDataContainer _textureDependencies;
    std::array<RenderPackage, to_base(RenderStage::COUNT)> _renderData[to_base(RenderPassType::COUNT)];
    
    IMPrimitive* _boundingBoxPrimitive[2];
    IMPrimitive* _boundingSpherePrimitive;
    IMPrimitive* _skeletonPrimitive;

    RenderCallback _reflectionCallback;
    RenderCallback _refractionCallback;

    EnvironmentProbeList _envProbes;
    vectorImpl<Line> _axisLines;
    IMPrimitive* _axisGizmo;

    ReflectorType _reflectorType;
    hashMapImpl<U32, GFXDevice::DebugView_ptr> _debugViews[2];
    ShaderProgram_ptr _previewRenderTargetColour;
    ShaderProgram_ptr _previewRenderTargetDepth;
};

namespace Attorney {
class RenderingCompRenderPass {
    private:
        static bool updateReflection(RenderingComponent& renderable,
                                     U32 reflectionIndex,
                                     Camera* camera,
                                     const SceneRenderState& renderState)
        {
            return renderable.updateReflection(reflectionIndex, camera, renderState);
        }

        static bool updateRefraction(RenderingComponent& renderable,
                                     U32 refractionIndex,
                                     Camera* camera,
                                     const SceneRenderState& renderState)
        {
            return renderable.updateRefraction(refractionIndex, camera, renderState);
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

class RenderingCompSceneNode {
    private:
        static RenderPackage& getDrawPackage(RenderingComponent& renderable, const RenderStagePass& renderStagePass) {
            return renderable.getDrawPackage(renderStagePass);
        }

    friend class Divide::Sky;
    friend class Divide::SubMesh;
    friend class Divide::WaterPlane;
    friend class Divide::ParticleEmitter;
};

class RenderingCompGFXDevice {
   private:
    static void prepareDrawPackage(RenderingComponent& renderable,
                                             const SceneRenderState& sceneRenderState,
                                             const RenderStagePass& renderStagePass) {
        renderable.prepareDrawPackage(sceneRenderState, renderStagePass);
    }

    static RenderPackage& getDrawPackage(RenderingComponent& renderable,
                                         const SceneRenderState& sceneRenderState,
                                         const RenderStagePass& renderStagePass,
                                         U32 cmdOffset,
                                         U32 cmdIndex) {
        renderable.commandIndex(cmdIndex);
        renderable.commandOffset(cmdOffset);
        return renderable.getDrawPackage(sceneRenderState, renderStagePass);
    }

    friend class Divide::GFXDevice;
};

class RenderingCompRenderBin {
   private:
    static const RenderPackage& getRenderData(RenderingComponent& renderable, const RenderStagePass& renderStagePass) {
        return renderable.renderData(renderStagePass);
    }

    static void postRender(RenderingComponent& renderable,
                           const SceneRenderState& sceneRenderState,
                           const RenderStagePass& renderStagePass,
                           RenderSubPassCmds& subPassesInOut) {
        renderable.postRender(sceneRenderState, renderStagePass, subPassesInOut);
    }

    static void drawOrder(RenderingComponent& renderable, U32 index) {
        renderable.drawOrder(index);
    }

    friend class Divide::RenderBin;
};

};  // namespace Attorney
};  // namespace Divide
#endif