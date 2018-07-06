#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(RenderTarget* hdrTarget, RenderTarget* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_SS_ANTIALIASING, hdrTarget, ldrTarget),
      _useSMAA(false),
      _postAASamples(0)
{
    _samplerCopy = GFX_DEVICE.newRT();
    _samplerCopy->addAttachment(_ldrTarget->getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);
    _samplerCopy->useAutoDepthBuffer(false);

    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(fxaa);

    ResourceDescriptor smaa("SMAA");
    smaa.setThreadedLoading(false);
    _smaa = CreateResource<ShaderProgram>(smaa);
}

PostAAPreRenderOperator::~PostAAPreRenderOperator()
{
}

void PostAAPreRenderOperator::idle() {
    ParamHandler& par = ParamHandler::instance();
    I32 samples= par.getParam<I32>(_ID("rendering.PostAASamples"), 0);
    if (_postAASamples != samples) {
        _postAASamples = samples;
        _fxaa->Uniform("dvd_qualityMultiplier", _postAASamples);
    }
    _useSMAA = par.getParam<stringImpl>(_ID("rendering.PostAAType"), "FXAA").compare("SMAA") == 0;
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute() {
    _samplerCopy->blitFrom(_ldrTarget);
    _samplerCopy->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);

    // Apply FXAA/SMAA to the specified render target
    _ldrTarget->begin(RenderTarget::defaultPolicy());
        GenericDrawCommand pointsCmd;
        pointsCmd.primitiveType(PrimitiveType::API_POINTS);
        pointsCmd.drawCount(1);
        pointsCmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));
        pointsCmd.shaderProgram(_useSMAA ? _smaa : _fxaa);

        GFX_DEVICE.draw(pointsCmd);
    _ldrTarget->end();
}

};