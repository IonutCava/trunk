#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(GFXDevice& context, ResourceCache& cache, RenderTarget* hdrTarget, RenderTarget* ldrTarget)
    : PreRenderOperator(context, cache, FilterType::FILTER_SS_ANTIALIASING, hdrTarget, ldrTarget),
      _useSMAA(false),
      _postAASamples(0),
      _idleCount(0)
{
    _samplerCopy = _context.allocateRT("PostAA");
    _samplerCopy._rt->addAttachment(_ldrTarget->getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0, false);

    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(cache, fxaa);

    ResourceDescriptor smaa("SMAA");
    smaa.setThreadedLoading(false);
    _smaa = CreateResource<ShaderProgram>(cache, smaa);
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

    if (_idleCount == 0) {
        _useSMAA = par.getParam<U64>(_ID("rendering.PostAAType"), _ID("FXAA")) == _ID("SMAA");
    }

    _idleCount = (++_idleCount % 60);
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute() {
    _samplerCopy._rt->blitFrom(_ldrTarget);
    _samplerCopy._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);

    // Apply FXAA/SMAA to the specified render target
    _ldrTarget->begin(RenderTarget::defaultPolicy());
        GenericDrawCommand pointsCmd;
        pointsCmd.primitiveType(PrimitiveType::API_POINTS);
        pointsCmd.drawCount(1);
        pointsCmd.stateHash(_context.getDefaultStateBlock(true));
        pointsCmd.shaderProgram(_useSMAA ? _smaa : _fxaa);

        _context.draw(pointsCmd);
    _ldrTarget->end();
}

};