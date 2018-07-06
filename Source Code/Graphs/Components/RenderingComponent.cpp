#include "Headers/RenderingComponent.h"

#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Dynamics/Entities/Headers/Impostor.h"

namespace Divide {

namespace {
    const F32 g_MaterialShininessThresholdForReflection = 127.0f;
};

RenderingComponent::RenderingComponent(Material* const materialInstance,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::RENDERING, parentSGN),
      _lodLevel(0),
      _drawOrder(0),
      _commandIndex(0),
      _commandOffset(0),
      _castsShadows(true),
      _receiveShadows(true),
      _renderWireframe(false),
      _renderGeometry(true),
      _renderBoundingBox(false),
      _renderBoundingSphere(false),
      _renderSkeleton(false),
      _materialInstance(materialInstance),
      _skeletonPrimitive(nullptr)
{
    Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
    bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
    bool nodeSkinned = parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

    if (_materialInstance) {
        if (!isSubMesh) {
            _materialInstance->addShaderModifier(RenderStage::SHADOW, "TriangleStrip");
            _materialInstance->setShaderDefines(RenderStage::SHADOW, "USE_TRIANGLE_STRIP");
        }
    }

    for (GFXDevice::RenderPackage& pkg : _renderData) {
        pkg._textureData.reserve(ParamHandler::getInstance().getParam<I32>(_ID("rendering.maxTextureSlots"), 16));
    }

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;

    _boundingBoxPrimitive[0] = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingBoxPrimitive[0]->name("BoundingBox_" + parentSGN.getName());
    _boundingBoxPrimitive[0]->stateHash(primitiveStateBlock.getHash());
    _boundingBoxPrimitive[0]->paused(true);

    _boundingBoxPrimitive[1] = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + parentSGN.getName());
    _boundingBoxPrimitive[1]->stateHash(primitiveStateBlock.getHash());
    _boundingBoxPrimitive[1]->paused(true);

    _boundingSpherePrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.getName());
    _boundingSpherePrimitive->stateHash(primitiveStateBlock.getHash());
    _boundingSpherePrimitive->paused(true);

    if (nodeSkinned) {
        primitiveStateBlock.setZRead(false);
        _skeletonPrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
        _skeletonPrimitive->name("Skeleton_" + parentSGN.getName());
        _skeletonPrimitive->stateHash(primitiveStateBlock.getHash());
        _skeletonPrimitive->paused(true);
    }

#ifdef _DEBUG
    // Red X-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255), 5.0f));
    // Green Y-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255), 5.0f));
    // Blue Z-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255), 5.0f));
    _axisGizmo = GFX_DEVICE.getOrCreatePrimitive(false);
    // Prepare it for line rendering
    U32 noDepthStateBlock = GFX_DEVICE.getDefaultStateBlock(true);
    RenderStateBlock stateBlock(GFX_DEVICE.getRenderStateBlock(noDepthStateBlock));
    _axisGizmo->name("AxisGizmo_" + parentSGN.getName());
    _axisGizmo->stateHash(stateBlock.getHash());
    _axisGizmo->paused(true);
    // Create the object containing all of the lines
    _axisGizmo->beginBatch(true, to_uint(_axisLines.size()) * 2, 1);
    _axisGizmo->attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(_axisLines[0]._colorStart));
    // Set the mode to line rendering
    _axisGizmo->begin(PrimitiveType::LINES);
    // Add every line in the list to the batch
    for (const Line& line : _axisLines) {
        _axisGizmo->attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(line._colorStart));
        _axisGizmo->vertex(line._startPoint);
        _axisGizmo->vertex(line._endPoint);
    }
    _axisGizmo->end();
    // Finish our object
    _axisGizmo->endBatch();
#endif
}

RenderingComponent::~RenderingComponent()
{
    _boundingBoxPrimitive[0]->_canZombify = true;
    _boundingBoxPrimitive[1]->_canZombify = true;
    _boundingSpherePrimitive->_canZombify = true;
    if (_skeletonPrimitive) {
        _skeletonPrimitive->_canZombify = true;
    }
#ifdef _DEBUG
    _axisGizmo->_canZombify = true;
#endif
    if (_materialInstance) {
        RemoveResource(_materialInstance);
    }

}

void RenderingComponent::update(const U64 deltaTime) {
    Material* mat = getMaterialInstance();
    if (mat) {
        mat->update(deltaTime);
    }

    Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
    // Continue only for skinned submeshes
    if (type == Object3D::ObjectType::SUBMESH)
    {
        _parentSGN.getParent().lock()->getTrackedBools().setTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED, false);

        if (_parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            _parentSGN.getParent().lock()->getTrackedBools().setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, false);
            _skeletonPrimitive->paused(true);
        }
    }

}

bool RenderingComponent::canDraw(const SceneRenderState& sceneRenderState,
                                 RenderStage renderStage) {
    bool canDraw = _parentSGN.getNode()->getDrawState(renderStage);
    if (canDraw) {
        Material* mat = getMaterialInstance();
        if (mat) {
            if (!mat->canDraw(renderStage)) {
                return false;
            }
        }
    }

    return canDraw;
}

void RenderingComponent::registerTextureDependency(const TextureData& additionalTexture) {
    U32 inputHash = additionalTexture.getHash();
    TextureDataContainer::const_iterator it;
    it = std::find_if(std::begin(_textureDependencies), std::end(_textureDependencies),
                      [&inputHash](const TextureData& textureData) { 
                            return (textureData.getHash() == inputHash); 
                      });

    if (it == std::end(_textureDependencies)) {
        _textureDependencies.push_back(additionalTexture);
    }
}

void RenderingComponent::removeTextureDependency(const TextureData& additionalTexture) {
    U32 inputHash = additionalTexture.getHash();
    TextureDataContainer::iterator it;
    it = std::find_if(std::begin(_textureDependencies), std::end(_textureDependencies),
                      [&inputHash](const TextureData& textureData) { 
                            return (textureData.getHash() == inputHash); 
                      });

    if (it != std::end(_textureDependencies)) {
        _textureDependencies.erase(it);
    }
}

bool RenderingComponent::onRender(RenderStage currentStage) {
    // Call any pre-draw operations on the SceneNode (refresh VB, update
    // materials, get list of textures, etc)

    GFXDevice::RenderPackage& pkg = _renderData[to_uint(currentStage)];

    pkg._textureData.resize(0);
    Material* mat = getMaterialInstance();
    if (mat) {
        mat->getTextureData(pkg._textureData);
    }

    for (const TextureData& texture : _textureDependencies) {
        pkg._textureData.push_back(texture);
    }

    return _parentSGN.getNode()->onRender(_parentSGN, currentStage);
}

void RenderingComponent::renderGeometry(const bool state) {
    if (_renderGeometry != state) {
        _renderGeometry = state;

        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = 
                _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->renderGeometry(state);
            }
        }
    }
}

void RenderingComponent::renderWireframe(const bool state) {
    if (_renderWireframe != state) {
        _renderWireframe = state;
    
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->renderWireframe(state);
            }
        }
    }
}

void RenderingComponent::renderBoundingBox(const bool state) {
    if (_renderBoundingBox != state) {
        _renderBoundingBox = state;
        if (!state) {
            _boundingBoxPrimitive[0]->paused(true);
            _boundingBoxPrimitive[1]->paused(true);
        }
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->renderBoundingBox(state);
            }
        }
    }
}

void RenderingComponent::renderBoundingSphere(const bool state) {
    if (_renderBoundingSphere != state) {
        _renderBoundingSphere = state;
        if (!state) {
            _boundingSpherePrimitive->paused(true);
        }
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->renderBoundingSphere(state);
            }
        }
    }
}

void RenderingComponent::renderSkeleton(const bool state) {
    if (_renderSkeleton != state) {
        _renderSkeleton = state;
        if (!state && _skeletonPrimitive) {
            _skeletonPrimitive->paused(true);
        }
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->renderSkeleton(state);
            }
        }
    }
}

void RenderingComponent::castsShadows(const bool state) {
    if (_castsShadows != state) {
        _castsShadows = state;
    
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->castsShadows(_castsShadows);
            }
        }
    }
}

void RenderingComponent::receivesShadows(const bool state) {
    if (_receiveShadows != state) {
        _receiveShadows = state;
    
        U32 childCount = _parentSGN.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).get<RenderingComponent>();
            if (renderable) {
                renderable->receivesShadows(_receiveShadows);
            }
        }
    }
}

bool RenderingComponent::castsShadows() const {
    return _castsShadows;
}

bool RenderingComponent::receivesShadows() const {
    return _receiveShadows;
}

void RenderingComponent::getMaterialColorMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    Material* mat = getMaterialInstance();
    if (mat) {
        mat->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(vec4<F32>& propertiesOut) const {
    propertiesOut.set(_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED
                                                     ? -1.0f
                                                     : _parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_HOVER
                                                                                      ? 1.0f
                                                                                      : 0.0f,
                      receivesShadows() ? 1.0f : 0.0f,
                      _lodLevel,
                      0.0);
}

bool RenderingComponent::preDraw(const SceneRenderState& renderState,
                                 RenderStage renderStage) const {
    return _parentSGN.prepareDraw(renderState, renderStage);
}

/// Called after the current node was rendered
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, RenderStage renderStage) {
    
    if (renderStage != RenderStage::DISPLAY) {
        return;
    }

    SceneNode* const node = _parentSGN.getNode();

#ifdef _DEBUG
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
#endif

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    StateTracker<bool>& parentStates = grandParent->getTrackedBools();

    // Draw bounding box if needed and only in the final stage to prevent
    // Shadow/PostFX artifacts
    if (renderBoundingBox() || sceneRenderState.drawBoundingBoxes()) {
        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        GFX_DEVICE.drawBox3D(*_boundingBoxPrimitive[0], bb.getMin(), bb.getMax(), vec4<U8>(0, 0, 255, 255));


        if (_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            renderBoundingSphere(true);
        } else {
            renderBoundingSphere(false);
        }

        Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
        bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
        if (isSubMesh) {
            bool renderParentBBFlagInitialized = false;
            bool renderParentBB = parentStates.getTrackedValue(StateTracker<bool>::State::BOUNDING_BOX_RENDERED,
                                   renderParentBBFlagInitialized);
            if (!renderParentBB || !renderParentBBFlagInitialized) {
                const BoundingBox& bbGrandParent = grandParent->get<BoundsComponent>()->getBoundingBox();
                GFX_DEVICE.drawBox3D(*_boundingBoxPrimitive[1],
                                     bbGrandParent.getMin() - vec3<F32>(0.0025f),
                                     bbGrandParent.getMax() + vec3<F32>(0.0025f),
                                     vec4<U8>(0, 128, 128, 255));
            }
        }
    } else {
        _boundingBoxPrimitive[0]->paused(true);
        _boundingBoxPrimitive[1]->paused(true);
        renderBoundingSphere(false);
    }

    
    if (renderBoundingSphere()) {
        const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        GFX_DEVICE.drawSphere3D(*_boundingSpherePrimitive, bs.getCenter(), bs.getRadius(),
                                vec4<U8>(0, 255, 0, 255));
    } else {
        _boundingSpherePrimitive->paused(true);
    }

    if (_renderSkeleton || sceneRenderState.drawSkeletons()) {
        // Continue only for skinned submeshes
        Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
        bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
        if (isSubMesh && _parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))
        {
            bool renderSkeletonFlagInitialized = false;
            bool renderSkeleton = parentStates.getTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED,
                                                               renderSkeletonFlagInitialized);
            if (!renderSkeleton || !renderSkeletonFlagInitialized) {
                // Get the animation component of any submesh. They should be synced anyway.
                AnimationComponent* childAnimComp =
                    _parentSGN.get<AnimationComponent>();
                // Get the skeleton lines from the submesh's animation component
                const vectorImpl<Line>& skeletonLines = childAnimComp->skeletonLines();
                _skeletonPrimitive->worldMatrix(_parentSGN.get<PhysicsComponent>()->getWorldMatrix());
                // Submit the skeleton lines to the GPU for rendering
                GFX_DEVICE.drawLines(*_skeletonPrimitive, skeletonLines,
                                     vec4<I32>(),
                                     false);
                parentStates.setTrackedValue(
                    StateTracker<bool>::State::SKELETON_RENDERED, true);
            }
        }
    } else {
        if (_skeletonPrimitive) {
            _skeletonPrimitive->paused(true);
        }
    }

    node->postRender(_parentSGN);
}

void RenderingComponent::registerShaderBuffer(ShaderBufferLocation slot,
                                              vec2<U32> bindRange,
                                              ShaderBuffer& shaderBuffer) {

    GFXDevice::ShaderBufferList::iterator it;
    for (GFXDevice::RenderPackage& pkg : _renderData) {
        GFXDevice::ShaderBufferList::iterator itEnd = std::end(pkg._shaderBuffers);
        it = std::find_if(std::begin(pkg._shaderBuffers), itEnd,
            [slot](const GFXDevice::ShaderBufferBinding& binding)
                    -> bool { return binding._slot == slot; });

        if (it == itEnd) {
           vectorAlg::emplace_back(pkg._shaderBuffers, slot, &shaderBuffer, bindRange);
        } else {
            it->set(slot, &shaderBuffer, bindRange);
        }
    }
}

void RenderingComponent::unregisterShaderBuffer(ShaderBufferLocation slot) {
    for (GFXDevice::RenderPackage& pkg : _renderData) {
        pkg._shaderBuffers.erase(
            std::remove_if(std::begin(pkg._shaderBuffers), std::end(pkg._shaderBuffers),
                [&slot](const GFXDevice::ShaderBufferBinding& binding)
                    -> bool { return binding._slot == slot; }),
            std::end(pkg._shaderBuffers));
    }
}

ShaderProgram* const RenderingComponent::getDrawShader(RenderStage renderStage) {
    return (getMaterialInstance()
                ? _materialInstance->getShaderInfo(renderStage).getProgram()
                : nullptr);
}

U32 RenderingComponent::getDrawStateHash(RenderStage renderStage) {
    if (!getMaterialInstance()) {
        return 0L;
    }
    
    bool shadowStage = renderStage == RenderStage::SHADOW;
    bool depthPass = renderStage == RenderStage::Z_PRE_PASS || shadowStage;
    bool reflectionStage = renderStage == RenderStage::REFLECTION;

    if (!_materialInstance && depthPass) {
        
        return shadowStage
                   ? _parentSGN.getNode()->renderState().getShadowStateBlock()
                   : _parentSGN.getNode()->renderState().getDepthStateBlock();
    }

    RenderStage blockStage = depthPass ? (shadowStage ? RenderStage::SHADOW
                                                      : RenderStage::Z_PRE_PASS)
                                       : (reflectionStage ? RenderStage::REFLECTION
                                                          : RenderStage::DISPLAY);
    I32 variant = 0;

    if (shadowStage) {
        LightType type = LightManager::getInstance().currentShadowCastingLight()->getLightType();
        type == LightType::DIRECTIONAL
               ? 0
               : type == LightType::POINT 
                       ? 1
                       : 2;
    }

    return _materialInstance->getRenderStateBlock(blockStage, variant);
    
}


GFXDevice::RenderPackage&
RenderingComponent::getDrawPackage(const SceneRenderState& sceneRenderState,
                                   RenderStage renderStage) {

    static const U32 SCENE_NODE_LOD0_SQ = Config::SCENE_NODE_LOD0 * Config::SCENE_NODE_LOD0;
    static const U32 SCENE_NODE_LOD1_SQ = Config::SCENE_NODE_LOD1 * Config::SCENE_NODE_LOD1;
    

    GFXDevice::RenderPackage& pkg = _renderData[to_uint(renderStage)];
    pkg.isRenderable(false);
    if (canDraw(sceneRenderState, renderStage) &&
        preDraw(sceneRenderState, renderStage))
    {
        if (_parentSGN.getNode()->getDrawCommands(_parentSGN,
                                                  renderStage,
                                                  sceneRenderState,
                                                  pkg._drawCommands)) {
            F32 cameraDistanceSQ =
                _parentSGN
                    .get<BoundsComponent>()
                    ->getBoundingSphere()
                    .getCenter()
                    .distanceSquared(sceneRenderState
                                     .getCameraConst()
                                     .getEye());

            U8 lodLevelTemp = cameraDistanceSQ > SCENE_NODE_LOD0_SQ
                                    ? cameraDistanceSQ > SCENE_NODE_LOD1_SQ ? 2 : 1
                                    : 0;
            U8 minLoD = to_ubyte(_parentSGN.getNode()->getLODcount() - 1);
            _lodLevel = renderStage == RenderStage::REFLECTION
                                    ? minLoD
                                    : std::min(minLoD, std::max(lodLevelTemp, to_ubyte(0)));

            U32 offset = commandOffset();
            for (GenericDrawCommand& cmd : pkg._drawCommands) {
                cmd.renderWireframe(cmd.renderWireframe() || sceneRenderState.drawWireframe());
                cmd.commandOffset(offset++);
                cmd.cmd().baseInstance = commandIndex();
                cmd.LoD(_lodLevel);
            }
            pkg.isRenderable(!pkg._drawCommands.empty());
        }
    }

    return pkg;
}

GFXDevice::RenderPackage& 
RenderingComponent::getDrawPackage(RenderStage renderStage) {
    return _renderData[to_uint(renderStage)];
}

void RenderingComponent::setActive(const bool state) {
    if (!state) {
        renderSkeleton(false);
        renderBoundingBox(false);
        renderBoundingSphere(false);
    }
    SGNComponent::setActive(state);
}

bool RenderingComponent::updateReflection(const vec3<F32>& camPos, const vec2<F32>& camZPlanes) {
    // Low lod entities don't need up to date reflections
    if (_lodLevel > 1) {
        return false;
    }
    // If we lake a material, we don't use reflections
    Material* mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }
    // If shininess is below a certain threshold, we don't have any reflections 
    if (mat->getShaderData()._shininess < g_MaterialShininessThresholdForReflection) {
        return false;
    }

    GFX_DEVICE.generateCubeMap(mat->reflectionTarget(),
                               0,
                               camPos,
                               vec2<F32>(camZPlanes.x, camZPlanes.y * 0.25f),
                               RenderStage::REFLECTION);

    return true;
}

#ifdef _DEBUG
/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
   
    PhysicsComponent* const transform =
        _parentSGN.get<PhysicsComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()));
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    _axisGizmo->paused(false);
}
#endif
};