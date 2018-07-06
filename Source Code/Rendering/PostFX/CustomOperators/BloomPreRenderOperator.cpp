#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_BLOOM)
{
    for (U8 i = 0; i < 2; ++i) {
        _bloomBlurBuffer[i] = _context.allocateRT(Util::StringFormat("Bloom_Blur_%d", i));
        _bloomBlurBuffer[i]._rt->addAttachment(parent.inputRT().getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);
        _bloomBlurBuffer[i]._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());
    }

    _bloomOutput = _context.allocateRT("Bloom");
    _bloomOutput._rt->addAttachment(parent.inputRT().getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);
    _bloomOutput._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());

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
    triangleCmd.stateHash(_context.getDefaultStateBlock(true));

    RenderTarget* screen = &_parent.inputRT();

     // Step 1: generate bloom
    screen->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0); //screen

    // render all of the "bright spots"
    _bloomOutput._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_bloomCalc);
        _context.draw(triangleCmd);
    _bloomOutput._rt->end();

    // Step 2: blur bloom
    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _bloomOutput._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);
    _bloomBlurBuffer[0]._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_blur);
        _context.draw(triangleCmd);
    _bloomBlurBuffer[0]._rt->end();

    // Blur vertically (recycle the render target. We have a copy)
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _bloomBlurBuffer[0]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);
    _bloomBlurBuffer[1]._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_blur);
        _context.draw(triangleCmd);
    _bloomBlurBuffer[1]._rt->end();
        
    // Step 3: apply bloom
    _bloomBlurBuffer[0]._rt->blitFrom(screen);
    _bloomBlurBuffer[0]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0); //Screen
    _bloomBlurBuffer[1]._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0); //Bloom
    screen->begin(_screenOnlyDraw);
        triangleCmd.shaderProgram(_bloomApply);
        _context.draw(triangleCmd);
    screen->end();
}

void BloomPreRenderOperator::debugPreview(U8 slot) const {
}

};