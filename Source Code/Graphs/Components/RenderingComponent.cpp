#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
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

RenderingComponent::RenderingComponent(GFXDevice& context,
                                       Material_ptr materialInstance,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::RENDERING, parentSGN),
      _context(context),
      _lodLevel(0),
      _commandIndex(0),
      _commandOffset(0),
      _renderMask(0),
      _depthStateBlockHash(0),
      _shadowStateBlockHash(0),
      _renderPackagesDirty(true),
      _reflectorType(ReflectorType::PLANAR_REFLECTOR),
      _materialInstance(materialInstance),
      _skeletonPrimitive(nullptr)
{

    toggleRenderOption(RenderOptions::RENDER_GEOMETRY, true);
    toggleRenderOption(RenderOptions::CAST_SHADOWS, true);
    toggleRenderOption(RenderOptions::RECEIVE_SHADOWS, true);
    toggleRenderOption(RenderOptions::IS_VISIBLE, true);

    Object3D_ptr node = parentSGN.getNode<Object3D>();
    Object3D::ObjectType type = node->getObjectType();
    SceneNodeType nodeType = node->getType();

    bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
    bool nodeSkinned = parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

    assert(!_materialInstance || (_materialInstance && !_materialInstance->getName().empty()));

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        if (_materialInstance) {
            if (!isSubMesh) {
                _materialInstance->addShaderModifier(RenderStagePass(RenderStage::SHADOW, static_cast<RenderPassType>(pass)), "TriangleStrip");
                _materialInstance->setShaderDefines(RenderStagePass(RenderStage::SHADOW, static_cast<RenderPassType>(pass)), "USE_TRIANGLE_STRIP");
            }
        }

        for (RenderPackage& pkg : _renderData[pass]) {
            pkg.isOcclusionCullable(nodeType != SceneNodeType::TYPE_SKY);
        }
    }

    RenderStateBlock depthDesc;
    depthDesc.setColourWrites(false, false, false, false);
    depthDesc.setZFunc(ComparisonFunction::LESS);
    _depthStateBlockHash = depthDesc.getHash();

    RenderStateBlock shadowDesc;
    /// Cull back faces for shadow rendering
    shadowDesc.setCullMode(CullMode::CCW);
    // depthDesc.setZBias(1.0f, 2.0f);
    shadowDesc.setColourWrites(true, true, false, false);
    _shadowStateBlockHash = shadowDesc.getHash();

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    Pipeline pipeline = _context.newPipeline(pipelineDescriptor);

    _boundingBoxPrimitive[0] = _context.newIMP();
    _boundingBoxPrimitive[0]->name("BoundingBox_" + parentSGN.getName());
    _boundingBoxPrimitive[0]->pipeline(pipeline);
    _boundingBoxPrimitive[0]->paused(true);

    _boundingBoxPrimitive[1] = _context.newIMP();
    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + parentSGN.getName());
    _boundingBoxPrimitive[1]->pipeline(pipeline);
    _boundingBoxPrimitive[1]->paused(true);

    _boundingSpherePrimitive = _context.newIMP();
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.getName());
    _boundingSpherePrimitive->pipeline(pipeline);
    _boundingSpherePrimitive->paused(true);

    if (nodeSkinned) {
        primitiveStateBlock.setZRead(false);
        _skeletonPrimitive = _context.newIMP();
        _skeletonPrimitive->name("Skeleton_" + parentSGN.getName());
        _skeletonPrimitive->pipeline(pipeline);
        _skeletonPrimitive->paused(true);
    }
    
    if (Config::Build::IS_DEBUG_BUILD) {
        ResourceDescriptor previewReflectionRefractionColour("fbPreview");
        previewReflectionRefractionColour.setThreadedLoading(true);
        _previewRenderTargetColour = CreateResource<ShaderProgram>(context.parent().resourceCache(), previewReflectionRefractionColour);

        ResourceDescriptor previewReflectionRefractionDepth("fbPreview.LinearDepth.ScenePlanes");
        previewReflectionRefractionDepth.setPropertyList("USE_SCENE_ZPLANES");
        _previewRenderTargetDepth = CreateResource<ShaderProgram>(context.parent().resourceCache(), previewReflectionRefractionDepth);

        // Red X-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255), 5.0f));
        // Green Y-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255), 5.0f));
        // Blue Z-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255), 5.0f));
        _axisGizmo = _context.newIMP();
        // Prepare it for line rendering
        size_t noDepthStateBlock = _context.getDefaultStateBlock(true);
        RenderStateBlock stateBlock(RenderStateBlock::get(noDepthStateBlock));

        pipelineDescriptor._stateHash = stateBlock.getHash();
        _axisGizmo->name("AxisGizmo_" + parentSGN.getName());
        _axisGizmo->pipeline(_context.newPipeline(pipelineDescriptor));
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

        _axisGizmo->paused(true);
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

void RenderingComponent::rebuildDrawCommands(const RenderStagePass& stagePass) {
    RenderPackage& pkg = renderData(stagePass);

    pkg._commands.clear();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = getMaterialStateHash(stagePass);
    pipelineDescriptor._shaderProgram = getDrawShader(stagePass);

    BindPipelineCommand pipelineCommand;
    pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
    pkg._commands.add(pipelineCommand);

    SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _globalPushConstants;
    pkg._commands.add(pushConstantsCommand);

    _parentSGN.getNode()->buildDrawCommands(_parentSGN, stagePass, pkg);
}

void RenderingComponent::postLoad() {
}

void RenderingComponent::update(const U64 deltaTime) {
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->update(deltaTime);
    }

    Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
    if (type == Object3D::ObjectType::SUBMESH)
    {
        StateTracker<bool>& parentStates = _parentSGN.getParent().lock()->getTrackedBools();
        parentStates.setTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED, false);

        if (_parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            parentStates.setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, false);
        }
    }
}

bool RenderingComponent::canDraw(const RenderStagePass& renderStagePass) {
    if (_parentSGN.getNode()->getDrawState(renderStagePass)) {
        const Material_ptr& mat = getMaterialInstance();
        if (mat) {
            if (!mat->canDraw(renderStagePass)) {
                return false;
            }
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

void RenderingComponent::registerTextureDependency(const TextureData& additionalTexture) {
    _textureDependencies.addTexture(additionalTexture);
}

void RenderingComponent::removeTextureDependency(const TextureData& additionalTexture) {
    _textureDependencies.removeTexture(additionalTexture);
}

bool RenderingComponent::onRender(const RenderStagePass& renderStagePass) {
    // Call any pre-draw operations on the SceneNode (refresh VB, update
    // materials, get list of textures, etc)
    RenderPackage& pkg = renderData(renderStagePass);

    pkg._textureData.clear(false);
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->getTextureData(pkg._textureData);
    }

    for (const TextureData& texture : _textureDependencies.textures()) {
        pkg._textureData.addTexture(texture);
    }

    return _parentSGN.getNode()->onRender(renderStagePass);
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(vec4<F32>& propertiesOut, F32& reflectionIndex, F32& refractionIndex) const {
    propertiesOut.set(_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED
                                                     ? -1.0f
                                                     : _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_HOVER
                                                                                      ? 1.0f
                                                                                      : 0.0f,
                      renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS) ? 1.0f : 0.0f,
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
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, RenderSubPassCmds& subPassesInOut) {
    
    if (renderStagePass.stage() != RenderStage::DISPLAY || renderStagePass.pass() == RenderPassType::DEPTH_PASS) {
        return;
    }

    const SceneNode_ptr& node = _parentSGN.getNode();

    if (Config::Build::IS_DEBUG_BUILD) {
        switch(sceneRenderState.gizmoState()){
            case SceneRenderState::GizmoState::ALL_GIZMO: {
                if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
                    if (_parentSGN.getNode<Object3D>()->getObjectType() == Object3D::ObjectType::SUBMESH) {
                        drawDebugAxis();
                    }
                }
            } break;
            case SceneRenderState::GizmoState::SELECTED_GIZMO: {
                switch (_parentSGN.getSelectionFlag()) {
                    case SceneGraphNode::SelectionFlag::SELECTION_SELECTED : {
                        drawDebugAxis();
                    } break;
                    default: {
                        _axisGizmo->paused(true);
                    } break;
                }
            } break;
            default: {
                _axisGizmo->paused(true);
            } break;
        }
    }

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    StateTracker<bool>& parentStates = grandParent->getTrackedBools();

    // Draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    bool renderBBox = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB);
    renderBBox = renderBBox || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);

    bool renderBSphere = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE);

    if (renderBBox) {
        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive[0]->fromBox(bb.getMin(), bb.getMax(), vec4<U8>(0, 0, 255, 255));

        renderBSphere = renderBSphere || _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED;
        toggleRenderOption(RenderOptions::RENDER_BOUNDS_SPHERE, renderBSphere);

        Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
        bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
        if (isSubMesh) {
            bool renderParentBBFlagInitialized = false;
            bool renderParentBB = parentStates.getTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED,
                                   renderParentBBFlagInitialized);
            if (!renderParentBB || !renderParentBBFlagInitialized) {
                const BoundingBox& bbGrandParent = grandParent->get<BoundsComponent>()->getBoundingBox();
                _boundingBoxPrimitive[1]->fromBox(
                                     bbGrandParent.getMin() - vec3<F32>(0.0025f),
                                     bbGrandParent.getMax() + vec3<F32>(0.0025f),
                                     vec4<U8>(0, 128, 128, 255));
            }
        }
    }

    if (renderBSphere) {
        const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        _boundingSpherePrimitive->fromSphere(bs.getCenter(), bs.getRadius(), vec4<U8>(0, 255, 0, 255));
    }

    bool renderSkeleton = renderOptionEnabled(RenderOptions::RENDER_SKELETON);
    renderSkeleton = renderSkeleton || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);

    if (renderSkeleton) {
        // Continue only for skinned submeshes
        Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
        bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
        if (isSubMesh && _parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))
        {
            bool renderSkeletonFlagInitialized = false;
            bool renderParentSkeleton = parentStates.getTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, renderSkeletonFlagInitialized);
            if (!renderParentSkeleton || !renderSkeletonFlagInitialized) {
                // Get the animation component of any submesh. They should be synced anyway.
                AnimationComponent* childAnimComp = _parentSGN.get<AnimationComponent>();
                // Get the skeleton lines from the submesh's animation component
                const vectorImpl<Line>& skeletonLines = childAnimComp->skeletonLines();
                _skeletonPrimitive->worldMatrix(_parentSGN.get<PhysicsComponent>()->getWorldMatrix());
                // Submit the skeleton lines to the GPU for rendering
                _skeletonPrimitive->fromLines(skeletonLines);
                parentStates.setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, true);
            }
        }
    } else {
        if (_skeletonPrimitive) {
            _skeletonPrimitive->paused(true);
        }
    }

    RenderSubPassCmd& subPassInOut = subPassesInOut.back();
    subPassInOut._commands.add(_boundingBoxPrimitive[0]->toDrawCommands());
    subPassInOut._commands.add(_boundingBoxPrimitive[1]->toDrawCommands());
    subPassInOut._commands.add(_boundingSpherePrimitive->toDrawCommands());
    if (_skeletonPrimitive) {
        subPassInOut._commands.add(_skeletonPrimitive->toDrawCommands());
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        subPassInOut._commands.add(_axisGizmo->toDrawCommands());
    }
}

void RenderingComponent::registerShaderBuffer(ShaderBufferLocation slot,
                                              vec2<U32> bindRange,
                                              ShaderBuffer& shaderBuffer) {
    ShaderBufferList::iterator it;
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (RenderPackage& pkg : _renderData[pass]) {
            ShaderBufferList::iterator itEnd = std::end(pkg._shaderBuffers);
            it = std::find_if(std::begin(pkg._shaderBuffers), itEnd,
                [slot](const ShaderBufferBinding& binding)
                        -> bool { return binding._binding == slot; });

            if (it == itEnd) {
               vectorAlg::emplace_back(pkg._shaderBuffers, slot, &shaderBuffer, bindRange);
            } else {
                it->set(slot, &shaderBuffer, bindRange);
            }
        }
    }
}

void RenderingComponent::unregisterShaderBuffer(ShaderBufferLocation slot) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (RenderPackage& pkg : _renderData[pass]) {
            pkg._shaderBuffers.erase(
                std::remove_if(std::begin(pkg._shaderBuffers), std::end(pkg._shaderBuffers),
                    [&slot](const ShaderBufferBinding& binding)
                    -> bool { return binding._binding == slot; }),
                std::end(pkg._shaderBuffers));
        }
    }
}

ShaderProgram_ptr RenderingComponent::getDrawShader(const RenderStagePass& renderStagePass) {
    if (_materialInstance) {
        return _materialInstance->getShaderInfo(renderStagePass).getProgram();
    }

    return nullptr;
}

size_t RenderingComponent::getMaterialStateHash(const RenderStagePass& renderStagePass) {
    bool shadowStage = renderStagePass.stage() == RenderStage::SHADOW;
    bool depthPass   = renderStagePass.pass() == RenderPassType::DEPTH_PASS || shadowStage;

    if (!getMaterialInstance() && depthPass) {
        return shadowStage ? _shadowStateBlockHash : _depthStateBlockHash;
    }

    if (!_materialInstance) {
        return 0;
    }

    I32 variant = 0;
    if (shadowStage && LightPool::currentShadowCastingLight()) {
        LightType type = LightPool::currentShadowCastingLight()->getLightType();
        variant = (type == LightType::DIRECTIONAL
                         ? 0
                         : type == LightType::POINT 
                                 ? 1
                                 : 2);
    }

    return _materialInstance->getRenderStateBlock(renderStagePass, variant);
}

void RenderingComponent::updateLoDLevel(const Camera& camera, const RenderStagePass& renderStagePass) {
    static const U32 SCENE_NODE_LOD0_SQ = Config::SCENE_NODE_LOD0 * Config::SCENE_NODE_LOD0;
    static const U32 SCENE_NODE_LOD1_SQ = Config::SCENE_NODE_LOD1 * Config::SCENE_NODE_LOD1;

    _lodLevel = to_U8(_parentSGN.getNode()->getLODcount() - 1);

    // ToDo: Hack for lower LoD rendering in reflection and refraction passes
    if (renderStagePass.stage() != RenderStage::REFLECTION && renderStagePass.stage() != RenderStage::REFRACTION) {
        const vec3<F32>& eyePos = camera.getEye();
        const BoundingSphere& bSphere = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        F32 cameraDistanceSQ = bSphere.getCenter().distanceSquared(eyePos);

        U8 lodLevelTemp = cameraDistanceSQ > SCENE_NODE_LOD0_SQ
                                           ? cameraDistanceSQ > SCENE_NODE_LOD1_SQ ? 2 : 1
                                           : 0;

        _lodLevel = std::min(_lodLevel, std::max(lodLevelTemp, to_U8(0)));
    }
}

void RenderingComponent::setDrawIDs(const RenderStagePass& renderStagePass,
                                    U32 cmdOffset,
                                    U32 cmdIndex) {
    commandOffset(cmdOffset);
    commandIndex(cmdIndex);

    RenderPackage& pkg = renderData(renderStagePass);
    
    const vectorImpl<GenericDrawCommand*>& commands = pkg._commands.getDrawCommands();

    for (GenericDrawCommand* cmd : commands) {
        cmd->commandOffset(cmdOffset++);
        cmd->toggleOption(GenericDrawCommand::RenderOptions::RENDER_INDIRECT, true);
        cmd->cmd().baseInstance = cmdIndex;
    }
}

void RenderingComponent::prepareDrawPackage(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass) {
    RenderPackage& pkg = renderData(renderStagePass);
    pkg.isRenderable(false);
    if (canDraw(renderStagePass) && _parentSGN.prepareDraw(sceneRenderState, renderStagePass)) {

        if (_renderPackagesDirty) {
            for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
                rebuildDrawCommands(RenderStagePass::stagePass(i));
            }
            _renderPackagesDirty = false;
        }

        _parentSGN.getNode()->updateDrawCommands(_parentSGN, renderStagePass, sceneRenderState, pkg);

        updateLoDLevel(*Camera::activeCamera(), renderStagePass);

        bool renderGeometry = renderOptionEnabled(RenderOptions::RENDER_GEOMETRY);
        renderGeometry = renderGeometry || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);

        bool renderWireframe = renderOptionEnabled(RenderOptions::RENDER_WIREFRAME);
        renderWireframe = renderWireframe || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);

        const vectorImpl<GenericDrawCommand*>& commands = pkg._commands.getDrawCommands();
        for (GenericDrawCommand* cmd : commands) {
            cmd->toggleOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY,
                              renderGeometry  || cmd->isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY));

            cmd->toggleOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME,
                              renderWireframe || cmd->isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME));

            cmd->LoD(_lodLevel);
        }
        if (!commands.empty()) {
            pkg.isRenderable(true);
            setDrawIDs(renderStagePass, commandOffset(), commandIndex());
        }
    }
}

const RenderPackage& RenderingComponent::getDrawPackage(const RenderStagePass& renderStagePass) const {
    const RenderPackage& ret = renderData(renderStagePass);
    //ToDo: Some validation? -Ionut
    return ret;
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
                                          const SceneRenderState& renderState)
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
        GFXDevice::DebugView_ptr& viewPtr = _debugViews[0][reflectionIndex];
        const RenderTarget& target = _context.renderTargetPool().renderTarget(reflectRTID);
        if (!viewPtr) {
            viewPtr = std::make_shared<GFXDevice::DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _previewRenderTargetColour;
            viewPtr->_shaderData.set("lodLevel", PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set("linearSpace", PushConstantType::BOOL, false);
            viewPtr->_shaderData.set("unpack2Channel", PushConstantType::BOOL, false);

            viewPtr->_name = Util::StringFormat("Reflection_", reflectRTID);
            _context.addDebugView(viewPtr);
        } else {
            if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (viewPtr->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    viewPtr->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    viewPtr->_shader = _previewRenderTargetDepth;
                } else {
                    viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    viewPtr->_shader = _previewRenderTargetColour;
                }
                
            }

        }
    }

    if (_reflectionCallback) {
        RenderCbkParams params(_context, _parentSGN, renderState, reflectRTID, reflectionIndex, camera);
        _reflectionCallback(params);
    } else {
        if (_reflectorType == ReflectorType::CUBE_REFLECTOR) {
            const vec2<F32>& zPlanes = camera->getZPlanes();
            _context.generateCubeMap(reflectRTID,
                                     0,
                                     camera->getEye(),
                                     vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                     RenderStagePass(RenderStage::REFLECTION, RenderPassType::COLOUR_PASS),
                                     reflectionIndex);
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
                                          const SceneRenderState& renderState) {
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
        GFXDevice::DebugView_ptr& viewPtr = _debugViews[1][refractionIndex];
        const RenderTarget& target = _context.renderTargetPool().renderTarget(refractRTID);

        if (!viewPtr) {
            viewPtr = std::make_shared<GFXDevice::DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _previewRenderTargetColour;
            viewPtr->_shaderData.set("lodLevel", PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set("linearSpace", PushConstantType::BOOL, false);
            viewPtr->_shaderData.set("unpack2Channel", PushConstantType::BOOL, false);
            viewPtr->_name = Util::StringFormat("Refraction", refractRTID);
            _context.addDebugView(viewPtr);
        } else {
            if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (viewPtr->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    viewPtr->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    viewPtr->_shader = _previewRenderTargetDepth;
                } else {
                    viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    viewPtr->_shader = _previewRenderTargetColour;
                }
            } 
        }
    }

    RenderCbkParams params(_context, _parentSGN, renderState, refractRTID, refractionIndex, camera);
    _refractionCallback(params);

    return false;
}

void RenderingComponent::updateEnvProbeList(const EnvironmentProbeList& probes) {
    _envProbes.resize(0);
    if (probes.empty()) {
        return;
    }

    _envProbes.insert(std::cend(_envProbes), std::cbegin(probes), std::cend(probes));

    PhysicsComponent* const transform = _parentSGN.get<PhysicsComponent>();
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

    PhysicsComponent* const transform = _parentSGN.get<PhysicsComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()), false);
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    _axisGizmo->paused(false);
}

};
