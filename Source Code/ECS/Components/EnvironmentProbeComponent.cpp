#include "stdafx.h"

#include "Headers/EnvironmentProbeComponent.h"
#include "Headers/TransformComponent.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {


EnvironmentProbeComponent::EnvironmentProbeComponent(SceneGraphNode* sgn, PlatformContext& context)
    : BaseComponentType<EnvironmentProbeComponent, ComponentType::ENVIRONMENT_PROBE>(sgn, context),
      GUIDWrapper(),
      _aabb(vec3<F32>(-1), vec3<F32>(1)),
      _refaabb(vec3<F32>(-1), vec3<F32>(1))
{
    Scene& parentScene = sgn->sceneGraph()->parentScene();

    EditorComponentField layerField = {};
    layerField._name = "RT Layer index";
    layerField._data = &_rtLayerIndex;
    layerField._type = EditorComponentFieldType::PUSH_TYPE;
    layerField._readOnly = true;
    layerField._serialise = false;
    layerField._basicType = GFX::PushConstantType::UINT;
    layerField._basicTypeSize = GFX::PushConstantSize::WORD;

    getEditorComponent().registerField(MOV(layerField));

    EditorComponentField typeField = {};
    typeField._name = "Is Local";
    typeField._dataGetter = [this](void* dataOut) {
        *static_cast<bool*>(dataOut) = _type == ProbeType::TYPE_LOCAL;
    };
    typeField._dataSetter = [this](const void* data) {
        _type = *static_cast<const bool*>(data) ? ProbeType::TYPE_LOCAL : ProbeType::TYPE_INFINITE;
    };
    typeField._type = EditorComponentFieldType::PUSH_TYPE;
    typeField._readOnly = false;
    typeField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(MOV(typeField));

    EditorComponentField bbField = {};
    bbField._name = "Bounding Box";
    bbField._data = &_refaabb;
    bbField._type = EditorComponentFieldType::BOUNDING_BOX;
    bbField._readOnly = false;
    getEditorComponent().registerField(MOV(bbField));

    EditorComponentField updateRateField = {};
    updateRateField._name = "Update Rate";
    updateRateField._tooltip = "[0...TARGET_FPS]. Every Nth frame. 0 = disabled;";
    updateRateField._data = &_updateRate; 
    updateRateField._type = EditorComponentFieldType::PUSH_TYPE;
    updateRateField._readOnly = false;
    updateRateField._range = { 0.f, Config::TARGET_FRAME_RATE };
    updateRateField._basicType = GFX::PushConstantType::UINT;
    updateRateField._basicTypeSize = GFX::PushConstantSize::BYTE;

    getEditorComponent().registerField(MOV(updateRateField));

    EditorComponentField showBoxField = {};
    showBoxField._name = "Show parallax correction AABB";
    showBoxField._data = &_showParallaxAABB;
    showBoxField._type = EditorComponentFieldType::PUSH_TYPE;
    showBoxField._readOnly = false;
    showBoxField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(MOV(showBoxField));

    getEditorComponent().onChangedCbk([this](std::string_view) {
        _aabb.set(_refaabb.getMin(), _refaabb.getMax());
        _aabb.translate(_parentSGN->get<TransformComponent>()->getPosition());
    });

    Attorney::SceneEnvironmentProbeComponent::registerProbe(parentScene, this);
}

EnvironmentProbeComponent::~EnvironmentProbeComponent()
{
    Attorney::SceneEnvironmentProbeComponent::unregisterProbe(_parentSGN->sceneGraph()->parentScene(), this);
}

SceneGraphNode* EnvironmentProbeComponent::findNodeToIgnore() const noexcept {
    //If we are not a root-level probe. Avoid rendering our parent and children into the reflection
    if (getSGN()->parent() != nullptr) {
        SceneGraphNode* parent = getSGN()->parent();
        while (parent != nullptr) {
            const SceneNodeType type = parent->getNode().type();

            if (type != SceneNodeType::TYPE_TRANSFORM || type != SceneNodeType::TYPE_TRIGGER) {
                return parent;
            }

            // Keep walking up
            parent = parent->parent();
        }
    }

    return nullptr;
}

void EnvironmentProbeComponent::refresh(GFX::CommandBuffer& bufferInOut) {
    if (!_dirty) {
        return;
    }

    if (++_currentUpdateCall % _updateRate == 0) {
        if (_updateRate != 0) {
            rtLayerIndex(SceneEnvironmentProbePool::allocateSlice(false));
        }

        EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand(Util::StringFormat("EnvironmentProbePass Id: [ %d ]", rtLayerIndex()).c_str()));

        vectorEASTL<Camera*>& probeCameras = SceneEnvironmentProbePool::probeCameras();

        std::array<Camera*, 6> cameras = {};
        std::copy_n(std::begin(probeCameras), std::min(cameras.size(), probeCameras.size()), std::begin(cameras));

        RenderPassParams params = {};
        params._target = SceneEnvironmentProbePool::reflectionTarget()._targetID;
        params._sourceNode = findNodeToIgnore();
        params._stagePass = RenderStagePass(RenderStage::REFLECTION, RenderPassType::COUNT, to_U8(ReflectorType::CUBE), rtLayerIndex());
        params._shadowMappingEnabled = false;

        _context.gfx().generateCubeMap(params,
                                       rtLayerIndex(),
                                       _aabb.getCenter(),
                                       vec2<F32>(0.1f, _aabb.getHalfExtent().length()),
                                       bufferInOut,
                                       cameras);

        EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        _currentUpdateCall = 0;
        _dirty = false;
    }
}

void EnvironmentProbeComponent::setBounds(const vec3<F32>& min, const vec3<F32>& max) {
    _aabb.set(min, max);
}

void EnvironmentProbeComponent::setBounds(const vec3<F32>& center, F32 radius) {
    _aabb.createFromSphere(center, radius);
}

void EnvironmentProbeComponent::setUpdateRate(U8 rate) {
    if (_updateRate != rate) {
        if (_updateRate == 0) {
            SceneEnvironmentProbePool::unlockSlice(rtLayerIndex());
        } else if (rate == 0) {
            rtLayerIndex(SceneEnvironmentProbePool::allocateSlice(true));
        }
        _updateRate = rate;
    }
}

F32 EnvironmentProbeComponent::distanceSqTo(const vec3<F32>& pos) const {
    return _aabb.getCenter().distanceSquared(pos);
}

void EnvironmentProbeComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<EnvironmentProbeComponent, ComponentType::ENVIRONMENT_PROBE>;
    if (_drawImpostor || showParallaxAABB()) {
        context().gfx().debugDrawSphere(_aabb.getCenter(), 0.5f, DefaultColours::BLUE);
        context().gfx().debugDrawBox(_aabb.getMin(), _aabb.getMax(), DefaultColours::BLUE);
    }

    Parent::PreUpdate(deltaTime);
}

void EnvironmentProbeComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        _aabb.set(_refaabb.getMin(), _refaabb.getMax());
        _aabb.translate(_parentSGN->get<TransformComponent>()->getPosition());
    } else if (data._type == ECS::CustomEvent::Type::EntityFlagChanged) {
        const SceneGraphNode::Flags flag = static_cast<SceneGraphNode::Flags>(data._flag);
        if (flag == SceneGraphNode::Flags::SELECTED) {
            _drawImpostor = data._dataFirst == 1u;
        }
    }
}

std::array<Camera*, 6> EnvironmentProbeComponent::probeCameras() const noexcept {
    // Because we pool probe cameras, the current look at and projection settings may not  match
    // this probe specifically. Because we update the cameras per refresh call, changing camera settings
    // should be safe.

    static const std::array<vec3<F32>, 6> TabUp ={
          WORLD_Y_NEG_AXIS,
          WORLD_Y_NEG_AXIS,
          WORLD_Z_AXIS,
          WORLD_Z_NEG_AXIS,
          WORLD_Y_NEG_AXIS,
          WORLD_Y_NEG_AXIS
    };

    // Get the center and up vectors for each cube face
    static const std::array<vec3<F32>,6> TabCenter = {
        vec3<F32>( 1.0f,  0.0f,  0.0f), //Pos X
        vec3<F32>(-1.0f,  0.0f,  0.0f), //Neg X
        vec3<F32>( 0.0f,  1.0f,  0.0f), //Pos Y
        vec3<F32>( 0.0f, -1.0f,  0.0f), //Neg Y
        vec3<F32>( 0.0f,  0.0f,  1.0f), //Pos Z
        vec3<F32>( 0.0f,  0.0f, -1.0f)  //Neg Z
    };

    vectorEASTL<Camera*>& probeCameras = SceneEnvironmentProbePool::probeCameras();

    std::array<Camera*, 6> cameras = {};
    std::copy_n(std::begin(probeCameras), std::min(cameras.size(), probeCameras.size()), std::begin(cameras));

    for (U8 i = 0; i < 6; ++i) {
        Camera* camera = cameras[i];
        // Set a 90 degree horizontal FoV perspective projection
        camera->setProjection(1.0f, Angle::to_VerticalFoV(Angle::DEGREES<F32>(90.0f), 1.0f), vec2<F32>(0.1f, _aabb.getHalfExtent().length()));
        // Point our camera to the correct face
        camera->lookAt(_aabb.getCenter(), _aabb.getCenter() + TabCenter[i] * _aabb.getHalfExtent().length(), TabUp[i]);
        camera->updateLookAt();
    }

    return cameras;
}
} //namespace Divide