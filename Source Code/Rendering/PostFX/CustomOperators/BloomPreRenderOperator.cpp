#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(RenderTarget* hdrTarget, RenderTarget* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_BLOOM, hdrTarget, ldrTarget)
{
    for (U8 i = 0; i < 2; ++i) {
        _bloomBlurBuffer[i] = GFX_DEVICE.allocateRT(false);
        _bloomBlurBuffer[i]._rt->addAttachment(_hdrTarget->getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);
        _bloomBlurBuffer[i]._rt->useAutoDepthBuffer(false);
        _bloomBlurBuffer[i]._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());
    }

    _bloomOutput = GFX_DEVICE.allocateRT(false);
    _bloomOutput._rt->addAttachment(_hdrTarget->getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);
    _bloomOutput._rt->useAutoDepthBuffer(false);
    _bloomOutput._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());

    ResourceDescriptor bloomCalc("bloom.BloomCalc");
    bloomCalc.setThreadedLoading(false);
    _bloomCalc = CreateResource<ShaderProgram>(bloomCalc);

    ResourceDescriptor bloomApply("bloom.BloomApply");
    bloomApply.setThreadedLoading(false);
    _bloomApply = CreateResource<ShaderProgram>(bloomApply);

    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);
    _blur = CreateResource<ShaderProgram>(blur);

    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    GFX_DEVICE.deallocateRT(_bloomOutput);
    GFX_DEVICE.deallocateRT(_bloomBlurBuffer[0]);
    GFX_DEVICE.deallocateRT(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::idle() {
    _bloomApply->Uniform("bloomFactor",
                         ParamHandler::instance().getParam<F32>(_ID("postProcessing.bloomFactor"), 0.8f));
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_ushort(width / 4.0f);
    U16 h = to_ushort(height / 4.0f);
    _bloomOutput._rt->create(w, h);
    _bloomBlurBuffer[0]._rt->create(width, height);
    _bloomBlurBuffer[1]._rt->create(width, height);

    _blur->Uniform("size", vec2<F32>(width, height));
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute() {
    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));

     // Step 1: generate bloom
    _hdrTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0); //screen
    // render all of the "bright spots"
    _bloomOutput._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_bloomCalc);
        GFX_DEVICE.draw(triangleCmd);
    _bloomOutput._rt->end();

    // Step 2: blur bloom
    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _bloomOutput._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);
    _bloomBlurBuffer[0]._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_blur);
        GFX_DEVICE.draw(triangleCmd);
    _bloomBlurBuffer[0]._rt->end();

    // Blur vertically (recycle the render target. We have a copy)
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _bloomBlurBuffer[0]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);
    _bloomBlurBuffer[1]._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_blur);
        GFX_DEVICE.draw(triangleCmd);
    _bloomBlurBuffer[1]._rt->end();
        
    // Step 3: apply bloom
    _bloomBlurBuffer[0]._rt->blitFrom(_hdrTarget);
    _bloomBlurBuffer[0]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0); //Screen
    _bloomBlurBuffer[1]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0); //Bloom
    _hdrTarget->begin(_screenOnlyDraw);
        triangleCmd.shaderProgram(_bloomApply);
        GFX_DEVICE.draw(triangleCmd);
    _hdrTarget->end();
}

void BloomPreRenderOperator::debugPreview(U8 slot) const {
}

};