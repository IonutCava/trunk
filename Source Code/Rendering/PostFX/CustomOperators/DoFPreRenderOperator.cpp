#include "stdafx.h"

#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_DEPTH_OF_FIELD)
{
    vectorImpl<RTAttachmentDescriptor> att = {
        { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "DoF";
    desc._resolution = vec2<U16>(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    _samplerCopy = _context.renderTargetPool().allocateRT(desc);

    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(cache, dof);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
}

void DoFPreRenderOperator::idle(const Configuration& config) {
    ACKNOWLEDGE_UNUSED(config);
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

void DoFPreRenderOperator::execute() {
    // Copy current screen
    /*
    RenderTarget* screen = &_parent.inputRT();
    _samplerCopy._rt->blitFrom(screen);
    _samplerCopy._rt->bind(to_base(ShaderProgram::TextureUsage::UNIT0), RTAttachmentType::Colour, 0);  // screenFB
    screen->bind(to_base(ShaderProgram::TextureUsage::UNIT1), RTAttachmentType::Depth, 0);  // depthFB
        
    screen->begin(_screenOnlyDraw);
        GenericDrawCommand triangleCmd;
        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);
        triangleCmd.stateHash(_context.getDefaultStateBlock(true));
        triangleCmd.shaderProgram(_dofShader);
        _context.draw(triangleCmd);
    screen->end();
    */
}
};