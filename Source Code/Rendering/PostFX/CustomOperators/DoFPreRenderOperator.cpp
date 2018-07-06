#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(Framebuffer* hdrTarget, Framebuffer* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_DEPTH_OF_FIELD, hdrTarget, ldrTarget)
{
    _samplerCopy = GFX_DEVICE.newFB();
    _samplerCopy->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    _samplerCopy->create(_hdrTarget->getWidth(), _hdrTarget->getHeight());

    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(dof);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
    RemoveResource(_dofShader);
    MemoryManager::DELETE(_samplerCopy);
}

void DoFPreRenderOperator::idle() {
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    _samplerCopy->create(width, height);
}

void DoFPreRenderOperator::execute() {
    // Copy current screen
    _samplerCopy->blitFrom(_hdrTarget);
    _samplerCopy->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));  // screenFB
    _inputFB[0]->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1),
                      TextureDescriptor::AttachmentType::Depth);  // depthFB
        
    _hdrTarget->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _dofShader);
    _hdrTarget->end();
}
};