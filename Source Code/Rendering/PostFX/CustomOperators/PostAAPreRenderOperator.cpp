#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(Framebuffer* renderTarget)
    : PreRenderOperator(FilterType::FILTER_SS_ANTIALIASING, renderTarget),
      _useSMAA(false),
      _postAASamples(0)
{
    _samplerCopy = GFX_DEVICE.newFB();
    
    Texture* targetTexture = renderTarget->getAttachment();

    TextureDescriptor postaaDescriptor(targetTexture->getDescriptor());
    _samplerCopy->addAttachment(postaaDescriptor, TextureDescriptor::AttachmentType::Color0);
    _samplerCopy->toggleDepthBuffer(false);

    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(fxaa);

    ResourceDescriptor smaa("SMAA");
    smaa.setThreadedLoading(false);
    _smaa = CreateResource<ShaderProgram>(smaa);
}

PostAAPreRenderOperator::~PostAAPreRenderOperator() {
    RemoveResource(_fxaa);
    RemoveResource(_smaa);
    MemoryManager::DELETE(_samplerCopy);
}

void PostAAPreRenderOperator::idle() {
    ParamHandler& par = ParamHandler::getInstance();
    _postAASamples = par.getParam<I32>("rendering.PostAASamples", 0);
    _useSMAA = par.getParam<stringImpl>("rendering.PostAAType", "FXAA").compare("SMAA") == 0;
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    _samplerCopy->create(width, height);
    _fxaa->Uniform("dvd_fxaaSpanMax", 8.0f);
    _fxaa->Uniform("dvd_fxaaReduceMul", 1.0f / _postAASamples);
    _fxaa->Uniform("dvd_fxaaReduceMin", 1.0f / 128.0f);
    _fxaa->Uniform("dvd_fxaaSubpixShift", 1.0f / (_postAASamples / 2));
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute() {
    // Copy current screen and bind it as an input texture
    _samplerCopy->blitFrom(_renderTarget);
    _samplerCopy->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));

    // Apply FXAA/SMAA to the specified render target
    _renderTarget->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _useSMAA ? _smaa : _fxaa);
    _renderTarget->end();
}

};