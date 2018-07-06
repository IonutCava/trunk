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
    _samplerCopy = _context.allocateRT("DoF");
    _samplerCopy._rt->addAttachment(parent.inputRT().getDescriptor(RTAttachment::Type::Colour, 0), RTAttachment::Type::Colour, 0);

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
    _samplerCopy._rt->bind(to_base(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);  // screenFB
    screen->bind(to_base(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Depth, 0);  // depthFB
        
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