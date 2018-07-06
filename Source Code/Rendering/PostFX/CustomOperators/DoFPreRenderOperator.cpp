#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(GFXDevice& context, RenderTarget* hdrTarget, RenderTarget* ldrTarget)
    : PreRenderOperator(context, FilterType::FILTER_DEPTH_OF_FIELD, hdrTarget, ldrTarget)
{
    _samplerCopy = _context.allocateRT("DoF");
    _samplerCopy._rt->addAttachment(_hdrTarget->getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0, false);

    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(dof);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
}

void DoFPreRenderOperator::idle() {
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

void DoFPreRenderOperator::execute() {
    // Copy current screen
    /*
    _samplerCopy._rt->blitFrom(_hdrTarget);
    _samplerCopy._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);  // screenFB
    _inputFB[0]->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Depth, 0);  // depthFB
        
    _hdrTarget->begin(_screenOnlyDraw);
        GenericDrawCommand triangleCmd;
        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);
        triangleCmd.stateHash(_context.getDefaultStateBlock(true));
        triangleCmd.shaderProgram(_dofShader);
        _context.draw(triangleCmd);
    _hdrTarget->end();
    */
}
};