#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"
#include "Headers/BoundsComponent.h"
#include "Headers/AnimationComponent.h"
#include "Headers/TransformComponent.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"

namespace Divide {

namespace {
    constexpr I16 g_renderRangeLimit = std::numeric_limits<I16>::max();

    I32 getUsageIndex(RenderTargetUsage usage) {
        for (I32 i = 0; i < (sizeof(g_texUsage) / sizeof(g_texUsage[0])); ++i) {
            if (g_texUsage[i].first == usage) {
                return i;
            }
        }

        return -1;
    }
};

hashMap<U32, DebugView*> RenderingComponent::s_debugViews[2];

RenderingComponent::RenderingComponent(SceneGraphNode& parentSGN,
                                       PlatformContext& context)
    : BaseComponentType<RenderingComponent, ComponentType::RENDERING>(parentSGN, context),
      _context(context.gfx()),
      _config(context.config()),
      _lodLocked(false),
      _cullFlagValue(1.0f),
      _renderMask(0),
      _dataIndex({-1, false}),
      _reflectionIndex(-1),
      _refractionIndex(-1),
      _reflectorType(ReflectorType::PLANAR_REFLECTOR),
      _materialInstance(nullptr),
      _materialInstanceCache(nullptr),
      _skeletonPrimitive(nullptr)
{
    _lodLevels.fill(0u);

    _renderRange.min = -1.0f * g_renderRangeLimit;
    _renderRange.max =  1.0f* g_renderRangeLimit;

    _materialInstance = parentSGN.getNode().getMaterialTpl();

    toggleRenderOption(RenderOptions::RENDER_GEOMETRY, true);
    toggleRenderOption(RenderOptions::CAST_SHADOWS, true);
    toggleRenderOption(RenderOptions::RECEIVE_SHADOWS, true);
    toggleRenderOption(RenderOptions::IS_VISIBLE, true);

    defaultReflectionTexture(nullptr, 0);
    defaultRefractionTexture(nullptr, 0);

    // Do not cull the sky
    if (_parentSGN.getNode<Object3D>().type() == SceneNodeType::TYPE_SKY) {
        _cullFlagValue = -1.0f;
    }

    const Object3D& node = parentSGN.getNode<Object3D>();

    bool nodeSkinned = node.getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

    if (_materialInstance != nullptr) {
        assert(!_materialInstance->resourceName().empty());

        EditorComponentField materialField = {};
        materialField._name = "Material";
        materialField._data = _materialInstance.get();
        materialField._type = EditorComponentFieldType::MATERIAL;
        materialField._readOnly = false;

        _editorComponent.registerField(std::move(materialField));

        _materialInstanceCache = _materialInstance.get();
    }

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;
    PipelineDescriptor pipelineDescriptor;

    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();
    Pipeline* pipeline = _context.newPipeline(pipelineDescriptor);

    _boundingBoxPrimitive[0] = _context.newIMP();
    _boundingBoxPrimitive[0]->name("BoundingBox_" + parentSGN.name());
    _boundingBoxPrimitive[0]->pipeline(*pipeline);
    _boundingBoxPrimitive[0]->skipPostFX(true);

    _boundingBoxPrimitive[1] = _context.newIMP();
    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + parentSGN.name());
    _boundingBoxPrimitive[1]->pipeline(*pipeline);
    _boundingBoxPrimitive[1]->skipPostFX(true);

    _boundingSpherePrimitive = _context.newIMP();
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.name());
    _boundingSpherePrimitive->pipeline(*pipeline);
    _boundingSpherePrimitive->skipPostFX(true);

    if (nodeSkinned) {
        RenderStateBlock primitiveStateBlockNoZRead;
        primitiveStateBlockNoZRead.depthTestEnabled(false);
        pipelineDescriptor._stateHash = primitiveStateBlockNoZRead.getHash();
        Pipeline* pipelineNoZRead = _context.newPipeline(pipelineDescriptor);

        _skeletonPrimitive = _context.newIMP();
        _skeletonPrimitive->skipPostFX(true);
        _skeletonPrimitive->name("Skeleton_" + parentSGN.name());
        _skeletonPrimitive->pipeline(*pipelineNoZRead);
    }
    
    if (Config::Build::ENABLE_EDITOR) {
        Line temp;
        temp.widthStart(10.0f);
        temp.widthEnd(10.0f);
        temp.pointStart(VECTOR3_ZERO);

        // Red X-axis
        temp.pointEnd(WORLD_X_AXIS * 4);
        temp.colourStart(UColour4(255, 0, 0, 255));
        temp.colourEnd(UColour4(255, 0, 0, 255));
        _axisLines.push_back(temp);

        // Green Y-axis
        temp.pointEnd(WORLD_Y_AXIS * 4);
        temp.colourStart(UColour4(0, 255, 0, 255));
        temp.colourEnd(UColour4(0, 255, 0, 255));
        _axisLines.push_back(temp);

        // Blue Z-axis
        temp.pointEnd(WORLD_Z_AXIS * 4);
        temp.colourStart(UColour4(0, 0, 255, 255));
        temp.colourEnd(UColour4(0, 0, 255, 255));
        _axisLines.push_back(temp);

        _axisGizmo = _context.newIMP();
        // Prepare it for line rendering
        size_t noDepthStateBlock = _context.getDefaultStateBlock(true);
        RenderStateBlock stateBlock(RenderStateBlock::get(noDepthStateBlock));

        pipelineDescriptor._stateHash = stateBlock.getHash();
        _axisGizmo->name("AxisGizmo_" + parentSGN.name());
        _axisGizmo->skipPostFX(true);
        _axisGizmo->pipeline(*_context.newPipeline(pipelineDescriptor));
        // Create the object containing all of the lines
        _axisGizmo->beginBatch(true, to_U32(_axisLines.size()) * 2, 1);
        _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(_axisLines[0].colourStart()));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : _axisLines) {
            _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line.colourStart()));
            _axisGizmo->vertex(line.pointStart());
            _axisGizmo->vertex(line.pointEnd());
        }
        _axisGizmo->end();
        // Finish our object
        _axisGizmo->endBatch();
    } else {
        _axisGizmo = nullptr;
    }
}

RenderingComponent::~RenderingComponent()
{
    _boundingBoxPrimitive[0]->reset();
    _boundingBoxPrimitive[1]->reset();
    _boundingSpherePrimitive->reset();

    if (_skeletonPrimitive != nullptr) {
        _skeletonPrimitive->reset();
    }

    if (Config::Build::ENABLE_EDITOR) {
        _axisGizmo->reset();
    }
}

void RenderingComponent::useUniqueMaterialInstance() {
    if (_materialInstance == nullptr) {
        return;
    }

    _materialInstance = _materialInstance->clone("_instance_" + _parentSGN.name());
    _materialInstanceCache = _materialInstance.get();
}

void RenderingComponent::setMinRenderRange(F32 minRange) {
    _renderRange.min = std::max(minRange, -1.0f * g_renderRangeLimit);
}

void RenderingComponent::setMaxRenderRange(F32 maxRange) {
    _renderRange.max = std::min(maxRange,  1.0f * g_renderRangeLimit);
}

void RenderingComponent::rebuildDrawCommands(RenderStagePass stagePass) {
    RenderPackage& pkg = getDrawPackage(stagePass);
    pkg.clear();

    // The following commands are needed for material rendering
    // In the absence of a material, use the SceneNode buildDrawCommands to add all of the needed commands
    if (getMaterialCache() != nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = getMaterialCache()->getRenderStateBlock(stagePass);
        pipelineDescriptor._shaderProgramHandle = getMaterialCache()->getProgramID(stagePass);

        GFX::BindPipelineCommand pipelineCommand = {};
        pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
        pkg.addPipelineCommand(pipelineCommand);

        GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand = {};
        for (const ShaderBufferBinding& binding : getShaderBuffers()) {
            bindDescriptorSetsCommand._set.addShaderBuffer(binding);
        }
        pkg.addDescriptorSetsCommand(bindDescriptorSetsCommand);

        if (!_globalPushConstants.empty()) {
            GFX::SendPushConstantsCommand pushConstantsCommand = {};
            pushConstantsCommand._constants = _globalPushConstants;
            pkg.addPushConstantsCommand(pushConstantsCommand);
        }
    }

    _parentSGN.getNode().buildDrawCommands(_parentSGN, stagePass, pkg);
}

void RenderingComponent::Update(const U64 deltaTimeUS) {
    if (getMaterialCache() != nullptr) {
        getMaterialCache()->update(deltaTimeUS);
    }

    const Object3D& node = _parentSGN.getNode<Object3D>();
    if (node.getObjectType()._value == ObjectType::SUBMESH) {
        StateTracker<bool>& parentStates = _parentSGN.getParent()->getTrackedBools();
        parentStates.setTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED, false);

        if (node.getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            parentStates.setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, false);
        }
    }

    BaseComponentType<RenderingComponent, ComponentType::RENDERING>::Update(deltaTimeUS);
}

void RenderingComponent::FrameEnded() {
    SGNComponent::FrameEnded();
}

bool RenderingComponent::canDraw(RenderStagePass renderStagePass, U8 LoD, bool refreshData) {
    if (_parentSGN.getDrawState(renderStagePass, LoD)) {
        Material* matCache = getMaterialCache();
        // Can we render without a material? Maybe. IDK.
        if (matCache == nullptr || matCache->canDraw(renderStagePass)) {
            return renderOptionEnabled(RenderOptions::IS_VISIBLE);
        }
    }

    return false;
}

void RenderingComponent::rebuildMaterial() {
    if (getMaterialCache() != nullptr) {
        getMaterialCache()->rebuild();
    }

    _parentSGN.forEachChild([](const SceneGraphNode* child) {
        RenderingComponent* const renderable = child->get<RenderingComponent>();
        if (renderable) {
            renderable->rebuildMaterial();
        }
    });
}

void RenderingComponent::onRender(RenderStagePass renderStagePass, bool refreshData) {
    RenderPackage& pkg = getDrawPackage(renderStagePass);
    TextureDataContainer& textures = pkg.descriptorSet(0)._textureData;

    if (getMaterialCache() != nullptr) {
        getMaterialCache()->getTextureData(renderStagePass, textures);
    }

    for (U8 i = 0; i < _externalTextures.size(); ++i) {
        const Texture_ptr& crtTexture = _externalTextures[i];
        if (crtTexture != nullptr) {
            textures.setTexture(crtTexture->data(), to_base(g_texUsage[i].second));
        }
    }
}

void RenderingComponent::setDataIndex(U32 idx) {
    _dataIndex.first = to_I64(idx);
    _dataIndex.second = true;
}

bool RenderingComponent::getDataIndex(U32& idxOut) {
    idxOut = to_U32(_dataIndex.first);
    return _dataIndex.second;
}

bool RenderingComponent::onQuickRefreshNodeData(RefreshNodeDataParams& refreshParams) {
    _parentSGN.onRefreshNodeData(refreshParams._stagePass, *refreshParams._camera, true, refreshParams._bufferInOut);
    return true;
}

bool RenderingComponent::onRefreshNodeData(RefreshNodeDataParams& refreshParams) {
    RenderPackage& pkg = getDrawPackage(refreshParams._stagePass);
    I32 drawCommandCount = pkg.drawCommandCount();

    if (drawCommandCount > 0) {
        if (!_dataIndex.second) {
            _dataIndex.first = to_I64(refreshParams._dataIdx);
        }

        if (refreshParams._stagePass._stage == RenderStage::SHADOW) {
            Attorney::RenderPackageRenderingComponent::updateDrawCommands(pkg, refreshParams._dataIdx, to_U32(refreshParams._drawCommandsInOut.size()));
        } else {
            RenderPackagesPerPassType& packages = _renderPackagesNormal[to_base(refreshParams._stagePass._stage) - 1];
            for (RenderPackage& package : packages) {
                Attorney::RenderPackageRenderingComponent::updateDrawCommands(package, refreshParams._dataIdx, to_U32(refreshParams._drawCommandsInOut.size()));
            }
        }
        for (I32 i = 0; i < drawCommandCount; ++i) {
            const GenericDrawCommand& cmd = pkg.drawCommand(i, 0);
            if (cmd._drawCount > 1 && isEnabledOption(cmd, CmdRenderOptions::CONVERT_TO_INDIRECT)) {
                std::fill_n(std::back_inserter(refreshParams._drawCommandsInOut), cmd._drawCount, cmd._cmd);
            } else {
                refreshParams._drawCommandsInOut.push_back(cmd._cmd);
            }
        }

        _parentSGN.onRefreshNodeData(refreshParams._stagePass, *refreshParams._camera, false, refreshParams._bufferInOut);
        return true;
    }

    return false;
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    if (getMaterialCache() != nullptr) {
        getMaterialCache()->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(RenderStagePass& stagePass, vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const {
    const bool shadowMappingEnabled = _config.rendering.shadowMapping.enabled;

    propertiesOut.set(_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTED
                                                     ? -1.0f
                                                     : _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::HOVER
                                                                                      ? 1.0f
                                                                                      : 0.0f,
                      (shadowMappingEnabled && renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS)) ? 1.0f : 0.0f,
                      to_F32(getDrawPackage(stagePass).lodLevel()),
                      _cullFlagValue);

    reflectionIndex = to_F32(defaultReflectionTextureIndex());
    refractionIndex = to_F32(defaultRefractionTextureIndex());
}

/// Called after the current node was rendered
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass, GFX::CommandBuffer& bufferInOut) {
    
    if (renderStagePass._stage != RenderStage::DISPLAY || renderStagePass._passType == RenderPassType::PRE_PASS) {
        return;
    }

    const SceneNode& node = _parentSGN.getNode();

    if (Config::Build::IS_DEBUG_BUILD) {
        switch(sceneRenderState.gizmoState()){
            case SceneRenderState::GizmoState::ALL_GIZMO: {
                if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
                    if (_parentSGN.getNode<Object3D>().getObjectType()._value == ObjectType::SUBMESH) {
                        drawDebugAxis();
                        bufferInOut.add(_axisGizmo->toCommandBuffer());
                    }
                }
            } break;
            case SceneRenderState::GizmoState::SELECTED_GIZMO: {
                switch (_parentSGN.getSelectionFlag()) {
                    case SceneGraphNode::SelectionFlag::SELECTED : {
                        drawDebugAxis();
                        bufferInOut.add(_axisGizmo->toCommandBuffer());
                    } break;
                }
            } break;
        }
    }

    SceneGraphNode* grandParent = _parentSGN.getParent();
    StateTracker<bool>& parentStates = grandParent->getTrackedBools();

    // Draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    bool renderBBox = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB);
    renderBBox = renderBBox || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);

    bool renderBSphere = _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTED;
    renderBSphere = renderBSphere || renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE);


    if (renderBBox) {
        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive[0]->fromBox(bb.getMin(), bb.getMax(), UColour4(0, 0, 255, 255));
        bufferInOut.add(_boundingBoxPrimitive[0]->toCommandBuffer());

        bool isSubMesh = _parentSGN.getNode<Object3D>().getObjectType()._value == ObjectType::SUBMESH;
        if (isSubMesh) {
            bool renderParentBBFlagInitialized = false;
            bool renderParentBB = parentStates.getTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED,
                                   renderParentBBFlagInitialized);
            if (!renderParentBB || !renderParentBBFlagInitialized) {
                const BoundingBox& bbGrandParent = grandParent->get<BoundsComponent>()->getBoundingBox();
                _boundingBoxPrimitive[1]->fromBox(
                                     bbGrandParent.getMin() - vec3<F32>(0.0025f),
                                     bbGrandParent.getMax() + vec3<F32>(0.0025f),
                                     UColour4(255, 0, 0, 255));
                bufferInOut.add(_boundingBoxPrimitive[1]->toCommandBuffer());
            }
        }

        if (renderBSphere) {
            const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
            _boundingSpherePrimitive->fromSphere(bs.getCenter(), bs.getRadius(), UColour4(0, 255, 0, 255));
            bufferInOut.add(_boundingSpherePrimitive->toCommandBuffer());
        }
    }



    bool renderSkeleton = renderOptionEnabled(RenderOptions::RENDER_SKELETON);
    renderSkeleton = renderSkeleton || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);

    if (renderSkeleton) {
        // Continue only for skinned 3D objects
        if (_parentSGN.getNode<Object3D>().getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))
        {
            //bool renderSkeletonFlagInitialized = false;
            //bool renderParentSkeleton = parentStates.getTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, renderSkeletonFlagInitialized);
            //if (!renderParentSkeleton || !renderSkeletonFlagInitialized) {
                // Get the animation component of any submesh. They should be synced anyway.
                AnimationComponent* childAnimComp = _parentSGN.get<AnimationComponent>();
                // Get the skeleton lines from the submesh's animation component
                const vector<Line>& skeletonLines = childAnimComp->skeletonLines();
                _skeletonPrimitive->worldMatrix(_parentSGN.get<TransformComponent>()->getWorldMatrix());
                // Submit the skeleton lines to the GPU for rendering
                _skeletonPrimitive->fromLines(skeletonLines);
                parentStates.setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, true);


                bufferInOut.add(_skeletonPrimitive->toCommandBuffer());
            //}
        }
    }
}

U8 RenderingComponent::getLoDLevel(const Camera& camera, RenderStage renderStage, const vec4<U16>& lodThresholds) {
    U8 lodLevel = 0u;

    if (_lodLocked) {
        return lodLevel;
    }

    const vec3<F32>& eyePos = camera.getEye();
    BoundsComponent* bounds = _parentSGN.get<BoundsComponent>();

    const BoundingSphere& bSphere = bounds->getBoundingSphere();
    if (bSphere.getCenter().distanceSquared(eyePos) <= SQUARED(lodThresholds.x)) {
        return lodLevel;
    }

    lodLevel += 1;

    const F32 cameraDistanceSQ = bounds->getBoundingBox().nearestDistanceFromPointSquared(eyePos);
    if (cameraDistanceSQ > SQUARED(lodThresholds.y)) {
        lodLevel += 1;
        if (cameraDistanceSQ > SQUARED(lodThresholds.z)) {
            lodLevel += 1;
            if (cameraDistanceSQ > SQUARED(lodThresholds.w)) {
                lodLevel += 1;
            }
        }
    }

    // ToDo: Hack for lower LoD rendering in reflection and refraction passes
    if (lodLevel < 4 && (renderStage == RenderStage::REFLECTION || renderStage == RenderStage::REFRACTION)) {
        lodLevel += 1;
    }

    return lodLevel;
}

void RenderingComponent::queueRebuildCommands(RenderStagePass renderStagePass) {
    getDrawPackage(renderStagePass).clear();
}

void RenderingComponent::prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass, bool refreshData) {
    U8& lod = _lodLevels[to_base(renderStagePass._stage)];
    if (refreshData) {
        lod = getLoDLevel(camera, renderStagePass._stage, sceneRenderState.lodThresholds());
    }

    if (canDraw(renderStagePass, lod, refreshData)) {
        RenderPackage& pkg = getDrawPackage(renderStagePass);

        bool rebuildCommandsOut = false;
        if (_parentSGN.preRender(camera, renderStagePass, refreshData, rebuildCommandsOut)) {
            if (pkg.empty() || rebuildCommandsOut) {
                rebuildDrawCommands(renderStagePass);
            }
            if (_parentSGN.prepareRender(camera, renderStagePass, refreshData)) {

                Attorney::RenderPackageRenderingComponent::setLoDLevel(pkg, lod);

                bool renderGeometry = renderOptionEnabled(RenderOptions::RENDER_GEOMETRY);
                renderGeometry = renderGeometry || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);

                bool renderWireframe = renderOptionEnabled(RenderOptions::RENDER_WIREFRAME);
                renderWireframe = renderWireframe || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);
                if (renderWireframe && renderGeometry) {
                    pkg.enableOptions(to_base(CmdRenderOptions::RENDER_GEOMETRY) | to_base(CmdRenderOptions::RENDER_WIREFRAME));
                } else if (!renderWireframe && !renderGeometry) {
                    pkg.disableOptions(to_base(CmdRenderOptions::RENDER_GEOMETRY) | to_base(CmdRenderOptions::RENDER_WIREFRAME));
                } else {
                    pkg.setDrawOption(CmdRenderOptions::RENDER_GEOMETRY, renderGeometry);
                    pkg.setDrawOption(CmdRenderOptions::RENDER_WIREFRAME, renderWireframe);
                }
            }
        }
    }
}

RenderPackage& RenderingComponent::getDrawPackage(RenderStagePass renderStagePass) {
    if (renderStagePass._stage == RenderStage::SHADOW) {
        constexpr U32 stride = std::max(to_U32(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS), 6u);

        U32 passIndex = renderStagePass._passIndex % stride;
        U32 lightIndex = (renderStagePass._passIndex - passIndex) / stride;
        return _renderPackagesShadow[lightIndex][passIndex];
    } else {
        return _renderPackagesNormal[to_base(renderStagePass._stage) - 1][to_base(renderStagePass._passType)];
    }
}

const RenderPackage& RenderingComponent::getDrawPackage(RenderStagePass renderStagePass) const {
    if (renderStagePass._stage == RenderStage::SHADOW) {
        constexpr U32 stride = std::max(to_U32(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS), 6u);

        U32 passIndex = renderStagePass._passIndex % stride;
        U32 lightIndex = (renderStagePass._passIndex - passIndex) / stride;
        return _renderPackagesShadow[lightIndex][passIndex];
    } else {
        return _renderPackagesNormal[to_base(renderStagePass._stage) - 1][to_base(renderStagePass._passType)];
    }
}


size_t RenderingComponent::getSortKeyHash(RenderStagePass renderStagePass) const {
    return getDrawPackage(renderStagePass).getSortKeyHash();
}

void RenderingComponent::updateReflectionIndex(ReflectorType type, I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget =
            _context.renderTargetPool().renderTarget(RenderTargetID(type == ReflectorType::PLANAR_REFLECTOR
                ? RenderTargetUsage::REFLECTION_PLANAR
                : RenderTargetUsage::REFLECTION_CUBE,
                to_U16(index)));
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR_REFLECTOR
                                             ? RenderTargetUsage::REFLECTION_PLANAR
                                             : RenderTargetUsage::REFLECTION_CUBE)] = refTex;
    } else {
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR_REFLECTOR
                                              ? RenderTargetUsage::REFLECTION_PLANAR
                                              : RenderTargetUsage::REFLECTION_CUBE)] = _defaultReflection.first;
    }
}

bool RenderingComponent::clearReflection() {
    updateReflectionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateReflection(U16 reflectionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut)
{
    updateReflectionIndex(_reflectorType, reflectionIndex);

    RenderTargetID reflectRTID(_reflectorType == ReflectorType::PLANAR_REFLECTOR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                                 : RenderTargetUsage::REFLECTION_CUBE, 
                               reflectionIndex);

    if (Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(reflectRTID);

        DebugView* debugView = s_debugViews[0][reflectionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            viewPtr->_name = Util::StringFormat("REFLECTION_%d", reflectRTID._index);
            debugView = _context.addDebugView(viewPtr);
            s_debugViews[0][reflectionIndex] = debugView;
        } else {
            /*if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (debugView->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(true);
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(false);
                }
            }*/
        }
    }

    if (_reflectionCallback) {
        RenderCbkParams params(_context, _parentSGN, renderState, reflectRTID, reflectionIndex, camera);
        _reflectionCallback(params, bufferInOut);
    } else {
        if (_reflectorType == ReflectorType::CUBE_REFLECTOR) {
            const vec2<F32>& zPlanes = camera->getZPlanes();
            _context.generateCubeMap(reflectRTID,
                                     0,
                                     camera->getEye(),
                                     vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                     RenderStagePass(RenderStage::REFLECTION, RenderPassType::MAIN_PASS),
                                     reflectionIndex,
                                     bufferInOut);
        }
    }

    return true;
}

void RenderingComponent::updateRefractionIndex(ReflectorType type, I32 index) {
    _refractionIndex = index;
    if (_refractionIndex > -1) {
        RenderTarget& refractionTarget =
            _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::REFRACTION_PLANAR, to_U16(index)));
        const Texture_ptr& refTex = refractionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
        _externalTextures[getUsageIndex(RenderTargetUsage::REFRACTION_PLANAR)] = refTex;
    } else {
        _externalTextures[getUsageIndex(RenderTargetUsage::REFRACTION_PLANAR)] = _defaultRefraction.first;
    }
}

bool RenderingComponent::clearRefraction() {
    updateRefractionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateRefraction(U16 refractionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut) {
    // no default refraction system!
    if (!_refractionCallback) {
        return false;
    }

    updateRefractionIndex(_reflectorType, refractionIndex);

    RenderTargetID refractRTID(RenderTargetUsage::REFRACTION_PLANAR, refractionIndex);

    if (Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(refractRTID);

        DebugView* debugView = s_debugViews[1][refractionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            viewPtr->_name = Util::StringFormat("REFRACTION_%d", refractRTID._index);
            debugView = _context.addDebugView(viewPtr);
            s_debugViews[1][refractionIndex] = debugView;
        } else {
            /*if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (debugView->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(true);
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(false);
                }
            } */
        }
    }

    RenderCbkParams params(_context, _parentSGN, renderState, refractRTID, refractionIndex, camera);
    _refractionCallback(params, bufferInOut);

    return false;
}

U32 RenderingComponent::defaultReflectionTextureIndex() const {
    return _reflectionIndex > -1 ? to_U32(_reflectionIndex) : _defaultReflection.second;
}

U32 RenderingComponent::defaultRefractionTextureIndex() const {
    return _refractionIndex > -1 ? to_U32(_refractionIndex) : _defaultRefraction.second;
}

void RenderingComponent::defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex) {
    _defaultReflection.first = reflectionPtr;
    _defaultReflection.second = arrayIndex;
}

void RenderingComponent::defaultRefractionTexture(const Texture_ptr& refractionPtr, U32 arrayIndex) {
    _defaultRefraction.first = refractionPtr;
    _defaultRefraction.second = arrayIndex;
}

void RenderingComponent::updateEnvProbeList(const EnvironmentProbeList& probes) {
    _envProbes.resize(0);
    if (probes.empty()) {
        return;
    }

    _envProbes.insert(std::cend(_envProbes), std::cbegin(probes), std::cend(probes));

    TransformComponent* const transform = _parentSGN.get<TransformComponent>();
    if (transform) {
        const vec3<F32>& nodePos = transform->getPosition();
        auto sortFunc = [&nodePos](const EnvironmentProbe_ptr& a, const EnvironmentProbe_ptr& b) -> bool {
            return a->distanceSqTo(nodePos) < b->distanceSqTo(nodePos);
        };

        std::sort(std::begin(_envProbes), std::end(_envProbes), sortFunc);
    }

    RenderTarget* rt = EnvironmentProbe::reflectionTarget()._rt;
    defaultReflectionTexture(rt->getAttachment(RTAttachmentType::Colour, 0).texture(), _envProbes.front()->getRTIndex());
}

/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
    if (!Config::Build::IS_DEBUG_BUILD) {
        return;
    }

    TransformComponent* const transform = _parentSGN.get<TransformComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()), false);
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
}

void RenderingComponent::cullFlagValue(F32 newValue) {
    _cullFlagValue = newValue;
}

};
