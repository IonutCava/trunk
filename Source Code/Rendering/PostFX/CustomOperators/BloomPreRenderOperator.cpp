#include "stdafx.h"

#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_BLOOM)
{
    vec2<U16> res(parent.inputRT().getWidth(), parent.inputRT().getHeight());

    vectorImpl<RTAttachmentDescriptor> att = {
        { parent.inputRT().getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour, 0, DefaultColours::BLACK() },
    };

    RenderTargetDescriptor desc = {};
    desc._resolution = res;
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    desc._name = "Bloom_Blur_0";
    _bloomBlurBuffer[0] = _context.allocateRT(desc);
    desc._name = "Bloom_Blur_1";
    _bloomBlurBuffer[1] = _context.allocateRT(desc);

    desc._name = "Bloom";
    desc._resolution = vec2<U16>(to_U16(res.w / 4.0f), to_U16(res.h / 4.0f));
    _bloomOutput = _context.allocateRT(desc);

    ResourceDescriptor bloomCalc("bloom.BloomCalc");
    bloomCalc.setThreadedLoading(false);
    _bloomCalc = CreateResource<ShaderProgram>(cache, bloomCalc);

    ResourceDescriptor bloomApply("bloom.BloomApply");
    bloomApply.setThreadedLoading(false);
    _bloomApply = CreateResource<ShaderProgram>(cache, bloomApply);

    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);
    _blur = CreateResource<ShaderProgram>(cache, blur);

    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    _context.deallocateRT(_bloomOutput);
    _context.deallocateRT(_bloomBlurBuffer[0]);
    _context.deallocateRT(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::idle(const Configuration& config) {
    _bloomApply->Uniform("bloomFactor", config.rendering.bloomFactor);
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_U16(width / 4.0f);
    U16 h = to_U16(height / 4.0f);
    _bloomOutput._rt->resize(w, h);
    _bloomBlurBuffer[0]._rt->resize(width, height);
    _bloomBlurBuffer[1]._rt->resize(width, height);

    _blur->Uniform("size", vec2<F32>(width, height));
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute() {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.getDefaultStateBlock(true);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    RenderTarget* screen = &_parent.inputRT();

     // Step 1: generate bloom
    screen->bind(to_U8(ShaderProgram::TextureUsage::UNIT0), RTAttachmentType::Colour, 0); //screen

    // render all of the "bright spots"
    _bloomOutput._rt->begin(RenderTarget::defaultPolicy());
        pipelineDescriptor._shaderProgram = _bloomCalc;
        triangleCmd.pipeline(_context.newPipeline(pipelineDescriptor));
        _context.draw(triangleCmd);
    _bloomOutput._rt->end();

    // Step 2: blur bloom
    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _bloomOutput._rt->bind(to_U8(ShaderProgram::TextureUsage::UNIT0), RTAttachmentType::Colour, 0);
    _bloomBlurBuffer[0]._rt->begin(RenderTarget::defaultPolicy());
        pipelineDescriptor._shaderProgram = _blur;
        triangleCmd.pipeline(_context.newPipeline(pipelineDescriptor));
        _context.draw(triangleCmd);
    _bloomBlurBuffer[0]._rt->end();

    // Blur vertically (recycle the render target. We have a copy)
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _bloomBlurBuffer[0]._rt->bind(to_U8(ShaderProgram::TextureUsage::UNIT0), RTAttachmentType::Colour, 0);
    _bloomBlurBuffer[1]._rt->begin(RenderTarget::defaultPolicy());
        _context.draw(triangleCmd);
    _bloomBlurBuffer[1]._rt->end();
        
    // Step 3: apply bloom
    _bloomBlurBuffer[0]._rt->blitFrom(screen);
    _bloomBlurBuffer[0]._rt->bind(to_U8(ShaderProgram::TextureUsage::UNIT0), RTAttachmentType::Colour, 0); //Screen
    _bloomBlurBuffer[1]._rt->bind(to_U8(ShaderProgram::TextureUsage::UNIT1), RTAttachmentType::Colour, 0); //Bloom
    screen->begin(_screenOnlyDraw);
        pipelineDescriptor._shaderProgram = _bloomApply;
        triangleCmd.pipeline(_context.newPipeline(pipelineDescriptor));
        _context.draw(triangleCmd);
    screen->end();
}

void BloomPreRenderOperator::debugPreview(U8 slot) const {
}

};