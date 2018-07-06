#include "config.h"

#include "Headers/RenderingComponent.h"

#include "Core/Headers/Kernel.h"
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
      _drawOrder(0),
      _commandIndex(0),
      _commandOffset(0),
      _preDrawPass(false),
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
    _customShaders.fill(nullptr);

    Object3D_ptr node = parentSGN.getNode<Object3D>();
    Object3D::ObjectType type = node->getObjectType();
    SceneNodeType nodeType = node->getType();

    bool isSubMesh = type == Object3D::ObjectType::SUBMESH;
    bool nodeSkinned = parentSGN.getNode<Object3D>()->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

    if (_materialInstance) {
        assert(!_materialInstance->getName().empty());
        if (!isSubMesh) {
            _materialInstance->addShaderModifier(RenderStage::SHADOW, "TriangleStrip");
            _materialInstance->setShaderDefines(RenderStage::SHADOW, "USE_TRIANGLE_STRIP");
        }
    }

    for (RenderPackage& pkg : _renderData) {
        pkg.isOcclusionCullable(nodeType != SceneNodeType::TYPE_SKY);
    }

    // Prepare it for rendering lines
    RenderStateBlock primitiveStateBlock;

    _boundingBoxPrimitive[0] = _context.newIMP();
    _boundingBoxPrimitive[0]->name("BoundingBox_" + parentSGN.getName());
    _boundingBoxPrimitive[0]->stateHash(primitiveStateBlock.getHash());
    _boundingBoxPrimitive[0]->paused(true);

    _boundingBoxPrimitive[1] = _context.newIMP();
    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + parentSGN.getName());
    _boundingBoxPrimitive[1]->stateHash(primitiveStateBlock.getHash());
    _boundingBoxPrimitive[1]->paused(true);

    _boundingSpherePrimitive = _context.newIMP();
    _boundingSpherePrimitive->name("BoundingSphere_" + parentSGN.getName());
    _boundingSpherePrimitive->stateHash(primitiveStateBlock.getHash());
    _boundingSpherePrimitive->paused(true);

    if (nodeSkinned) {
        primitiveStateBlock.setZRead(false);
        _skeletonPrimitive = _context.newIMP();
        _skeletonPrimitive->name("Skeleton_" + parentSGN.getName());
        _skeletonPrimitive->stateHash(primitiveStateBlock.getHash());
        _skeletonPrimitive->paused(true);
    }
    
    if (Config::Build::IS_DEBUG_BUILD) {

        ResourceDescriptor previewReflectionRefraction("fbPreview");
        previewReflectionRefraction.setThreadedLoading(true);
        _previewRenderTarget = CreateResource<ShaderProgram>(context.parent().resourceCache(), previewReflectionRefraction);

        // Red X-axis
        _axisLines.push_back(
            Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255), 5.0f));
        // Green Y-axis
        _axisLines.push_back(
            Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255), 5.0f));
        // Blue Z-axis
        _axisLines.push_back(
            Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255), 5.0f));
        _axisGizmo = _context.newIMP();
        // Prepare it for line rendering
        size_t noDepthStateBlock = _context.getDefaultStateBlock(true);
        RenderStateBlock stateBlock(RenderStateBlock::get(noDepthStateBlock));
        _axisGizmo->name("AxisGizmo_" + parentSGN.getName());
        _axisGizmo->stateHash(stateBlock.getHash());
        _axisGizmo->paused(true);
        // Create the object containing all of the lines
        _axisGizmo->beginBatch(true, to_uint(_axisLines.size()) * 2, 1);
        _axisGizmo->attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColour(_axisLines[0]._colourStart));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : _axisLines) {
            _axisGizmo->attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColour(line._colourStart));
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

void RenderingComponent::postLoad() {
    for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        RenderPackage& pkg = _renderData[to_uint(static_cast<RenderStage>(i))];
        _parentSGN.getNode()->initialiseDrawCommands(_parentSGN, static_cast<RenderStage>(i), pkg._drawCommands);
    }
}

void RenderingComponent::update(const U64 deltaTime) {
    const Material_ptr& mat = getMaterialInstance();
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

bool RenderingComponent::canDraw(RenderStage renderStage) {
    if (_parentSGN.getNode()->getDrawState(renderStage)) {
        const Material_ptr& mat = getMaterialInstance();
        if (mat) {
            if (!mat->canDraw(renderStage)) {
                return false;
            }
        }
        return true;
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

bool RenderingComponent::onRender(RenderStage currentStage) {
    // Call any pre-draw operations on the SceneNode (refresh VB, update
    // materials, get list of textures, etc)

    RenderPackage& pkg = _renderData[to_uint(currentStage)];

    pkg._textureData.clear(false);
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        mat->getTextureData(pkg._textureData);
    }

    for (const TextureData& texture : _textureDependencies.textures()) {
        pkg._textureData.addTexture(texture);
    }

    return _parentSGN.getNode()->onRender(currentStage);
}

void RenderingComponent::renderGeometry(const bool state) {
    if (_renderGeometry != state) {
        _renderGeometry = state;

        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->renderGeometry(state);
            }
        });
    }
}

void RenderingComponent::renderWireframe(const bool state) {
    if (_renderWireframe != state) {
        _renderWireframe = state;
    
        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->renderWireframe(state);
            }
        });
    }
}

void RenderingComponent::renderBoundingBox(const bool state) {
    if (_renderBoundingBox != state) {
        _renderBoundingBox = state;
        if (!state) {
            _boundingBoxPrimitive[0]->paused(true);
            _boundingBoxPrimitive[1]->paused(true);
        }

        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->renderBoundingBox(state);
            }
        });
    }
}

void RenderingComponent::renderBoundingSphere(const bool state) {
    if (_renderBoundingSphere != state) {
        _renderBoundingSphere = state;
        if (!state) {
            _boundingSpherePrimitive->paused(true);
        }

        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->renderBoundingSphere(state);
            }
        });
    }
}

void RenderingComponent::renderSkeleton(const bool state) {
    if (_renderSkeleton != state) {
        _renderSkeleton = state;
        if (!state && _skeletonPrimitive) {
            _skeletonPrimitive->paused(true);
        }

        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->renderSkeleton(state);
            }
        });
    }
}

void RenderingComponent::castsShadows(const bool state) {
    if (_castsShadows != state) {
        _castsShadows = state;

        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->castsShadows(state);
            }
        });
    }
}

void RenderingComponent::receivesShadows(const bool state) {
    if (_receiveShadows != state) {
        _receiveShadows = state;
    
        _parentSGN.forEachChild([state](const SceneGraphNode& child) {
            RenderingComponent* const renderable = child.get<RenderingComponent>();
            if (renderable) {
                renderable->receivesShadows(state);
            }
        });
    }
}

bool RenderingComponent::castsShadows() const {
    return _castsShadows;
}

bool RenderingComponent::receivesShadows() const {
    return _receiveShadows;
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
                      receivesShadows() ? 1.0f : 0.0f,
                      _lodLevel,
                      0.0);
    const Material_ptr& mat = getMaterialInstance();
    if (mat) {
        reflectionIndex = to_float(mat->defaultReflectionTextureIndex());
        refractionIndex = to_float(mat->defaultRefractionTextureIndex());
    } else {
        reflectionIndex = refractionIndex = 0.0f;
    }
}

/// Called after the current node was rendered
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, RenderStage renderStage, RenderSubPassCmds& subPassesInOut) {
    
    if (renderStage != RenderStage::DISPLAY || _context.isPrePass()) {
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

    // Draw bounding box if needed and only in the final stage to prevent
    // Shadow/PostFX artifacts
    if (renderBoundingBox() || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB)) {
        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive[0]->fromBox(bb.getMin(), bb.getMax(), vec4<U8>(0, 0, 255, 255));

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
                _boundingBoxPrimitive[1]->fromBox(
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
        _boundingSpherePrimitive->fromSphere(bs.getCenter(), bs.getRadius(), vec4<U8>(0, 255, 0, 255));
    } else {
        _boundingSpherePrimitive->paused(true);
    }

    if (_renderSkeleton || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS)) {
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
                _skeletonPrimitive->fromLines(skeletonLines);
                parentStates.setTrackedValue(
                    StateTracker<bool>::State::SKELETON_RENDERED, true);
            }
        }
    } else {
        if (_skeletonPrimitive) {
            _skeletonPrimitive->paused(true);
        }
    }

    RenderSubPassCmd& subPassInOut = subPassesInOut.back();
    subPassInOut._commands.reserve(subPassInOut._commands.size() + 5);

    subPassInOut._commands.push_back(_boundingBoxPrimitive[0]->toDrawCommand());
    subPassInOut._commands.push_back(_boundingBoxPrimitive[1]->toDrawCommand());
    subPassInOut._commands.push_back(_boundingSpherePrimitive->toDrawCommand());
    if (_skeletonPrimitive) {
        subPassInOut._commands.push_back(_skeletonPrimitive->toDrawCommand());
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        subPassInOut._commands.push_back(_axisGizmo->toDrawCommand());
     }
}

void RenderingComponent::registerShaderBuffer(ShaderBufferLocation slot,
                                              vec2<U32> bindRange,
                                              ShaderBuffer& shaderBuffer) {

    ShaderBufferList::iterator it;
    for (RenderPackage& pkg : _renderData) {
        ShaderBufferList::iterator itEnd = std::end(pkg._shaderBuffers);
        it = std::find_if(std::begin(pkg._shaderBuffers), itEnd,
            [slot](const ShaderBufferBinding& binding)
                    -> bool { return binding._slot == slot; });

        if (it == itEnd) {
           vectorAlg::emplace_back(pkg._shaderBuffers, slot, &shaderBuffer, bindRange);
        } else {
            it->set(slot, &shaderBuffer, bindRange);
        }
    }
}

void RenderingComponent::unregisterShaderBuffer(ShaderBufferLocation slot) {
    for (RenderPackage& pkg : _renderData) {
        pkg._shaderBuffers.erase(
            std::remove_if(std::begin(pkg._shaderBuffers), std::end(pkg._shaderBuffers),
                [&slot](const ShaderBufferBinding& binding)
                    -> bool { return binding._slot == slot; }),
            std::end(pkg._shaderBuffers));
    }
}

ShaderProgram_ptr RenderingComponent::getDrawShader(RenderStage renderStage) {
    return (getMaterialInstance()
                ? _materialInstance->getShaderInfo(renderStage).getProgram()
                : _customShaders[to_uint(renderStage)]);
}

size_t RenderingComponent::getDrawStateHash(RenderStage renderStage) {
    bool shadowStage = renderStage == RenderStage::SHADOW;
    bool depthPass = renderStage == RenderStage::Z_PRE_PASS || shadowStage;
    bool reflectionStage = renderStage == RenderStage::REFLECTION;
    bool refractionStage = renderStage == RenderStage::REFRACTION;

    if (!getMaterialInstance() && depthPass) {
        
        return shadowStage
                   ? _parentSGN.getNode()->renderState().getShadowStateBlock()
                   : _parentSGN.getNode()->renderState().getDepthStateBlock();
    }

    if (!_materialInstance) {
        return 0;
    }

    RenderStage blockStage = depthPass ? (shadowStage ? RenderStage::SHADOW
                                                      : RenderStage::Z_PRE_PASS)
                                       : (reflectionStage ? RenderStage::REFLECTION
                                                          : refractionStage ? RenderStage::REFRACTION
                                                                            : RenderStage::DISPLAY);
    I32 variant = 0;
    if (shadowStage) {
        LightType type = LightPool::currentShadowCastingLight()->getLightType();
        variant = (type == LightType::DIRECTIONAL
                         ? 0
                         : type == LightType::POINT 
                                 ? 1
                                 : 2);
    }

    return _materialInstance->getRenderStateBlock(blockStage, variant);
    
}

void RenderingComponent::updateLoDLevel(const Camera& camera, RenderStage renderStage) {
    static const U32 SCENE_NODE_LOD0_SQ = Config::SCENE_NODE_LOD0 * Config::SCENE_NODE_LOD0;
    static const U32 SCENE_NODE_LOD1_SQ = Config::SCENE_NODE_LOD1 * Config::SCENE_NODE_LOD1;

    _lodLevel = to_ubyte(_parentSGN.getNode()->getLODcount() - 1);

    // ToDo: Hack for lower LoD rendering in reflection and refraction passes
    if (renderStage != RenderStage::REFLECTION && renderStage != RenderStage::REFRACTION) {
        const vec3<F32>& eyePos = camera.getEye();
        const BoundingSphere& bSphere = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        F32 cameraDistanceSQ = bSphere.getCenter().distanceSquared(eyePos);

        U8 lodLevelTemp = cameraDistanceSQ > SCENE_NODE_LOD0_SQ
                                           ? cameraDistanceSQ > SCENE_NODE_LOD1_SQ ? 2 : 1
                                           : 0;

        _lodLevel = std::min(_lodLevel, std::max(lodLevelTemp, to_ubyte(0)));
    }
}

void RenderingComponent::prepareDrawPackage(const SceneRenderState& sceneRenderState, RenderStage renderStage) {
    _preDrawPass = canDraw(renderStage) && _parentSGN.prepareDraw(sceneRenderState, renderStage);
}

RenderPackage&
RenderingComponent::getDrawPackage(const SceneRenderState& sceneRenderState, RenderStage renderStage) {
    RenderPackage& pkg = _renderData[to_uint(renderStage)];
    pkg.isRenderable(false);
    if (_preDrawPass)
    {
        for (GenericDrawCommand& cmd : pkg._drawCommands) {
            cmd.renderMask(renderMask());
            cmd.stateHash(getDrawStateHash(renderStage));
            cmd.shaderProgram(getDrawShader(renderStage));
        }

        _parentSGN.getNode()->updateDrawCommands(_parentSGN, renderStage, sceneRenderState, pkg._drawCommands);

        updateLoDLevel(*Camera::activeCamera(), renderStage);

        U32 offset = commandOffset();
        bool sceneRenderWireframe = sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);
        for (GenericDrawCommand& cmd : pkg._drawCommands) {
            bool renderWireframe = cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME) ||
                                   sceneRenderWireframe;
            cmd.toggleOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME, renderWireframe);
            cmd.commandOffset(offset++);
            cmd.cmd().baseInstance = commandIndex();
            cmd.LoD(_lodLevel);
        }
        
        pkg.isRenderable(!pkg._drawCommands.empty());
    }

    return pkg;
}

RenderPackage& 
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

bool RenderingComponent::clearReflection() {
    // If we lake a material, we don't use reflections
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }

    mat->updateReflectionIndex(-1);
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

    mat->updateReflectionIndex(reflectionIndex);

    RenderTargetID reflectRTID(RenderTargetUsage::REFLECTION, reflectionIndex);

    if (Config::Build::IS_DEBUG_BUILD) {
        GFXDevice::DebugView_ptr& viewPtr = _debugViews[0][reflectionIndex];
        if (!viewPtr) {
            viewPtr = std::make_shared<GFXDevice::DebugView>();
            RenderTarget& target = _context.renderTarget(reflectRTID);
            viewPtr->_texture = target.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
            viewPtr->_shader = _previewRenderTarget;
            viewPtr->_shaderData._floatValues.push_back(std::make_pair("lodLevel", 0.0f));
            viewPtr->_shaderData._boolValues.push_back(std::make_pair("linearSpace", false));
            viewPtr->_shaderData._boolValues.push_back(std::make_pair("unpack2Channel", false));
            _context.addDebugView(viewPtr);
        }
    }

    if (_reflectionCallback) {
        RenderCbkParams params(_context, _parentSGN, renderState, reflectRTID, reflectionIndex, camera);
        _reflectionCallback(params);
    } else {
        const vec2<F32>& zPlanes = camera->getZPlanes();
        _context.generateCubeMap(reflectRTID,
                                 0,
                                 camera->getEye(),
                                 vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                 RenderStage::REFLECTION,
                                 reflectionIndex);
    }

    return true;
}

bool RenderingComponent::clearRefraction() {
    const Material_ptr& mat = getMaterialInstance();
    if (mat == nullptr) {
        return false;
    }
    if (!mat->isTranslucent()) {
        return false;
    }
    mat->updateRefractionIndex(-1);
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

    mat->updateRefractionIndex(refractionIndex);

    RenderTargetID refractRTID(RenderTargetUsage::REFRACTION, refractionIndex);
    if (Config::Build::IS_DEBUG_BUILD) {
        GFXDevice::DebugView_ptr& viewPtr = _debugViews[1][refractionIndex];
        if (!viewPtr) {
            viewPtr = std::make_shared<GFXDevice::DebugView>();
            RenderTarget& target = _context.renderTarget(refractRTID);
            viewPtr->_texture = target.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
            viewPtr->_shader = _previewRenderTarget;
            viewPtr->_shaderData._floatValues.push_back(std::make_pair("lodLevel", 0.0f));
            viewPtr->_shaderData._boolValues.push_back(std::make_pair("linearSpace", false));
            viewPtr->_shaderData._boolValues.push_back(std::make_pair("unpack2Channel", false));
            _context.addDebugView(viewPtr);
        }
    }

    RenderCbkParams params(_context, _parentSGN, renderState, refractRTID, refractionIndex, camera);
    _refractionCallback(params);

    return true;
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
    mat->defaultReflectionTexture(rt->getAttachment(RTAttachment::Type::Colour, 0).asTexture(),
                                  _envProbes.front()->getRTIndex());
}

/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
    if (!Config::Build::IS_DEBUG_BUILD) {
        return;
    }

    PhysicsComponent* const transform = _parentSGN.get<PhysicsComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()));
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    _axisGizmo->paused(false);
}

};
