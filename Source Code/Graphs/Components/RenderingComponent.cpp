#include "Headers/RenderingComponent.h"

#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

RenderingComponent::RenderingComponent(Material* const materialInstance,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::ANIMATION, parentSGN),
      _lodLevel(0),
      _drawOrder(0),
      _castsShadows(true),
      _receiveShadows(true),
      _renderWireframe(false),
      _renderBoundingBox(false),
      _renderSkeleton(false),
      _materialInstance(materialInstance) {
    _renderData._textureData.reserve(ParamHandler::getInstance().getParam<I32>(
        "rendering.maxTextureSlots", 16));
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
    _axisGizmo->begin(PrimitiveType::LINES);
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

void RenderingComponent::makeTextureResident(const Texture& texture, U8 slot) {
    TextureDataContainer::iterator it;
    it = std::find_if(std::begin(_renderData._textureData),
                      std::end(_renderData._textureData),
                      [&slot](const TextureData& data)
                          -> bool { return data.getHandleLow() == slot; });

    TextureData data = texture.getData();
    data.setHandleLow(static_cast<U32>(slot));

    if (it == std::end(_renderData._textureData)) {
        _renderData._textureData.push_back(data);
    } else {
        *it = data;
    }
}

bool RenderingComponent::canDraw(const SceneRenderState& sceneRenderState,
                                 RenderStage renderStage) {
    Material* mat = getMaterialInstance();
    if (mat) {
        if (!mat->computeShader(
                renderStage, false,
                DELEGATE_BIND(&SceneGraphNode::firstDraw, &_parentSGN))) {
            return false;
        }
    }

    return _parentSGN.getNode()->getDrawState(renderStage);
}

bool RenderingComponent::onDraw(RenderStage currentStage) {
    // Call any pre-draw operations on the SceneNode (refresh VB, update
    // materials, get list of textures, etc)

    _renderData._textureData.resize(0);
    Material* mat = getMaterialInstance();
    if (mat) {
        mat->getTextureData(_renderData._textureData);
    }

    return _parentSGN.getNode()->onDraw(_parentSGN, currentStage);
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

bool RenderingComponent::preDraw(const SceneRenderState& renderState,
                                 RenderStage renderStage) const {
    return _parentSGN.prepareDraw(renderState, renderStage);
}

/// Called after the current node was rendered
void RenderingComponent::postDraw(const SceneRenderState& sceneRenderState,
                                  RenderStage renderStage) {
    SceneNode* const node = _parentSGN.getNode();

#ifdef _DEBUG
    if (sceneRenderState.gizmoState() ==
        SceneRenderState::GizmoState::ALL_GIZMO) {
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (_parentSGN.getNode<Object3D>()->getObjectType() ==
                Object3D::ObjectType::SUBMESH) {
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
        sceneRenderState.objectState() ==
            SceneRenderState::ObjectRenderState::DRAW_BOUNDING_BOX) {
        const BoundingBox& bb = _parentSGN.getBoundingBoxConst();
        GFX_DEVICE.drawBox3D(bb.getMin(), bb.getMax(),
                             vec4<U8>(0, 0, 255, 255));
        node->postDrawBoundingBox(_parentSGN);
    }

    if (_parentSGN.getComponent<AnimationComponent>()) {
        _parentSGN.getComponent<AnimationComponent>()->renderSkeleton();
    }
}

void RenderingComponent::registerShaderBuffer(ShaderBufferLocation slot,
                                              ShaderBuffer& shaderBuffer) {
    GFXDevice::ShaderBufferList::iterator it;
    it = std::find_if(
        std::begin(_renderData._shaderBuffers),
        std::end(_renderData._shaderBuffers),
        [&slot](const std::pair<ShaderBufferLocation, ShaderBuffer*>& binding)
            -> bool { return binding.first == slot; });

    if (it == std::end(_renderData._shaderBuffers)) {
        _renderData._shaderBuffers.push_back(
            std::make_pair(slot, &shaderBuffer));
    } else {
        it->second = &shaderBuffer;
    }
}

void RenderingComponent::unregisterShaderBuffer(ShaderBufferLocation slot) {
    _renderData._shaderBuffers.erase(
        std::remove_if(
            std::begin(_renderData._shaderBuffers),
            std::end(_renderData._shaderBuffers),
            [&slot](
                const std::pair<ShaderBufferLocation, ShaderBuffer*>& binding)
                -> bool { return binding.first == slot; }),
        std::end(_renderData._shaderBuffers));
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

vectorImpl<GenericDrawCommand>& RenderingComponent::getDrawCommands(
    SceneRenderState& sceneRenderState, RenderStage renderStage) {
    _renderData._drawCommands.clear();
    if (canDraw(sceneRenderState, renderStage) &&
        preDraw(sceneRenderState, renderStage))
    {
        _parentSGN.getNode()->getDrawCommands(_parentSGN, renderStage,
                                              sceneRenderState,
                                              _renderData._drawCommands);
    }

    return _renderData._drawCommands;
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
                   GFX_DEVICE.getRenderStage() == RenderStage::SHADOW)
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