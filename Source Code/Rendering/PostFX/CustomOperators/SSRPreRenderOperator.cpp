#include "stdafx.h"

#include "Headers/SSRPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

SSRPreRenderOperator::SSRPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache)
    : PreRenderOperator(context, parent, FilterType::FILTER_SS_REFLECTIONS)
{
    SamplerDescriptor screenSampler = {};
    screenSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.minFilter(TextureFilter::LINEAR);
    screenSampler.magFilter(TextureFilter::LINEAR);
    screenSampler.anisotropyLevel(0);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "ScreenSpaceReflections.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor ssr("ScreenSpaceReflections");
    ssr.waitForReady(false);
    ssr.propertyDescriptor(shaderDescriptor);
    _ssrShader = CreateResource<ShaderProgram>(cache, ssr);
    _ssrShader->addStateCallback(ResourceState::RES_LOADED, [this](CachedResource*) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _ssrShader->getGUID();
        _pipeline = _context.newPipeline(pipelineDescriptor);
    });

    _constants.set(_ID("size"), GFX::PushConstantType::VEC2, _parent.screenRT()._rt->getResolution());

    parameters(_parameters);
}

bool SSRPreRenderOperator::ready() const {
    if (_ssrShader->getState() == ResourceState::RES_LOADED) {
        return PreRenderOperator::ready();
    }

    return false;
}

void SSRPreRenderOperator::parameters(const Parameters& params) noexcept {
    _parameters = params;

    _constantsDirty = true;
}

void SSRPreRenderOperator::reshape(const U16 width, const U16 height) {
    PreRenderOperator::reshape(width, height);
    const vec2<F32> s{ width * 0.5f, height * 0.5f };
    _projToPixelBasis = mat4<F32>
    {
       s.x, 0.f, 0.f, 0.f,
       0.f, s.y, 0.f, 0.f,
       0.f, 0.f, 1.f, 0.f,
       s.x, s.y, 0.f, 1.f
    };
}

bool SSRPreRenderOperator::execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut) {

    _constants.set(_ID("projToPixel"), GFX::PushConstantType::MAT4, camera->projectionMatrix() * _projToPixelBasis);
    _constants.set(_ID("projectionMatrix"), GFX::PushConstantType::MAT4, camera->projectionMatrix());
    _constants.set(_ID("invProjectionMatrix"), GFX::PushConstantType::MAT4, GetInverse(camera->projectionMatrix()));
    _constants.set(_ID("invViewMatrix"), GFX::PushConstantType::MAT4, camera->worldMatrix());
    _constants.set(_ID("zPlanes"), GFX::PushConstantType::VEC2, camera->getZPlanes());

    const auto& screenAtt = input._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO));
    const auto& normalsAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_MATERIAL_PROPERTIES));
    const auto& depthAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0);

    const TextureData screenTex = screenAtt.texture()->data();
    const TextureData normalsTex = normalsAtt.texture()->data();
    const TextureData depthTex = depthAtt.texture()->data();

    /// We need mipmaps for roughness based LoD lookup
    GFX::ComputeMipMapsCommand computeMipMapsCommand = {};
    computeMipMapsCommand._texture = screenAtt.texture().get();
    computeMipMapsCommand._defer = false;
    EnqueueCommand(bufferInOut, computeMipMapsCommand);

    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set._textureData.add({ screenTex, screenAtt.samplerHash(),TextureUsage::UNIT0 });
    descriptorSetCmd._set._textureData.add({ depthTex, depthAtt.samplerHash(),TextureUsage::UNIT1 });
    descriptorSetCmd._set._textureData.add({ normalsTex, normalsAtt.samplerHash(), TextureUsage::SCENE_NORMALS });
    EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = { RenderTargetUsage::SSR_RESULT };
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    beginRenderPassCmd._name = "DO_SSR_PASS";
    EnqueueCommand(bufferInOut, beginRenderPassCmd);

    EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _pipeline });

    EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _constants });

    EnqueueCommand(bufferInOut, _triangleDrawCmd);

    EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

    return false;
}

} //namespace Divide
