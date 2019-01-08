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
#include "Platform/Video/Headers/RenderPackage.h"
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
      _lodLevel(0),
      _renderMask(0),
      _reflectorType(ReflectorType::PLANAR_REFLECTOR),
      _materialInstance(nullptr),
      _skeletonPrimitive(nullptr)
{
    const Material_ptr& materialTpl = parentSGN.getNode().getMaterialTpl();
    if (!materialTpl) {
        //ToDo: REMOVE THIS HACK!
        Console::printfn(Locale::get(_ID("LOAD_DEFAULT_MATERIAL")));
        Material_ptr materialTemplate = CreateResource<Material>(context.kernel().resourceCache(), ResourceDescriptor("defaultMaterial_" + parentSGN.name()));
        materialTemplate->setShadingMode(Material::ShadingMode::BLINN_PHONG);
        _materialInstance = materialTemplate->clone("_instance_" + parentSGN.name());
    } else {
        _materialInstance = materialTpl->clone("_instance_" + parentSGN.name());
    }

    _renderPackagesDirty.fill(true);

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
    }

    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        RenderPackagesPerPassType& packages = _renderPackages[to_base(RenderStagePass::stage(i))];
        std::unique_ptr<RenderPackage>& pkg = packages[to_base(RenderStagePass::pass(i))];

        pkg = std::make_unique<RenderPackage>(true);

        STUBBED("ToDo: Use quality requirements for rendering packages! -Ionut");
        pkg->qualityRequirement(RenderPackage::MinQuality::FULL);
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
        _axisGizmo->attribute4f(to_base(AttribLocation::VERTEX_COLOR), Util::ToFloatColour(_axisLines[0]._colourStart));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : _axisLines) {
            _axisGizmo->attribute4f(to_base(AttribLocation::VERTEX_COLOR), Util::ToFloatColour(line._colourStart));
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

    const Material_ptr& mat = getMaterialInstance();
    // The following commands are needed for material rendering
    // In the absence of a material, use the SceneNode buildDrawCommands to add all of the needed commands
    if (mat != nullptr) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = mat->getRenderStateBlock(stagePass);
        pipelineDescriptor._shaderProgramHandle = mat->getProgramID(stagePass);

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
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->update(deltaTimeUS);
    }

    if (_parentSGN.getNode<Object3D>().getObjectType()._value == ObjectType::SUBMESH) {
        StateTracker<bool>& parentStates = _parentSGN.getParent()->getTrackedBools();
        parentStates.setTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED, false);

        if (_parentSGN.getNode<Object3D>().getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            parentStates.setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, false);
        }
    }

    BaseComponentType<RenderingComponent, ComponentType::RENDERING>::Update(deltaTimeUS);
}

bool RenderingComponent::canDraw(RenderStagePass renderStagePass) {
    if (_parentSGN.getDrawState(renderStagePass)) {
        const Material_ptr& mat = getMaterialInstance();
        if (mat && !mat->canDraw(renderStagePass)) {
            return false;
        }
        return renderOptionEnabled(RenderOptions::IS_VISIBLE);
    }

    return false;
}

void RenderingComponent::rebuildMaterial() {
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->rebuild();
    }

    _parentSGN.forEachChild([](const SceneGraphNode& child) {
        RenderingComponent* const renderable = child.get<RenderingComponent>();
        if (renderable) {
            renderable->rebuildMaterial();
        }
    });
}

void RenderingComponent::onRender(RenderStagePass renderStagePass) {
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        DescriptorSet set = getDrawPackage(renderStagePass).descriptorSet(0);
        if (mat->getTextureData(renderStagePass, set._textureData)) {
            getDrawPackage(renderStagePass).descriptorSet(0, set);
        }
    }
}

void RenderingComponent::onRefreshNodeData(RenderStagePass renderStagePass, GFX::CommandBuffer& bufferInOut) {
    _parentSGN.onRefreshNodeData(renderStagePass, bufferInOut);
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const {
    bool shadowMappingEnabled = _context.parent().platformContext().config().rendering.shadowMapping.enabled;

    propertiesOut.set(_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED
                                                     ? -1.0f
                                                     : _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_HOVER
                                                                                      ? 1.0f
                                                                                      : 0.0f,
                      (shadowMappingEnabled && renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS)) ? 1.0f : 0.0f,
                      _lodLevel,
                      0.0);
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        reflectionIndex = to_F32(mat->defaultReflectionTextureIndex());
        refractionIndex = to_F32(mat->defaultRefractionTextureIndex());
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

void RenderingComponent::updateLoDLevel(const Camera& camera, RenderStagePass renderStagePass) {
    static const U32 SCENE_NODE_LOD0_SQ = Config::SCENE_NODE_LOD0 * Config::SCENE_NODE_LOD0;
    static const U32 SCENE_NODE_LOD1_SQ = Config::SCENE_NODE_LOD1 * Config::SCENE_NODE_LOD1;

    _lodLevel = to_U8(_parentSGN.getNode().getLODcount() - 1);

    // ToDo: Hack for lower LoD rendering in reflection and refraction passes
    if (renderStagePass._stage != RenderStage::REFLECTION && renderStagePass._stage != RenderStage::REFRACTION) {
        const vec3<F32>& eyePos = camera.getEye();
        const BoundingSphere& bSphere = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        F32 cameraDistanceSQ = bSphere.getCenter().distanceSquared(eyePos);

        U8 lodLevelTemp = cameraDistanceSQ > SCENE_NODE_LOD0_SQ
                                           ? cameraDistanceSQ > SCENE_NODE_LOD1_SQ ? 2 : 1
                                           : 0;

        _lodLevel = std::min(_lodLevel, std::max(lodLevelTemp, to_U8(0)));
    }
}

void RenderingComponent::prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, RenderStagePass renderStagePass) {
    RenderPackage& pkg = getDrawPackage(renderStagePass);
    if (canDraw(renderStagePass)) {

        bool& dirty = _renderPackagesDirty[renderStagePass.index()];
        if (dirty) {
            pkg.setDrawOption(CmdRenderOptions::RENDER_INDIRECT, true);
            rebuildDrawCommands(renderStagePass);
            dirty = false;
        }

        if (_parentSGN.prepareRender(camera, renderStagePass)) {
            updateLoDLevel(camera, renderStagePass);

            bool renderGeometry = renderOptionEnabled(RenderOptions::RENDER_GEOMETRY);
            renderGeometry = renderGeometry || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);

            bool renderWireframe = renderOptionEnabled(RenderOptions::RENDER_WIREFRAME);
            renderWireframe = renderWireframe || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);
            pkg.setDrawOption(CmdRenderOptions::RENDER_GEOMETRY, renderGeometry);
            pkg.setDrawOption(CmdRenderOptions::RENDER_WIREFRAME, renderWireframe);
        }
    }
}

void RenderingComponent::updateDrawCommands(RenderStage stage, vectorEASTL<IndirectDrawCommand>& drawCmdsInOut) {
    RenderPackagesPerPassType& packagesPerPass = _renderPackages[to_base(stage)];
    for (std::unique_ptr<RenderPackage>& pkg : packagesPerPass) {
        Attorney::RenderPackageRenderingComponent::updateDrawCommands(*pkg, to_U32(drawCmdsInOut.size()));
    }

    std::unique_ptr<RenderPackage>& pkg = packagesPerPass.front();
    for (I32 i = 0; i < pkg->drawCommandCount(); ++i) {
        GenericDrawCommand cmd = pkg->drawCommand(i, 0);
        if (cmd._drawCount > 1 && isEnabledOption(cmd, CmdRenderOptions::CONVERT_TO_INDIRECT)) {
            std::fill_n(std::back_inserter(drawCmdsInOut), cmd._drawCount, cmd._cmd);
        } else {
            drawCmdsInOut.push_back(cmd._cmd);
        }
    }
}

void RenderingComponent::setDataIndex(RenderStage stage, U32 dataIndex) {
    RenderPackagesPerPassType& packagesPerPassType = _renderPackages[to_base(stage)];
    for (std::unique_ptr<RenderPackage>& package : packagesPerPassType) {
        Attorney::RenderPackageRenderingComponent::setDataIndex(*package, dataIndex);
    }
}

bool RenderingComponent::hasDrawCommands(RenderStage stage) {
    RenderPackagesPerPassType& packagesPerPass = _renderPackages[to_base(stage)];
    for (std::unique_ptr<RenderPackage>& pkg : packagesPerPass) {
        if (pkg->drawCommandCount() > 0) {
            return true;
        }
    }

    return false;
}

RenderPackage& RenderingComponent::getDrawPackage(RenderStagePass renderStagePass) {
    RenderPackage& ret = *_renderPackages[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)].get();
    //ToDo: Some validation? -Ionut
    return ret;
}

const RenderPackage& RenderingComponent::getDrawPackage(RenderStagePass renderStagePass) const {
    const RenderPackage& ret = *_renderPackages[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)].get();
    //ToDo: Some validation? -Ionut
    return ret;
}


size_t RenderingComponent::getSortKeyHash(RenderStagePass renderStagePass) const {
    return getDrawPackage(renderStagePass).getSortKeyHash();
}

bool RenderingComponent::clearReflection() {
    // If we lake a material, we don't use reflections
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }

    mat->updateReflectionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateReflection(U32 reflectionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut)
{
    // Low lod entities don't need up to date reflections
    if (_lodLevel > 1) {
        return false;
    }
    // If we lake a material, we don't use reflections
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }

    mat->updateReflectionIndex(_reflectorType, reflectionIndex);

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
            viewPtr->_shaderData.set("linearSpace", GFX::PushConstantType::BOOL, false);
            viewPtr->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);

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
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }
    if (!mat->hasTransparency()) {
        return false;
    }
    mat->updateRefractionIndex(_reflectorType, -1);
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
    // Low lod entities don't need up to date reflections
    if (_lodLevel > 1) {
        return false;
    }
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }

    mat->updateRefractionIndex(_reflectorType, refractionIndex);

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
            viewPtr->_shaderData.set("linearSpace", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
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
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return;
    }

    RenderTarget* rt = EnvironmentProbe::reflectionTarget()._rt;
    mat->defaultReflectionTexture(rt->getAttachment(RTAttachmentType::Colour, 0).texture(),
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
