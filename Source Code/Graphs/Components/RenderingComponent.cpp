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
     _isSubMesh = type == Object3D::ObjectType::SUBMESH;
    _nodeSkinned = parentSGN.getNode<Object3D>()->hasFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);
    if (_materialInstance && !_isSubMesh) {
        _materialInstance->addShaderModifier(RenderStage::SHADOW, "TriangleStrip");
        _materialInstance->setShaderDefines(RenderStage::SHADOW, "USE_TRIANGLE_STRIP");
    }
    for (GFXDevice::RenderPackage& pkg : _renderData) {
        pkg._textureData.reserve(ParamHandler::getInstance().getParam<I32>(_ID("rendering.maxTextureSlots"), 16));
    }

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;

    _boundingBoxPrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingBoxPrimitive->name("BoundingBox_" + parentSGN.getName());
    _boundingBoxPrimitive->stateHash(primitiveStateBlock.getHash());
    
    _boundingSpherePrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.getName());
    _boundingSpherePrimitive->stateHash(primitiveStateBlock.getHash());

    if (_nodeSkinned) {
        primitiveStateBlock.setZReadWrite(false, true);
        _skeletonPrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
        _skeletonPrimitive->name("Skeleton_" + parentSGN.getName());
        _skeletonPrimitive->stateHash(primitiveStateBlock.getHash());
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
    _axisGizmo->beginBatch(true, to_uint(_axisLines.size()) * 2);
    _axisGizmo->attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(_axisLines[0]._colorStart));
    // Set the mode to line rendering
    _axisGizmo->begin(PrimitiveType::LINES);
    // Add every line in the list to the batch
    for (const Line& line : _axisLines) {
        _axisGizmo->attribute4f(to_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(line._colorStart));
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
    _boundingBoxPrimitive->_canZombify = true;
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
        if (mat->isReflective()) {
            if (mat->getTexture(ShaderProgram::TextureUsage::REFLECTION) == nullptr) {
                mat->setTexture(ShaderProgram::TextureUsage::REFLECTION,
                                GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::ENVIRONMENT)->getAttachment());
            }
        }
    }

    Object3D::ObjectType type = _parentSGN.getNode<Object3D>()->getObjectType();
    // Continue only for skinned submeshes
    if (type == Object3D::ObjectType::SUBMESH && _nodeSkinned) {
        _parentSGN.getParent().lock()->getTrackedBools().setTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED, false);
        _skeletonPrimitive->paused(true);
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

bool RenderingComponent::onDraw(RenderStage currentStage) {
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

    return _parentSGN.getNode()->onDraw(_parentSGN, currentStage);
}

void RenderingComponent::renderGeometry(const bool state) {
    _renderGeometry = state;

    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = 
            _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderGeometry(state);
        }
    }
}

void RenderingComponent::renderWireframe(const bool state) {
    _renderWireframe = state;
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderWireframe(state);
        }
    }
}

void RenderingComponent::renderBoundingBox(const bool state) {
    _renderBoundingBox = state;
    if (!state) {
        _boundingBoxPrimitive->paused(true);
    }
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderBoundingBox(state);
        }
    }
}

void RenderingComponent::renderBoundingSphere(const bool state) {
    _renderBoundingSphere = state;
    if (!state) {
        _boundingSpherePrimitive->paused(true);
    }
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderBoundingSphere(state);
        }
    }
}

void RenderingComponent::renderSkeleton(const bool state) {
    _renderSkeleton = state;
    if (!state && _skeletonPrimitive) {
        _skeletonPrimitive->paused(true);
    }
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderSkeleton(state);
        }
    }
}

void RenderingComponent::castsShadows(const bool state) {
    _castsShadows = state;
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->castsShadows(_castsShadows);
        }
    }
}

void RenderingComponent::receivesShadows(const bool state) {
    _receiveShadows = state;
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        RenderingComponent* const renderable = _parentSGN.getChild(i, childCount).getComponent<RenderingComponent>();
        if (renderable) {
            renderable->receivesShadows(_receiveShadows);
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
                      to_float(_lodLevel),
                      0.0);
}

bool RenderingComponent::preDraw(const SceneRenderState& renderState,
                                 RenderStage renderStage) const {
    return _parentSGN.prepareDraw(renderState, renderStage);
}

/// Called after the current node was rendered
void RenderingComponent::postDraw(const SceneRenderState& sceneRenderState, RenderStage renderStage) {
    
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

    // Draw bounding box if needed and only in the final stage to prevent
    // Shadow/PostFX artifacts
    if (renderBoundingBox() || sceneRenderState.drawBoundingBoxes()) {
        const BoundingBox& bb = _parentSGN.getBoundingBoxConst();
        if (bb.isComputed()) {
            GFX_DEVICE.drawBox3D(*_boundingBoxPrimitive, bb.getMin(), bb.getMax(),
                                 vec4<U8>(0, 0, 255, 255));


        }
        node->postDrawBoundingBox(_parentSGN);
        if (_parentSGN.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            renderBoundingSphere(true);
        } else {
            renderBoundingSphere(false);
        }
    } else {
        _boundingBoxPrimitive->paused(true);
        renderBoundingSphere(false);
    }

    
    if (renderBoundingSphere()) {
        const BoundingSphere& bs = _parentSGN.getBoundingSphereConst();
        GFX_DEVICE.drawSphere3D(*_boundingSpherePrimitive, bs.getCenter(), bs.getRadius(),
                                vec4<U8>(0, 255, 0, 255));
    } else {
        _boundingSpherePrimitive->paused(true);
    }

    if (_renderSkeleton || sceneRenderState.drawSkeletons()) {
        // Continue only for skinned submeshes
        if (_isSubMesh && _nodeSkinned) {
            SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
            StateTracker<bool>& parentStates = grandParent->getTrackedBools();
            bool renderSkeletonFlagInitialized = false;
            bool renderSkeleton = parentStates.getTrackedValue(StateTracker<bool>::State::SKELETON_RENDERED,
                                                               renderSkeletonFlagInitialized);
            if (!renderSkeleton || !renderSkeletonFlagInitialized) {
                // Get the animation component of any submesh. They should be synced anyway.
                AnimationComponent* childAnimComp =
                    _parentSGN.getComponent<AnimationComponent>();
                // Get the skeleton lines from the submesh's animation component
                const vectorImpl<Line>& skeletonLines = childAnimComp->skeletonLines();
                _skeletonPrimitive->worldMatrix(_parentSGN.getComponent<PhysicsComponent>()->getWorldMatrix());
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
}

void RenderingComponent::registerShaderBuffer(ShaderBufferLocation slot,
                                              vec2<U32> bindRange,
                                              ShaderBuffer& shaderBuffer) {

    GFXDevice::ShaderBufferList::iterator it;
    for (GFXDevice::RenderPackage& pkg : _renderData) {
        it = std::find_if(std::begin(pkg._shaderBuffers), std::end(pkg._shaderBuffers),
            [&slot](const GFXDevice::ShaderBufferBinding& binding)
                    -> bool { return binding._slot == slot; });

        if (it == std::end(pkg._shaderBuffers)) {
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

ShaderProgram* const RenderingComponent::getDrawShader(
    RenderStage renderStage) {
    return (getMaterialInstance()
                ? _materialInstance->getShaderInfo(renderStage).getProgram()
                : nullptr);
}

U32 RenderingComponent::getDrawStateHash(RenderStage renderStage) {
    if (!getMaterialInstance()) {
        return 0L;
    }

    bool depthPass = GFX_DEVICE.isDepthStage();
    bool shadowStage = GFX_DEVICE.getRenderStage() == RenderStage::SHADOW;
    bool reflectionStage = GFX_DEVICE.getRenderStage() == RenderStage::REFLECTION;

    if (!_materialInstance && depthPass) {
        
        return shadowStage
                   ? _parentSGN.getNode()->renderState().getShadowStateBlock()
                   : _parentSGN.getNode()->renderState().getDepthStateBlock();
    }

    return _materialInstance->getRenderStateBlock(
        depthPass ? (shadowStage ? RenderStage::SHADOW
                                 : RenderStage::Z_PRE_PASS)
                  : (reflectionStage ? RenderStage::REFLECTION
                                     : RenderStage::DISPLAY));
}


GFXDevice::RenderPackage&
RenderingComponent::getDrawPackage(const SceneRenderState& sceneRenderState,
                                   RenderStage renderStage) {

    GFXDevice::RenderPackage& pkg = _renderData[to_uint(renderStage)];
    pkg.isRenderable(false);
    if (canDraw(sceneRenderState, renderStage) &&
        preDraw(sceneRenderState, renderStage))
    {
        if (_parentSGN.getNode()->getDrawCommands(_parentSGN,
                                                  renderStage,
                                                  sceneRenderState,
                                                  pkg._drawCommands)) {
            F32 cameraDistance = 
                _parentSGN
                    .getBoundingSphereConst()
                    .getCenter()
                    .distance(sceneRenderState
                                .getCameraConst()
                                .getEye());

            U8 lodLevelTemp = cameraDistance > Config::SCENE_NODE_LOD0
                                    ? cameraDistance > Config::SCENE_NODE_LOD1 ? 2 : 1
                                    : 0;
            _lodLevel =
                std::min(to_ubyte(_parentSGN.getNode()->getLODcount() - 1),
                    std::max(lodLevelTemp, to_ubyte(0)));

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

void RenderingComponent::boundingBoxUpdatedCallback() {
}

void RenderingComponent::setActive(const bool state) {
    if (!state) {
        renderSkeleton(false);
        renderBoundingBox(false);
        renderBoundingSphere(false);
    }
    SGNComponent::setActive(state);
}

#ifdef _DEBUG
/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
   
    PhysicsComponent* const transform =
        _parentSGN.getComponent<PhysicsComponent>();
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