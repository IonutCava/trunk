#include "stdafx.h"

#include "Headers/DoFPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache)
    : PreRenderOperator(context, parent, FilterType::FILTER_DEPTH_OF_FIELD),
    _focalDepth(0.5f),
    _autoFocus(true)
{
    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "DepthOfField.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor dof("DepthOfField");
    dof.waitForReady(false);
    dof.propertyDescriptor(shaderDescriptor);
    _dofShader = CreateResource<ShaderProgram>(cache, dof);
    _dofShader->addStateCallback(ResourceState::RES_LOADED, [this](CachedResource* res) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _dofShader->getGUID();
        _pipeline = _context.newPipeline(pipelineDescriptor);
    });

    focalDepth(0.5f);
    autoFocus(true);

    _constants.set(_ID("size"), GFX::PushConstantType::VEC2, _parent.screenRT()._rt->getResolution());
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
}

bool DoFPreRenderOperator::ready() const {
    if (_dofShader->getState() == ResourceState::RES_LOADED) {
        return PreRenderOperator::ready();
    }

    return false;
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
    _constants.set(_ID("size"), GFX::PushConstantType::VEC2, vec2<F32>(width, height));
}

void DoFPreRenderOperator::focalDepth(const F32 val) {
    _focalDepth = val;
    _constants.set(_ID("focalDepth"), GFX::PushConstantType::FLOAT, _focalDepth);
}

void DoFPreRenderOperator::autoFocus(const bool state) {
    _autoFocus = state;
    _constants.set(_ID("autoFocus"), GFX::PushConstantType::BOOL, _autoFocus);
}

bool DoFPreRenderOperator::execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut) {
    const auto& screenAtt = input._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO));
    const auto& depthAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0);
    const TextureData screenTex = screenAtt.texture()->data();
    const TextureData depthTex = depthAtt.texture()->data();

    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set._textureData.setTexture(screenTex, screenAtt.samplerHash(),TextureUsage::UNIT0);
    descriptorSetCmd._set._textureData.setTexture(depthTex, depthAtt.samplerHash(),TextureUsage::UNIT1);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = output._targetID;
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    beginRenderPassCmd._name = "DO_DOF_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _pipeline });
    GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _constants });

    GFX::EnqueueCommand(bufferInOut, _triangleDrawCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

    return true;
}

};