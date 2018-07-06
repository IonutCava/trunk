#include "Headers/RenderingComponent.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

RenderingComponent::RenderingComponent(Material* const materialInstance,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::SGN_COMP_ANIMATION, parentSGN),
      _lodLevel(0),
      _castsShadows(true),
      _receiveShadows(true),
      _renderWireframe(false),
      _renderBoundingBox(false),
      _renderSkeleton(false),
      _materialInstance(materialInstance) {
#ifdef _DEBUG
    // Red X-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255)));
    // Green Y-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255)));
    // Blue Z-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255)));
    _axisGizmo = GFX_DEVICE.getOrCreatePrimitive(false);
    // Prepare it for line rendering
    _axisGizmo->_hasLines = true;
    _axisGizmo->_lineWidth = 5.0f;
    _axisGizmo->stateHash(GFX_DEVICE.getDefaultStateBlock(true));
    _axisGizmo->paused(true);
    // Create the object containing all of the lines
    _axisGizmo->beginBatch();
    _axisGizmo->attribute4ub("inColorData", _axisLines[0]._color);
    // Set the mode to line rendering
    _axisGizmo->begin(LINES);
    // Add every line in the list to the batch
    for (const Line& line : _axisLines) {
        _axisGizmo->attribute4ub("inColorData", line._color);
        _axisGizmo->vertex(line._startPoint);
        _axisGizmo->vertex(line._endPoint);
    }
    _axisGizmo->end();
    // Finish our object
    _axisGizmo->endBatch();
#endif
}

RenderingComponent::~RenderingComponent() {
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
}

bool RenderingComponent::onDraw(RenderStage currentStage) {
    // Call any pre-draw operations on the SceneNode (refresh VB, update
    // materials, etc)
    Material* mat = getMaterialInstance();
    if (mat) {
        if (!mat->computeShader(currentStage, false,
                                DELEGATE_BIND(&SceneGraphNode::scheduleReset,
                                              &_parentSGN, currentStage))) {
            return false;
        }
        if (mat->getShaderInfo(currentStage)._shaderCompStage !=
            Material::ShaderInfo::SHADER_STAGE_COMPUTED) {
            return false;
        }
    }
    if (!_parentSGN.getNode()->onDraw(_parentSGN, currentStage)) {
        return false;
    }
    return _parentSGN.getNode()->getDrawState(currentStage);
}

void RenderingComponent::renderWireframe(const bool state) {
    _renderWireframe = state;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        RenderingComponent* const renderable =
            it.second->getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderWireframe(_renderWireframe);
        }
    }
}

void RenderingComponent::renderBoundingBox(const bool state) {
    _renderBoundingBox = state;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        RenderingComponent* const renderable =
            it.second->getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderBoundingBox(_renderBoundingBox);
        }
    }
}

void RenderingComponent::renderSkeleton(const bool state) {
    _renderSkeleton = state;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        RenderingComponent* const renderable =
            it.second->getComponent<RenderingComponent>();
        if (renderable) {
            renderable->renderSkeleton(_renderSkeleton);
        }
    }
}

void RenderingComponent::castsShadows(const bool state) {
    _castsShadows = state;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        RenderingComponent* const renderable =
            it.second->getComponent<RenderingComponent>();
        if (renderable) {
            renderable->castsShadows(_castsShadows);
        }
    }
}

void RenderingComponent::receivesShadows(const bool state) {
    _receiveShadows = state;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        RenderingComponent* const renderable =
            it.second->getComponent<RenderingComponent>();
        if (renderable) {
            renderable->receivesShadows(_receiveShadows);
        }
    }
}

bool RenderingComponent::castsShadows() const {
    return _castsShadows && LightManager::getInstance().shadowMappingEnabled();
}

bool RenderingComponent::receivesShadows() const {
    return _receiveShadows &&
           LightManager::getInstance().shadowMappingEnabled();
}

/// Called after the current node was rendered
void RenderingComponent::postDraw(const SceneRenderState& sceneRenderState,
                                  RenderStage renderStage) {
    SceneNode* const node = _parentSGN.getNode();
    // Perform any post draw operations regardless of the draw state
    SceneNodeRenderAttorney::postDraw(*node, _parentSGN, renderStage);

#ifdef _DEBUG
    if (sceneRenderState.gizmoState() == SceneRenderState::ALL_GIZMO) {
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (_parentSGN.getNode<Object3D>()->getObjectType() ==
                Object3D::MESH) {
                drawDebugAxis();
            }
        }
    } else {
        if (!_parentSGN.isSelected()) {
            _axisGizmo->paused(true);
        }
    }
#endif
    // Draw bounding box if needed and only in the final stage to prevent
    // Shadow/PostFX artifacts
    if (renderBoundingBox() ||
        bitCompare(sceneRenderState.objectState(),
                   SceneRenderState::DRAW_BOUNDING_BOX)) {
        const BoundingBox& bb = _parentSGN.getBoundingBoxConst();
        GFX_DEVICE.drawBox3D(bb.getMin(), bb.getMax(),
                             vec4<U8>(0, 0, 255, 255));
        node->postDrawBoundingBox(_parentSGN);
    }

    if (_parentSGN.getComponent<AnimationComponent>()) {
        _parentSGN.getComponent<AnimationComponent>()->renderSkeleton();
    }
}

void RenderingComponent::render(const SceneRenderState& sceneRenderState,
                                const RenderStage& currentRenderStage) {
    // Call any pre-draw operations on the SceneGraphNode (e.g. tick animations)
    // Check if we should draw the node. (only after onDraw as it may contain
    // exclusion mask changes before draw)
    if (!_parentSGN.prepareDraw(sceneRenderState, currentRenderStage)) {
        // If the SGN isn't ready for rendering, skip it this frame
        return;
    }
    if (getMaterialInstance()) {
        getMaterialInstance()->bindTextures();
    }
    SceneNodeRenderAttorney::render(*_parentSGN.getNode(), _parentSGN,
                                    sceneRenderState, currentRenderStage);

    postDraw(sceneRenderState, currentRenderStage);
}

U8 RenderingComponent::lodLevel() const {
    return (_lodLevel < (_parentSGN.getNode()->getLODcount() - 1)
                ? _lodLevel
                : (_parentSGN.getNode()->getLODcount() - 1));
}

void RenderingComponent::lodLevel(U8 LoD) {
    _lodLevel =
        std::min(static_cast<U8>(_parentSGN.getNode()->getLODcount() - 1),
                 std::max(LoD, static_cast<U8>(0)));
}

ShaderProgram* const RenderingComponent::getDrawShader(
    RenderStage renderStage) {
    return (getMaterialInstance()
                ? _materialInstance->getShaderInfo(renderStage).getProgram()
                : nullptr);
}

size_t RenderingComponent::getDrawStateHash(RenderStage renderStage) {
    if (!getMaterialInstance()) {
        return 0L;
    }

    bool depthPass = GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE);
    bool shadowStage = GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE);

    if (!_materialInstance && depthPass) {
        return shadowStage
                   ? _parentSGN.getNode()->renderState().getShadowStateBlock()
                   : _parentSGN.getNode()->renderState().getDepthStateBlock();
    }

    bool reflectionStage = GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE);

    return _materialInstance->getRenderStateBlock(
        depthPass ? (shadowStage ? SHADOW_STAGE : Z_PRE_PASS_STAGE)
                  : (reflectionStage ? REFLECTION_STAGE : FINAL_STAGE));
}

const vectorImpl<GenericDrawCommand>& RenderingComponent::getDrawCommands(
    vectorAlg::vecSize commandOffset, SceneRenderState& sceneRenderState,
    RenderStage renderStage) {
    _drawCommandsCache.clear();
    _parentSGN.getNode()->getDrawCommands(
        _parentSGN, renderStage, sceneRenderState, _drawCommandsCache);
    vectorAlg::vecSize i = 0;
    for (GenericDrawCommand& cmd : _drawCommandsCache) {
        cmd.drawID(static_cast<I32>(commandOffset + i++));
    }
    return _drawCommandsCache;
}

void RenderingComponent::inViewCallback() {
    _materialColorMatrix.zero();
    _materialPropertyMatrix.zero();

    _materialPropertyMatrix.setCol(
        0, vec4<F32>(_parentSGN.isSelected() ? 1.0f : 0.0f,
                     receivesShadows() ? 1.0f : 0.0f,
                     static_cast<F32>(lodLevel()), 0.0f));

    Material* mat = getMaterialInstance();

    if (mat) {
        mat->getMaterialMatrix(_materialColorMatrix);
        bool isTranslucent =
            mat->isTranslucent()
                ? (mat->useAlphaTest() ||
                   GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE))
                : false;
        _materialPropertyMatrix.setCol(
            1, vec4<F32>(
                   isTranslucent ? 1.0f : 0.0f, (F32)mat->getTextureOperation(),
                   (F32)mat->getTextureCount(), mat->getParallaxFactor()));
    }
}

#ifdef _DEBUG
/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
    PhysicsComponent* const transform =
        _parentSGN.getComponent<PhysicsComponent>();
    if (transform) {
        mat4<F32> tempOffset(getMatrix(transform->getOrientation()));
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    _axisGizmo->paused(false);
}
#endif
};