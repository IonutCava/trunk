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

hashMap<U32, DebugView*> RenderingComponent::s_debugViews[2];

RenderingComponent::RenderingComponent(SceneGraphNode& parentSGN,
                                       PlatformContext& context)
    : BaseComponentType<RenderingComponent, ComponentType::RENDERING>(parentSGN, context),
      _context(context.gfx()),
      _lodLocked(false),
      _renderMask(0),
      _reflectorType(ReflectorType::PLANAR_REFLECTOR),
      _materialInstance(nullptr),
      _materialInstanceCache(nullptr),
      _skeletonPrimitive(nullptr)
{
    const Material_ptr& materialTpl = parentSGN.getNode().getMaterialTpl();
    if (materialTpl) {
        _materialInstance = materialTpl->clone("_instance_" + parentSGN.name());
    }

    toggleRenderOption(RenderOptions::RENDER_GEOMETRY, true);
    toggleRenderOption(RenderOptions::CAST_SHADOWS, true);
    toggleRenderOption(RenderOptions::RECEIVE_SHADOWS, true);
    toggleRenderOption(RenderOptions::IS_VISIBLE, true);
    toggleRenderOption(RenderOptions::IS_OCCLUSION_CULLABLE, _parentSGN.getNode<Object3D>().type() != SceneNodeType::TYPE_SKY);

    const Object3D& node = parentSGN.getNode<Object3D>();

    bool nodeSkinned = node.getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

    if (_materialInstance != nullptr) {
        assert(!_materialInstance->resourceName().empty());
        _editorComponent.registerField("Material", 
                                       _materialInstance.get(),
                                       EditorComponentFieldType::MATERIAL,
                                       false);

        _materialInstance->useTriangleStrip(node.getObjectType()._value != ObjectType::SUBMESH);
        _materialInstanceCache = _materialInstance.get();
    }

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;
    PipelineDescriptor pipelineDescriptor;

    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getID();
    Pipeline* pipeline = _context.newPipeline(pipelineDescriptor);

    _boundingBoxPrimitive[0] = _context.newIMP();
    _boundingBoxPrimitive[0]->name("BoundingBox_" + parentSGN.name());
    _boundingBoxPrimitive[0]->pipeline(*pipeline);

    _boundingBoxPrimitive[1] = _context.newIMP();
    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + parentSGN.name());
    _boundingBoxPrimitive[1]->pipeline(*pipeline);

    _boundingSpherePrimitive = _context.newIMP();
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.name());
    _boundingSpherePrimitive->pipeline(*pipeline);

    if (nodeSkinned) {
        RenderStateBlock primitiveStateBlockNoZRead;
        primitiveStateBlockNoZRead.depthTestEnabled(false);
        pipelineDescriptor._stateHash = primitiveStateBlockNoZRead.getHash();
        Pipeline* pipelineNoZRead = _context.newPipeline(pipelineDescriptor);

        _skeletonPrimitive = _context.newIMP();
        _skeletonPrimitive->name("Skeleton_" + parentSGN.name());
        _skeletonPrimitive->pipeline(*pipelineNoZRead);
    }
    
    if (Config::Build::IS_DEBUG_BUILD) {
        ResourceDescriptor previewReflectionRefractionColour("fbPreview");
        previewReflectionRefractionColour.setThreadedLoading(true);
        _previewRenderTargetColour = CreateResource<ShaderProgram>(context.kernel().resourceCache(), previewReflectionRefractionColour);

        ResourceDescriptor previewReflectionRefractionDepth("fbPreview.LinearDepth.ScenePlanes");
        _previewRenderTargetDepth = CreateResource<ShaderProgram>(context.kernel().resourceCache(), previewReflectionRefractionDepth);

        // Red X-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, UColour(255, 0, 0, 255), 5.0f));
        // Green Y-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, UColour(0, 255, 0, 255), 5.0f));
        // Blue Z-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, UColour(0, 0, 255, 255), 5.0f));
        _axisGizmo = _context.newIMP();
        // Prepare it for line rendering
        size_t noDepthStateBlock = _context.getDefaultStateBlock(true);
        RenderStateBlock stateBlock(RenderStateBlock::get(noDepthStateBlock));

        pipelineDescriptor._stateHash = stateBlock.getHash();
        _axisGizmo->name("AxisGizmo_" + parentSGN.name());
        _axisGizmo->pipeline(*_context.newPipeline(pipelineDescriptor));
        // Create the object containing all of the lines
        _axisGizmo->beginBatch(true, to_U32(_axisLines.size()) * 2, 1);
        _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(_axisLines[0]._colourStart));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : _axisLines) {
            _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line._colourStart));
            _axisGizmo->vertex(line._startPoint);
            _axisGizmo->vertex(line._endPoint);
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
    _boundingBoxPrimitive[0]->clear();
    _boundingBoxPrimitive[1]->clear();
    _boundingSpherePrimitive->clear();

    if (_skeletonPrimitive != nullptr) {
        _skeletonPrimitive->clear();
    }

    if (Config::Build::IS_DEBUG_BUILD) {
        _axisGizmo->clear();
    }
}


void RenderingComponent::rebuildDrawCommands(RenderStagePass stagePass) {
    RenderPackage& pkg = getDrawPackage(stagePass);
    pkg.clear();

    // The following commands are needed for material rendering
    // In the absence of a material, use the SceneNode buildDrawCommands to add all of the needed commands
    if (getMaterialInstanceCache() != nullptr) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = getMaterialInstanceCache()->getRenderStateBlock(stagePass);
        pipelineDescriptor._shaderProgramHandle = getMaterialInstanceCache()->getProgramID(stagePass);

        GFX::BindPipelineCommand pipelineCommand;
        pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
        pkg.addPipelineCommand(pipelineCommand);

        GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand;
        pkg.addDescriptorSetsCommand(bindDescriptorSetsCommand);

        if (!_globalPushConstants.empty()) {
            GFX::SendPushConstantsCommand pushConstantsCommand;
            pushConstantsCommand._constants = _globalPushConstants;
            pkg.addPushConstantsCommand(pushConstantsCommand);
        }
    } else {
        assert(_globalPushConstants.empty());
    }

    _parentSGN.getNode().buildDrawCommands(_parentSGN, stagePass, pkg);
}

void RenderingComponent::Update(const U64 deltaTimeUS) {
    if (getMaterialInstanceCache() != nullptr) {
        getMaterialInstanceCache()->update(deltaTimeUS);
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

bool RenderingComponent::canDraw(RenderStagePass renderStagePass) {
    if (_parentSGN.getDrawState(renderStagePass)) {
        if (getMaterialInstanceCache() != nullptr && !getMaterialInstanceCache()->canDraw(renderStagePass)) {
            return false;
        }
        return renderOptionEnabled(RenderOptions::IS_VISIBLE);
    }

    return false;
}

void RenderingComponent::rebuildMaterial() {
    if (getMaterialInstanceCache() != nullptr) {
        getMaterialInstanceCache()->rebuild();
    }

    _parentSGN.forEachChild([](const SceneGraphNode& child) {
        RenderingComponent* const renderable = child.get<RenderingComponent>();
        if (renderable) {
            renderable->rebuildMaterial();
        }
    });
}

void RenderingComponent::onRender(RenderStagePass renderStagePass) {
    if (getMaterialInstanceCache() != nullptr) {
        RenderPackage& pkg = getDrawPackage(renderStagePass);
        TextureDataContainer& textures = pkg.descriptorSet(0)._textureData;
        getMaterialInstanceCache()->getTextureData(renderStagePass, textures);
    }
}

bool RenderingComponent::onRefreshNodeData(RefreshNodeDataParams& refreshParams) {
    RenderPackage& pkg = getDrawPackage(refreshParams._stagePass);
    I32 drawCommandCount = pkg.drawCommandCount();

    if (drawCommandCount > 0) {
        if (refreshParams._stagePass._stage == RenderStage::SHADOW) {
            Attorney::RenderPackageRenderingComponent::updateDrawCommands(pkg, refreshParams._nodeCount, to_U32(refreshParams._drawCommandsInOut.size()));
        } else {
            RenderPackagesPerPassType& packages = _renderPackagesNormal[to_base(refreshParams._stagePass._stage) - 1];
            for (RenderPackage& package : packages) {
                Attorney::RenderPackageRenderingComponent::updateDrawCommands(package, refreshParams._nodeCount, to_U32(refreshParams._drawCommandsInOut.size()));
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

        _parentSGN.onRefreshNodeData(refreshParams._stagePass, refreshParams._bufferInOut);
        return true;
    }

    return false;
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    if (getMaterialInstanceCache() != nullptr) {
        getMaterialInstanceCache()->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(RenderStagePass& stagePass, vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const {
    const bool shadowMappingEnabled = _context.context().config().rendering.shadowMapping.enabled;

    propertiesOut.set(_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED
                                                     ? -1.0f
                                                     : _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_HOVER
                                                                                      ? 1.0f
                                                                                      : 0.0f,
                      (shadowMappingEnabled && renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS)) ? 1.0f : 0.0f,
                      to_F32(getDrawPackage(stagePass).lodLevel()),
                      0.0f);

    if (getMaterialInstanceCache() != nullptr) {
        reflectionIndex = to_F32(getMaterialInstanceCache()->defaultReflectionTextureIndex());
        refractionIndex = to_F32(getMaterialInstanceCache()->defaultRefractionTextureIndex());
        propertiesOut.w = getMaterialInstanceCache()->hasTransparency() ? 1.0f : 0.0f;
    } else {
        reflectionIndex = refractionIndex = 0.0f;
    }
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
                    case SceneGraphNode::SelectionFlag::SELECTION_SELECTED : {
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

    bool renderBSphere = _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED;
    renderBSphere = renderBSphere || renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE);


    if (renderBBox) {
        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive[0]->fromBox(bb.getMin(), bb.getMax(), UColour(0, 0, 255, 255));
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
                                     UColour(255, 0, 0, 255));
                bufferInOut.add(_boundingBoxPrimitive[1]->toCommandBuffer());
            }
        }

        if (renderBSphere) {
            const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
            _boundingSpherePrimitive->fromSphere(bs.getCenter(), bs.getRadius(), UColour(0, 255, 0, 255));
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
    U8 lodLevel = 0;

    if (_lodLocked) {
        return lodLevel;
    
    }
    const U32 SCENE_NODE_LOD0_SQ = lodThresholds.x * lodThresholds.x;
    const U32 SCENE_NODE_LOD1_SQ = lodThresholds.y * lodThresholds.y;
    const U32 SCENE_NODE_LOD2_SQ = lodThresholds.z * lodThresholds.z;
    const U32 SCENE_NODE_LOD3_SQ = lodThresholds.w * lodThresholds.w;

    const vec3<F32>& eyePos = camera.getEye();
    BoundsComponent* bounds = _parentSGN.get<BoundsComponent>();

    const BoundingSphere& bSphere = bounds->getBoundingSphere();
    F32 cameraDistanceSQ = bSphere.getCenter().distanceSquared(eyePos);

    if (cameraDistanceSQ <= SCENE_NODE_LOD0_SQ) {
        return lodLevel;
    }

    cameraDistanceSQ = bounds->getBoundingBox().nearestDistanceFromPointSquared(eyePos);
    if (cameraDistanceSQ > SCENE_NODE_LOD0_SQ) {
        lodLevel = 1;
        if (cameraDistanceSQ > SCENE_NODE_LOD1_SQ) {
            lodLevel = 2;
            if (cameraDistanceSQ > SCENE_NODE_LOD2_SQ) {
                lodLevel = 3;
                if (cameraDistanceSQ > SCENE_NODE_LOD3_SQ) {
                    lodLevel = 4;
                }
            }
        }
        
    }

    // ToDo: Hack for lower LoD rendering in reflection and refraction passes
    if (renderStage == RenderStage::REFLECTION || renderStage == RenderStage::REFRACTION) {
        lodLevel += 1;
    }

    return lodLevel;
}

void RenderingComponent::prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass) {
    RenderPackage& pkg = getDrawPackage(renderStagePass);
    if (canDraw(renderStagePass)) {
        if (pkg.empty()) {
            rebuildDrawCommands(renderStagePass);
        }

        if (_parentSGN.prepareRender(camera, renderStagePass)) {
            U8 lod = getLoDLevel(camera, renderStagePass._stage, sceneRenderState.lodThresholds());

            Attorney::RenderPackageRenderingComponent::setLoDLevel(pkg, lod);

            bool renderGeometry = renderOptionEnabled(RenderOptions::RENDER_GEOMETRY);
            renderGeometry = renderGeometry || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);

            bool renderWireframe = renderOptionEnabled(RenderOptions::RENDER_WIREFRAME);
            renderWireframe = renderWireframe || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);
            if (renderWireframe && renderGeometry) {
                pkg.enableOptions(to_U32(CmdRenderOptions::RENDER_GEOMETRY) | to_U32(CmdRenderOptions::RENDER_WIREFRAME));
            } else if (!renderWireframe && !renderGeometry) {
                pkg.disableOptions(to_U32(CmdRenderOptions::RENDER_GEOMETRY) | to_U32(CmdRenderOptions::RENDER_WIREFRAME));
            } else {
                pkg.setDrawOption(CmdRenderOptions::RENDER_GEOMETRY, renderGeometry);
                pkg.setDrawOption(CmdRenderOptions::RENDER_WIREFRAME, renderWireframe);
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

bool RenderingComponent::clearReflection() {
    // If we lake a material, we don't use reflections
    if (getMaterialInstanceCache() != nullptr) {
        getMaterialInstanceCache()->updateReflectionIndex(_reflectorType, -1);
        return true;
    }

    return false;
}

bool RenderingComponent::updateReflection(U32 reflectionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut)
{
    // If we lake a material, we don't use reflections
    if (getMaterialInstanceCache() == nullptr) {
        return false;
    }

    getMaterialInstanceCache()->updateReflectionIndex(_reflectorType, reflectionIndex);

    RenderTargetID reflectRTID(_reflectorType == ReflectorType::PLANAR_REFLECTOR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                                 : RenderTargetUsage::REFRACTION_CUBE, 
                               reflectionIndex);

    if (Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(reflectRTID);

        DebugView* debugView = s_debugViews[0][reflectionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _previewRenderTargetColour;
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
                    debugView->_shader = _previewRenderTargetDepth;
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _previewRenderTargetColour;
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

bool RenderingComponent::clearRefraction() {
    if (getMaterialInstanceCache() == nullptr) {
        return false;
    }
    if (!getMaterialInstanceCache()->hasTransparency()) {
        return false;
    }
    getMaterialInstanceCache()->updateRefractionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateRefraction(U32 refractionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut) {
    // no default refraction system!
    if (!_refractionCallback) {
        return false;
    }

    if (getMaterialInstanceCache() == nullptr) {
        return false;
    }

    getMaterialInstanceCache()->updateRefractionIndex(_reflectorType, refractionIndex);

    RenderTargetID refractRTID(_reflectorType == ReflectorType::PLANAR_REFLECTOR ? RenderTargetUsage::REFRACTION_PLANAR
                                                                                 : RenderTargetUsage::REFRACTION_CUBE,
                               refractionIndex);

    if (Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(refractRTID);

        DebugView* debugView = s_debugViews[1][refractionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _previewRenderTargetColour;
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
                    debugView->_shader = _previewRenderTargetDepth;
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _previewRenderTargetColour;
                }
            } */
        }
    }

    RenderCbkParams params(_context, _parentSGN, renderState, refractRTID, refractionIndex, camera);
    _refractionCallback(params, bufferInOut);

    return false;
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
    if (getMaterialInstanceCache() == nullptr) {
        return;
    }

    RenderTarget* rt = EnvironmentProbe::reflectionTarget()._rt;
    getMaterialInstanceCache()->defaultReflectionTexture(rt->getAttachment(RTAttachmentType::Colour, 0).texture(),
                                                         _envProbes.front()->getRTIndex());
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

};
