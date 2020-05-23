#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"
#include "Headers/BoundsComponent.h"
#include "Headers/AnimationComponent.h"
#include "Headers/TransformComponent.h"
#include "Headers/EnvironmentProbeComponent.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Editor/Headers/Editor.h"

#include "Managers/Headers/SceneManager.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"

namespace Divide {

namespace {
    constexpr U8 MAX_LOD_LEVEL = 4u;

    constexpr I16 g_renderRangeLimit = std::numeric_limits<I16>::max();

    inline I32 getUsageIndex(RenderTargetUsage usage) noexcept {
        for (I32 i = 0; i < g_texUsage.size(); ++i) {
            if (g_texUsage[i].first == usage) {
                return i;
            }
        }

        return -1;
    }
};

hashMap<U32, DebugView*> RenderingComponent::s_debugViews[2];

RenderingComponent::RenderingComponent(SceneGraphNode& parentSGN, PlatformContext& context) 
    : BaseComponentType<RenderingComponent, ComponentType::RENDERING>(parentSGN, context),
      _context(context.gfx()),
      _config(context.config())
{
    _lodLevels.fill(0u);
    _lodLockLevels.fill({ false, to_U8(0u) });

    _renderRange.min = -g_renderRangeLimit;
    _renderRange.max =  g_renderRangeLimit;

    instantiateMaterial(parentSGN.getNode().getMaterialTpl());

    toggleRenderOption(RenderOptions::RENDER_GEOMETRY, true);
    toggleRenderOption(RenderOptions::CAST_SHADOWS, true);
    toggleRenderOption(RenderOptions::RECEIVE_SHADOWS, true);
    toggleRenderOption(RenderOptions::IS_VISIBLE, true);

    defaultReflectionTexture(nullptr, 0u);
    defaultRefractionTexture(nullptr, 0u);

    EditorComponentField vaxisField = {};
    vaxisField._name = "Show Axis";
    vaxisField._data = &_showAxis;
    vaxisField._type = EditorComponentFieldType::PUSH_TYPE;
    vaxisField._basicType = GFX::PushConstantType::BOOL;
    vaxisField._readOnly = false;
    _editorComponent.registerField(std::move(vaxisField));

    _editorComponent.onChangedCbk([this](std::string_view field) {
        if (field == "Show Axis") {
            toggleRenderOption(RenderingComponent::RenderOptions::RENDER_AXIS, _showAxis);
        }
    });

    RenderStateBlock primitiveStateBlock = {};
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();

    const SceneNode& node = _parentSGN.getNode();
    if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
        // Do not cull the sky
        if (static_cast<const Object3D&>(node).type() == SceneNodeType::TYPE_SKY) {
            cullFlag(-1.0f);
        }
        // Prepare it for rendering lines
        if (static_cast<const Object3D&>(node).getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            RenderStateBlock primitiveStateBlockNoZRead = {};
            primitiveStateBlockNoZRead.depthTestEnabled(false);
            pipelineDescriptor._stateHash = primitiveStateBlockNoZRead.getHash();
            _primitivePipeline[1] = _context.newPipeline(pipelineDescriptor);
        }
    }

    _primitivePipeline[0] = _context.newPipeline(pipelineDescriptor);

    if_constexpr (Config::Build::ENABLE_EDITOR) {
        // Prepare it for line rendering
        pipelineDescriptor._stateHash = _context.getDefaultStateBlock(true);
        _primitivePipeline[2] = _context.newPipeline(pipelineDescriptor);
    }

    for (U8 s = 0; s < to_U8(RenderStage::COUNT); ++s) {
        if (s == to_U8(RenderStage::SHADOW)) {
            _renderPackages[s][to_base(RenderPassType::MAIN_PASS)].resize(
                RenderStagePass::passCountForStage(
                    RenderStagePass {
                        static_cast<RenderStage>(s),
                        RenderPassType::MAIN_PASS
                    }
                )
            );
        } else {
            PackagesPerPassType & perPassPkgs = _renderPackages[s];
            for (U8 p = 0; p < to_U8(RenderPassType::COUNT); ++p) {
                perPassPkgs[p].resize(
                    RenderStagePass::passCountForStage(
                        RenderStagePass {
                            static_cast<RenderStage>(s),
                            static_cast<RenderPassType>(p)
                        }
                    )
                );
            }
        }
    }
}

RenderingComponent::~RenderingComponent()
{
    if (_boundingBoxPrimitive) {
        _context.destroyIMP(_boundingBoxPrimitive);
    }

    if (_boundingSpherePrimitive) {
        _context.destroyIMP(_boundingSpherePrimitive);
    }

    if (_skeletonPrimitive) {
        _context.destroyIMP(_skeletonPrimitive);
    }

    if (_axisGizmo) {
        _context.destroyIMP(_axisGizmo);
    }

    if (_selectionGizmo) {
        _context.destroyIMP(_selectionGizmo);
    }
}

void RenderingComponent::instantiateMaterial(const Material_ptr& material) {
    if (material == nullptr) {
        return;
    }

    _materialInstance = material->clone(_parentSGN.name() + "_i");
    if (_materialInstance != nullptr) {
        assert(!_materialInstance->resourceName().empty());

        EditorComponentField materialField = {};
        materialField._name = "Material";
        materialField._data = _materialInstance.get();
        materialField._type = EditorComponentFieldType::MATERIAL;
        materialField._readOnly = false;
        // should override any existing entry
        _editorComponent.registerField(std::move(materialField));

        EditorComponentField lockLodField = {};
        lockLodField._name = "Rendered LOD Level";
        lockLodField._type = EditorComponentFieldType::PUSH_TYPE;
        lockLodField._basicTypeSize = GFX::PushConstantSize::BYTE;
        lockLodField._basicType = GFX::PushConstantType::UINT;
        lockLodField._data = &_lodLevels[to_base(RenderStage::DISPLAY)];
        lockLodField._readOnly = true;
        lockLodField._serialise = false;
        _editorComponent.registerField(std::move(lockLodField));

        EditorComponentField renderLodField = {};
        renderLodField._name = "Lock LoD";
        renderLodField._type = EditorComponentFieldType::PUSH_TYPE;
        renderLodField._basicType = GFX::PushConstantType::BOOL;
        renderLodField._data = &_lodLockLevels[to_base(RenderStage::DISPLAY)].first;
        renderLodField._readOnly = false;
        _editorComponent.registerField(std::move(renderLodField));

        EditorComponentField lockLodLevelField = {};
        lockLodLevelField._name = "Lock LoD Level";
        lockLodLevelField._type = EditorComponentFieldType::PUSH_TYPE;
        lockLodLevelField._range = { 0.0f, to_F32(MAX_LOD_LEVEL) };
        lockLodLevelField._basicType = GFX::PushConstantType::UINT;
        lockLodLevelField._basicTypeSize = GFX::PushConstantSize::BYTE;
        lockLodLevelField._data = &_lodLockLevels[to_base(RenderStage::DISPLAY)].second;
        lockLodLevelField._readOnly = false;
        _editorComponent.registerField(std::move(lockLodLevelField));

        _materialInstance->isStatic(_parentSGN.usageContext() == NodeUsageContext::NODE_STATIC);
    }
}

void RenderingComponent::setMinRenderRange(F32 minRange) noexcept {
    _renderRange.min = std::max(minRange, -1.0f * g_renderRangeLimit);
}

void RenderingComponent::setMaxRenderRange(F32 maxRange) noexcept {
    _renderRange.max = std::min(maxRange,  1.0f * g_renderRangeLimit);
}

void RenderingComponent::rebuildDrawCommands(const RenderStagePass& stagePass, const Camera& crtCamera, RenderPackage& pkg) {
    OPTICK_EVENT();
    pkg.clear();

    // The following commands are needed for material rendering
    // In the absence of a material, use the SceneNode buildDrawCommands to add all of the needed commands
    if (_materialInstance != nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _materialInstance->getRenderStateBlock(stagePass);
        pipelineDescriptor._shaderProgramHandle = _materialInstance->getProgramGUID(stagePass);

        pkg.add(GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

        GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand = {};
        bindDescriptorSetsCommand._set.addShaderBuffers(getShaderBuffers());
        pkg.add(bindDescriptorSetsCommand);
    }

    _parentSGN.getNode().buildDrawCommands(_parentSGN, stagePass, crtCamera, pkg);
}

void RenderingComponent::Update(const U64 deltaTimeUS) {
    if (_materialInstance != nullptr && _materialInstance->update(deltaTimeUS)) {
        onMaterialChanged();
    }

    SceneGraphNode* parent = _parentSGN.parent();
    if (parent != nullptr && !parent->hasFlag(SceneGraphNode::Flags::MESH_POST_RENDERED)) {
        parent->clearFlag(SceneGraphNode::Flags::MESH_POST_RENDERED);
    }

    BaseComponentType<RenderingComponent, ComponentType::RENDERING>::Update(deltaTimeUS);
}

void RenderingComponent::onMaterialChanged() {
    OPTICK_EVENT();

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        if (s == to_U8(RenderStage::SHADOW)) {
            continue;
        }
        PackagesPerPassType& perPassPkg = _renderPackages[s];
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            PackagesPerIndex& perIndexPkg = perPassPkg[p];
            for (RenderPackage& pkg : perIndexPkg) {
                pkg.textureDataDirty(true);
            }
        }
    }
}

bool RenderingComponent::canDraw(const RenderStagePass& renderStagePass, U8 LoD, bool refreshData) {
    OPTICK_EVENT();
    OPTICK_TAG("Node", (_parentSGN.name().c_str()));

    if (Attorney::SceneGraphNodeComponent::getDrawState(_parentSGN, renderStagePass, LoD)) {
        // Can we render without a material? Maybe. IDK.
        if (_materialInstance == nullptr || _materialInstance->canDraw(renderStagePass)) {
            return renderOptionEnabled(RenderOptions::IS_VISIBLE);
        }
    }

    return false;
}

void RenderingComponent::onParentUsageChanged(NodeUsageContext context) {
    if (_materialInstance != nullptr) {
        _materialInstance->isStatic(context == NodeUsageContext::NODE_STATIC);
    }
}

void RenderingComponent::rebuildMaterial() {
    if (_materialInstance != nullptr) {
        _materialInstance->rebuild();
        onMaterialChanged();
    }

    _parentSGN.forEachChild([](const SceneGraphNode* child, I32 /*childIdx*/) {
        RenderingComponent* const renderable = child->get<RenderingComponent>();
        if (renderable) {
            renderable->rebuildMaterial();
        }
        return true;
    });
}

void RenderingComponent::prepareRender(const RenderStagePass& renderStagePass) {
    OPTICK_EVENT();

    RenderPackage& pkg = getDrawPackage(renderStagePass);
    if (pkg.textureDataDirty()) {
        TextureDataContainer<>& textures = pkg.descriptorSet(0)._textureData;

        if (_materialInstance != nullptr) {
            _materialInstance->getTextureData(renderStagePass, textures);
        }

        if (!renderStagePass.isDepthPass()) {
            for (U8 i = 0; i < _externalTextures.size(); ++i) {
                const Texture_ptr& crtTexture = _externalTextures[i];
                if (crtTexture != nullptr) {
                    textures.setTexture(crtTexture->data(), g_texUsage[i].second);
                }
            }
        }

        pkg.textureDataDirty(false);
    }
}

bool RenderingComponent::onRefreshNodeData(RefreshNodeDataParams& refreshParams, const TargetDataBufferParams& bufferParams, bool quick) {
    OPTICK_EVENT();

    Attorney::SceneGraphNodeComponent::onRefreshNodeData(_parentSGN, *refreshParams._stagePass, *bufferParams._camera, !quick, *bufferParams._targetBuffer);

    if (quick) {
        // Nothing yet
        return true;
    }

    RenderPackage& pkg = getDrawPackage(*refreshParams._stagePass);
    if (pkg.empty()) {
        return false;
    }

    const I32 drawCommandCount = pkg.drawCommandCount();
    if (drawCommandCount == 0) {
        return false;
    }

    RenderStagePass tempStagePass = *refreshParams._stagePass;
    const RenderStage stage = tempStagePass._stage;

    const U8 lodLevel = _lodLevels[to_base(stage)];

    const U32 startOffset = to_U32(refreshParams._drawCommandsInOut->size());
    const U32 dataIndex = bufferParams._dataIndex;

    Attorney::RenderPackageRenderingComponent::updateDrawCommands(pkg, dataIndex, startOffset, lodLevel);
    if (stage != RenderStage::SHADOW) {
        const RenderPassType passType = tempStagePass._passType;
        for (U8 i = 0; i < to_base(RenderPassType::COUNT); ++i) {
            if (i == to_U8(passType)) {
                continue;
            }

            tempStagePass._passType = static_cast<RenderPassType>(i);
            Attorney::RenderPackageRenderingComponent::updateDrawCommands(getDrawPackage(tempStagePass), dataIndex, startOffset, lodLevel);
        }
    }

    for (I32 i = 0; i < drawCommandCount; ++i) {
        refreshParams._drawCommandsInOut->push_back(pkg.drawCommand(i, 0)._cmd);
    }

    return true;
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    if (_materialInstance != nullptr) {
        _materialInstance->getMaterialMatrix(matOut);
        Texture* refTexture = reflectionTexture();
        if (refTexture != nullptr) {
            matOut.element(1, 3) = to_F32(refTexture->width());
        }
    }
}

void RenderingComponent::getRenderingProperties(const RenderStagePass& stagePass, NodeRenderingProperties& propertiesOut) const {
    propertiesOut._isHovered = _parentSGN.hasFlag(SceneGraphNode::Flags::HOVERED);
    propertiesOut._isSelected = _parentSGN.hasFlag(SceneGraphNode::Flags::SELECTED);
    propertiesOut._receivesShadows = _config.rendering.shadowMapping.enabled &&
                                     renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS);
    propertiesOut._lod = _lodLevels[to_base(stagePass._stage)];
    propertiesOut._cullFlagValue = cullFlag();
    propertiesOut._reflectionIndex = defaultReflectionTextureIndex();
    propertiesOut._refractionIndex = defaultRefractionTextureIndex();
    if (_materialInstance != nullptr) {
        propertiesOut._texOperation = _materialInstance->textureOperation();
        propertiesOut._bumpMethod = _materialInstance->bumpMethod();
    }
}

/// Called after the current node was rendered
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, GFX::CommandBuffer& bufferInOut) {
    if (renderStagePass._stage != RenderStage::DISPLAY || renderStagePass._passType == RenderPassType::PRE_PASS) {
        return;
    }

    // Draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    const bool renderBBox = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);
    const bool renderBSphere = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
    const bool renderSkeleton = renderOptionEnabled(RenderOptions::RENDER_SKELETON) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);
    const bool renderSelection = renderOptionEnabled(RenderOptions::RENDER_SELECTION);
    const bool renderselectionGizmo = renderOptionEnabled(RenderOptions::RENDER_AXIS) || (sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::SELECTION_GIZMO) && _parentSGN.hasFlag(SceneGraphNode::Flags::SELECTED)) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::ALL_GIZMOS);

    if (renderselectionGizmo) {
        drawDebugAxis(bufferInOut);
    } else if (_axisGizmo) {
        _context.destroyIMP(_axisGizmo);
    }
    if (renderSelection) {
        drawSelectionGizmo(bufferInOut);
    } else if (_selectionGizmo) {
        _context.destroyIMP(_selectionGizmo);
    }

    drawBounds(renderBBox, renderBSphere, bufferInOut);
    if (renderSkeleton) {
        drawSkeleton(bufferInOut);
    }

    SceneGraphNode* parent = _parentSGN.parent();
    if (parent != nullptr && !parent->hasFlag(SceneGraphNode::Flags::MESH_POST_RENDERED)) {
        parent->setFlag(SceneGraphNode::Flags::MESH_POST_RENDERED);
        RenderingComponent* rComp = parent->get<RenderingComponent>();
        if (rComp != nullptr) {
            rComp->postRender(sceneRenderState, renderStagePass, bufferInOut);
        }
    }
}

U8 RenderingComponent::getLoDLevel(const BoundsComponent& bComp, const vec3<F32>& cameraEye, RenderStage renderStage, const vec4<U16>& lodThresholds) {
    OPTICK_EVENT();

    const auto[state, level] = _lodLockLevels[to_base(renderStage)];

    if (state) {
        return CLAMPED(level, to_U8(0u), MAX_LOD_LEVEL);
    }

    const F32 distSQtoCenter = std::max(bComp.getBoundingSphere().getCenter().distanceSquared(cameraEye), EPSILON_F32);

    if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.x))) {
        return 0u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.y))) {
        return 1u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.z))) {
        return 2u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.w))) {
        return 3u;
    }

    return MAX_LOD_LEVEL;
}

bool RenderingComponent::prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, bool refreshData) {
    OPTICK_EVENT();

    U8& lod = _lodLevels[to_base(renderStagePass._stage)];
    if (refreshData) {
        BoundsComponent* bComp = _parentSGN.get<BoundsComponent>();
        if (bComp != nullptr) {
            lod = getLoDLevel(*bComp, camera.getEye(), renderStagePass._stage, sceneRenderState.lodThresholds(renderStagePass._stage));
            const bool renderBS = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB);
            const bool renderAABB = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE);

            if (renderAABB != bComp->showAABB()) {
                bComp->showAABB(renderAABB);
            }
            if (renderBS != bComp->showBS()) {
                bComp->showAABB(renderBS);
            }
        }
    }

    if (canDraw(renderStagePass, lod, refreshData)) {
        RenderPackage& pkg = getDrawPackage(renderStagePass);

        if (pkg.empty() || _parentSGN.getNode().rebuildDrawCommands()) {
            rebuildDrawCommands(renderStagePass, camera, pkg);
            _parentSGN.getNode().rebuildDrawCommands(false);
        }

        if (Attorney::SceneGraphNodeComponent::prepareRender(_parentSGN, *this, camera, renderStagePass, refreshData)) {
            prepareRender(renderStagePass);

            pkg.setDrawOption(CmdRenderOptions::RENDER_GEOMETRY,  (renderOptionEnabled(RenderOptions::RENDER_GEOMETRY) &&
                                                                    sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)));
            pkg.setDrawOption(CmdRenderOptions::RENDER_WIREFRAME, (renderOptionEnabled(RenderOptions::RENDER_WIREFRAME) ||
                                                                    sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME)));
        }

        return true;
    }

    return false;
}

RenderPackage& RenderingComponent::getDrawPackage(const RenderStagePass& renderStagePass) {
    const U8 s = to_U8(renderStagePass._stage);
    const U8 p = to_U8(renderStagePass._stage == RenderStage::SHADOW ? RenderPassType::MAIN_PASS : renderStagePass._passType);
    const U16 i = RenderStagePass::indexForStage(renderStagePass);

    return _renderPackages[s][p][i];
}

const RenderPackage& RenderingComponent::getDrawPackage(const RenderStagePass& renderStagePass) const {
    const U8 s = to_U8(renderStagePass._stage);
    const U8 p = to_U8(renderStagePass._stage == RenderStage::SHADOW ? RenderPassType::MAIN_PASS : renderStagePass._passType);
    const U16 i = RenderStagePass::indexForStage(renderStagePass);

    return _renderPackages[s][p][i];
}

size_t RenderingComponent::getSortKeyHash(const RenderStagePass& renderStagePass) const {
    const RenderPackage& pkg = getDrawPackage(renderStagePass);
    return (pkg.empty() ? 0 : pkg.getSortKeyHash());
}

Texture* RenderingComponent::reflectionTexture() const {
    return _externalTextures[getUsageIndex(_reflectorType == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                                   : RenderTargetUsage::REFLECTION_CUBE)].get();
}

void RenderingComponent::updateReflectionIndex(ReflectorType type, I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget = _context.renderTargetPool().renderTarget(RenderTargetID(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                                                                               : RenderTargetUsage::REFLECTION_CUBE,
                                                     to_U16(index)));
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                      : RenderTargetUsage::REFLECTION_CUBE)] = refTex;
    } else {
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
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
    if (_reflectorType == ReflectorType::COUNT) {
        return false;
    }

    updateReflectionIndex(_reflectorType, reflectionIndex);

    RenderTargetID reflectRTID(_reflectorType == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                       : RenderTargetUsage::REFLECTION_CUBE, 
                               reflectionIndex);

    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(reflectRTID);

        DebugView* debugView = s_debugViews[0][reflectionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set(_ID("lodLevel"), GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set(_ID("unpack1Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("unpack2Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("startOnBlue"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("multiplier"), GFX::PushConstantType::FLOAT, 1.0f);
            viewPtr->_name = Util::StringFormat("REFLECTION_%d_%s", reflectRTID._index, _parentSGN.name().c_str());
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
        RenderCbkParams params(_context, _parentSGN, renderState, reflectRTID, reflectionIndex, to_U8(_reflectorType), camera);
        _reflectionCallback(params, bufferInOut);
    } else {
        if (_reflectorType == ReflectorType::CUBE) {
            static std::array<Camera*, 6> cameras = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

            const vec2<F32>& zPlanes = camera->getZPlanes();
            _context.generateCubeMap(reflectRTID,
                                     0,
                                     camera->getEye(),
                                     vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                     RenderStagePass{RenderStage::REFLECTION, RenderPassType::MAIN_PASS, to_U8(_reflectorType), reflectionIndex},
                                     bufferInOut,
                                     cameras,
                                     true);
        }
    }

    return true;
}

void RenderingComponent::updateRefractionIndex(ReflectorType type, I32 index) {
    _refractionIndex = index;
    if (_refractionIndex > -1) {
        RenderTarget& refractionTarget = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::REFRACTION_PLANAR, to_U16(index)));
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
    if (_refractorType == RefractorType::COUNT) {
        return false;
    }

    updateRefractionIndex(_reflectorType, refractionIndex);

    RenderTargetID refractRTID(RenderTargetUsage::REFRACTION_PLANAR, refractionIndex);

    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(refractRTID);

        DebugView* debugView = s_debugViews[1][refractionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = std::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set(_ID("lodLevel"), GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set(_ID("unpack1Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("unpack2Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("startOnBlue"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("multiplier"), GFX::PushConstantType::FLOAT, 1.0f);

            viewPtr->_name = Util::StringFormat("REFRACTION_%d_%s", refractRTID._index, _parentSGN.name().c_str());
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

    if (_refractionCallback) {
        RenderCbkParams params{ _context, _parentSGN, renderState, refractRTID, refractionIndex, to_U8(_refractorType), camera };
        _refractionCallback(params, bufferInOut);
        return true;
    }

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

namespace Hack {
    Sky* g_skyPtr = nullptr;
};

void RenderingComponent::clearEnvProbeList() {
    updateEnvProbeList({});
}

void RenderingComponent::updateEnvProbeList(const EnvironmentProbeList& probes) {
    STUBBED("ToDo: Fix Env probe rendering! and fix this -Ionut");

    if (probes.empty() || true) {
        // Need a way better way of handling this ...
        if (Hack::g_skyPtr == nullptr) {
            const auto& skies = _context.parent().sceneManager()->getActiveScene().sceneGraph().getNodesByType(SceneNodeType::TYPE_SKY);
            if (!skies.empty()) {
                Hack::g_skyPtr = &skies.front()->getNode<Sky>();
            }
        }
        defaultReflectionTexture(Hack::g_skyPtr->activeSkyBox(), 0);

    } else {
        _envProbes.resize(0);
        _envProbes.reserve(probes.size());
        for (const auto& probe : probes) {
            _envProbes.push_back(probe);
        }

        TransformComponent* const transform = _parentSGN.get<TransformComponent>();
        if (transform) {
            const vec3<F32>& nodePos = transform->getPosition();
            eastl::sort(eastl::begin(_envProbes),
                eastl::end(_envProbes),
                [&nodePos](const auto& a, const auto& b) -> bool {
                return a->distanceSqTo(nodePos) < b->distanceSqTo(nodePos);
            });
        }

        RenderTarget* rt = EnvironmentProbeComponent::reflectionTarget()._rt;
        defaultReflectionTexture(rt->getAttachment(RTAttachmentType::Colour, 0).texture(), _envProbes.front()->rtLayerIndex());
    }

    if (_reflectionIndex == -1) {
        _externalTextures[getUsageIndex(RenderTargetUsage::REFLECTION_CUBE)] = _defaultReflection.first;
    }
}

/// Draw some kind of selection doodad. May differ if editor is running or not
void RenderingComponent::drawSelectionGizmo(GFX::CommandBuffer& bufferInOut) {
    if (!_selectionGizmo) {
        _selectionGizmo = _context.newIMP();
        _selectionGizmo->name("AxisGizmo_" + _parentSGN.name());
        _selectionGizmo->skipPostFX(true);
        _selectionGizmo->pipeline(*_primitivePipeline[2]);
    }

    const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
    if_constexpr(Config::Build::ENABLE_EDITOR) {
        const Editor& editor = _context.parent().platformContext().editor();
        if (editor.inEditMode()) {

            //draw something
            _selectionGizmo->fromBox(bb.getMin(), bb.getMax(), UColour4(0, 0, 255, 255));
            bufferInOut.add(_selectionGizmo->toCommandBuffer());
            return;
        }
    }

    //draw something else (at some point ...)
    _selectionGizmo->fromBox(bb.getMin(), bb.getMax(), UColour4(255, 255, 255, 255));
    bufferInOut.add(_selectionGizmo->toCommandBuffer());
}

/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis(GFX::CommandBuffer& bufferInOut) {
    if (!_axisGizmo) {
        Line temp = {};
        temp.widthStart(10.0f);
        temp.widthEnd(10.0f);
        temp.positionStart(VECTOR3_ZERO);

        Line axisLines[3];
        // Red X-axis
        temp.positionEnd(WORLD_X_AXIS * 4);
        temp.colourStart(UColour4(255, 0, 0, 255));
        temp.colourEnd(UColour4(255, 0, 0, 255));
        axisLines[0] = temp;

        // Green Y-axis
        temp.positionEnd(WORLD_Y_AXIS * 4);
        temp.colourStart(UColour4(0, 255, 0, 255));
        temp.colourEnd(UColour4(0, 255, 0, 255));
        axisLines[1] = temp;

        // Blue Z-axis
        temp.positionEnd(WORLD_Z_AXIS * 4);
        temp.colourStart(UColour4(0, 0, 255, 255));
        temp.colourEnd(UColour4(0, 0, 255, 255));
        axisLines[2] = temp;

        _axisGizmo = _context.newIMP();
        _axisGizmo->name("AxisGizmo_" + _parentSGN.name());
        _axisGizmo->skipPostFX(true);
        _axisGizmo->pipeline(*_primitivePipeline[2]);
        // Create the object containing all of the lines
        _axisGizmo->beginBatch(true, 3 * 2, 1);
        _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(axisLines[0].colourStart()));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : axisLines) {
            _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line.colourStart()));
            _axisGizmo->vertex(line.positionStart());
            _axisGizmo->vertex(line.positionEnd());
        }
        _axisGizmo->end();
        // Finish our object
        _axisGizmo->endBatch();
    }

    TransformComponent* const transform = _parentSGN.get<TransformComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()), false);
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    bufferInOut.add(_axisGizmo->toCommandBuffer());
}

void RenderingComponent::drawSkeleton(GFX::CommandBuffer& bufferInOut) {
    const SceneNode& node = _parentSGN.getNode();
    const bool isMesh = node.type() == SceneNodeType::TYPE_OBJECT3D && static_cast<const Object3D&>(node).getObjectType()._value == ObjectType::MESH;
    if (!isMesh) {
        return;
    }

    // Continue only for skinned 3D objects
    if (static_cast<const Object3D&>(node).getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))
    {
        if (_skeletonPrimitive == nullptr) {
            _skeletonPrimitive = _context.newIMP();
            _skeletonPrimitive->skipPostFX(true);
            _skeletonPrimitive->name("Skeleton_" + _parentSGN.name());
            _skeletonPrimitive->pipeline(*_primitivePipeline[1]);
        }
        // Get the animation component of any submesh. They should be synced anyway.
        for (U32 i = 0; i < _parentSGN.getChildCount(); ++i) {
            const SceneGraphNode& child = _parentSGN.getChild(i);
            AnimationComponent* animComp = child.get<AnimationComponent>();
            if (animComp != nullptr) {
                // Get the skeleton lines from the submesh's animation component
                const vectorEASTL<Line>& skeletonLines = animComp->skeletonLines();
                // Submit the skeleton lines to the GPU for rendering
                _skeletonPrimitive->fromLines(skeletonLines.data(), skeletonLines.size());

                mat4<F32> mat = MAT4_IDENTITY;
                _parentSGN.get<TransformComponent>()->getWorldMatrix(mat);
                _skeletonPrimitive->worldMatrix(mat);
                bufferInOut.add(_skeletonPrimitive->toCommandBuffer());
                return;
            }
        };
    } else if (_skeletonPrimitive) {
        _context.destroyIMP(_skeletonPrimitive);
    }
}

void RenderingComponent::drawBounds(const bool AABB, const bool Sphere, GFX::CommandBuffer& bufferInOut) {
    if (!AABB && !Sphere) {
        return;
    }

    const SceneNode& node = _parentSGN.getNode();
    const bool isSubMesh = node.type() == SceneNodeType::TYPE_OBJECT3D && static_cast<const Object3D&>(node).getObjectType()._value == ObjectType::SUBMESH;

    if (AABB) {
        if (_boundingBoxPrimitive == nullptr) {
            _boundingBoxPrimitive = _context.newIMP();
            _boundingBoxPrimitive->name("BoundingBox_" + _parentSGN.name());
            _boundingBoxPrimitive->pipeline(*_primitivePipeline[0]);
            _boundingBoxPrimitive->skipPostFX(true);
        }


        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive->fromBox(bb.getMin(), bb.getMax(), isSubMesh ? UColour4(0, 0, 255, 255) : UColour4(255, 0, 255, 255));
        bufferInOut.add(_boundingBoxPrimitive->toCommandBuffer());
    } else if (_boundingBoxPrimitive != nullptr) {
        _context.destroyIMP(_boundingBoxPrimitive);
    }

    if (Sphere) {
        if (_boundingSpherePrimitive == nullptr) {
            _boundingSpherePrimitive = _context.newIMP();
            _boundingSpherePrimitive->name("BoundingSphere_" + _parentSGN.name());
            _boundingSpherePrimitive->pipeline(*_primitivePipeline[0]);
            _boundingSpherePrimitive->skipPostFX(true);
        }

        const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        _boundingSpherePrimitive->fromSphere(bs.getCenter(), bs.getRadius(), isSubMesh ? UColour4(0, 255, 0, 255) : UColour4(255, 255, 0, 255));
        bufferInOut.add(_boundingSpherePrimitive->toCommandBuffer());
    } else if (_boundingSpherePrimitive != nullptr) {
        _context.destroyIMP(_boundingSpherePrimitive);
    }
}

};
