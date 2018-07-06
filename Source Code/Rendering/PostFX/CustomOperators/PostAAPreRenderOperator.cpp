#include "stdafx.h"

#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/Configuration.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_SS_ANTIALIASING),
      _useSMAA(false),
      _postAASamples(0),
      _idleCount(0)
{
    _samplerCopy = _context.allocateRT("PostAA");
    _samplerCopy._rt->addAttachment(parent.outputRT().getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);

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

void PostAAPreRenderOperator::idle(const Configuration& config) {
    I32 samples = config.rendering.postAASamples;

    if (_postAASamples != samples) {
        _postAASamples = samples;
        _fxaa->Uniform("dvd_qualityMultiplier", _postAASamples);
    }

    if (_idleCount == 0) {
        _useSMAA = _ID_RT(config.rendering.postAAType) == _ID("SMAA");
    }

    _idleCount = (++_idleCount % 60);
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute() {
    STUBBED("ToDo: Move PostAA to compute shaders to avoid a blit and RT swap. -Ionut");
    RenderTarget& ldrTarget = _parent.outputRT();

    _samplerCopy._rt->blitFrom(&ldrTarget);
    _samplerCopy._rt->bind(to_U8(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);

    // Apply FXAA/SMAA to the specified render target
    ldrTarget.begin(RenderTarget::defaultPolicy());
        GenericDrawCommand pointsCmd;
        pointsCmd.primitiveType(PrimitiveType::API_POINTS);
        pointsCmd.drawCount(1);
        pointsCmd.stateHash(_context.getDefaultStateBlock(true));
        pointsCmd.shaderProgram(_useSMAA ? _smaa : _fxaa);

        _context.draw(pointsCmd);
    ldrTarget.end();
}

};