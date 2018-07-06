#include "Headers/DeferredShadingRenderer.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"

namespace Divide {

DeferredShadingRenderer::DeferredShadingRenderer()
    : Renderer(RendererType::RENDERER_DEFERRED_SHADING), _cachedLightCount(0)
{
    _lightTexture = GFX_DEVICE.newPB();
    ResourceDescriptor deferred("DeferredShadingPass2");
    deferred.setThreadedLoading(false);
    _deferredShader = CreateResource<ShaderProgram>(deferred);
    _deferredBuffer = GFX_DEVICE.allocateRT(false);

    ResourceDescriptor deferredPreview("deferredPreview");
    deferredPreview.setThreadedLoading(false);
    _previewDeferredShader = CreateResource<ShaderProgram>(deferredPreview);
    SamplerDescriptor gBufferSampler;
    gBufferSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    gBufferSampler.setFilters(TextureFilter::NEAREST);

    TextureDescriptor gBuffer[4];  /// 4 Gbuffer elements (mipmaps are ignored
                                   /// for deferredBuffers)
    // Albedo R8G8B8A8, 32bit format for diffuse
    gBuffer[0] = TextureDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA8, GFXDataFormat::UNSIGNED_BYTE);
    // Position R32G32B32A32, HDR 128bit format for position data
    gBuffer[1] = TextureDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA32F, GFXDataFormat::FLOAT_32);
    // Normals R16G16B16A16, 64bit format for normals
    gBuffer[2] = TextureDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA16F, GFXDataFormat::FLOAT_32);
    // Blend (for transparent objects - unused for now) R8G8B8A8, 32bit format for blend
    gBuffer[3] = TextureDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA8,  GFXDataFormat::UNSIGNED_BYTE);

    for (U8 i = 0; i < 4; i++) {
        gBuffer[i].setSampler(gBufferSampler);
    }

    _deferredBuffer._rt->addAttachment(gBuffer[0], RTAttachment::Type::Colour, 0);
    _deferredBuffer._rt->addAttachment(gBuffer[1], RTAttachment::Type::Colour, 1);
    _deferredBuffer._rt->addAttachment(gBuffer[2], RTAttachment::Type::Colour, 2);
    _deferredBuffer._rt->addAttachment(gBuffer[3], RTAttachment::Type::Colour, 3);
    _deferredBuffer._rt->useAutoDepthBuffer(true);
    _deferredBuffer._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());
    ResourceDescriptor mrtPreviewSmall("MRT RenderQuad SmallPreview");
    mrtPreviewSmall.setFlag(true);  // no default material
    ResourceDescriptor mrt("MRT RenderQuad");
    mrt.setFlag(true);  // no default material
    ResourceDescriptor mrt2("MRT RenderQuad2");
    mrt2.setFlag(true);  // no default material
    ResourceDescriptor mrt3("MRT RenderQuad3");
    mrt3.setFlag(true);  // no default material
    ResourceDescriptor mrt4("MRT RenderQuad4");
    mrt4.setFlag(true);  // no default material
    _renderQuads.push_back(CreateResource<Quad3D>(mrt));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt2));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt3));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt4));
    _renderQuads.push_back(CreateResource<Quad3D>(mrtPreviewSmall));

    ParamHandler& par = ParamHandler::instance();
    STUBBED("Shadow maps are currently disabled for Deferred Rendering! -Ionut")
    par.setParam(_ID("rendering.enableShadows"), false);

    U16 width = GFX_DEVICE.renderTarget(RenderTargetID::SCREEN).getWidth();
    U16 height = GFX_DEVICE.renderTarget(RenderTargetID::SCREEN).getHeight();

    updateResolution(width, height);

    GUI& gui = GUI::instance();
    gui.addText(_ID("PositionData"),                 // Unique ID
                      vec2<I32>(60, 60),             // Position
                      Font::DIVIDE_DEFAULT,          // Font
                      vec4<U8>(0, 64, 255, 255),     // Colour
                      "POSITION DATA");              // Text
    gui.addText(_ID("NormalData"),                   // Unique ID
                      vec2<I32>(60 + width / 2, 60), // Position
                      Font::DIVIDE_DEFAULT,          // Font
                      vec4<U8>(0, 64, 255, 255),     // Colour
                      "NORMAL DATA");                // Text
    gui.addText(_ID("FinalImage"),                    // Unique ID
                      vec2<I32>(60, 60 + height / 2), // Position
                      Font::DIVIDE_DEFAULT,           // Font
                      vec4<U8>(0, 64, 255, 255),      // Colour
                      "FINAL IMAGE");                 // Text
    gui.addText(_ID("LightTexture"),                               // Unique ID
                      vec2<I32>(60 + width / 2, 60 + height / 2),  // Position
                      Font::DIVIDE_DEFAULT,                        // Font
                      vec4<U8>(0, 64, 255, 255),                   // Colour
                      "LIGHT TEXTURE");                            // Text
}

DeferredShadingRenderer::~DeferredShadingRenderer()
{
    Console::printfn(Locale::get(_ID("DEFERRED_RT_DELETE")));
    _renderQuads.clear();
    GFX_DEVICE.deallocateRT(_deferredBuffer);
    MemoryManager::DELETE(_lightTexture);
}

void DeferredShadingRenderer::preRender(LightPool& lightPool) {
    Renderer::preRender(lightPool);

    Light::LightList& lights = lightPool.getLights(LightType::POINT);

    if (lights.size() != _cachedLightCount) {
        _cachedLightCount = (U16)lights.size();
        _lightTexture->create(2, _cachedLightCount);
    }

    U8 index = 0;
    F32* pixels = (F32*)_lightTexture->begin();
    for (U8 row = 0; row < 3; row++) {
        for (U8 col = 0; col < lights.size() / 3; col++) {
            U8 i = row * 10 + col;
            // Light Position
            pixels[index + 0] = lights[i]->getPosition().x;
            pixels[index + 1] = lights[i]->getPosition().y;
            pixels[index + 2] = lights[i]->getPosition().z;
            // Light Colour
            pixels[index + 3] = lights[i]->getDiffuseColour().r;
            pixels[index + 4] = lights[i]->getDiffuseColour().g;
            pixels[index + 5] = lights[i]->getDiffuseColour().b;
            index += 6;
        }
    }
    _lightTexture->end();
}

void DeferredShadingRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                     const SceneRenderState& sceneRenderState) {
    firstPass(renderCallback, sceneRenderState);
    secondPass(sceneRenderState);
}

void DeferredShadingRenderer::firstPass(const DELEGATE_CBK<>& renderCallback,
                                        const SceneRenderState& sceneRenderState) {
    // Pass 1
    // Draw the geometry, saving parameters into the buffer
    _deferredBuffer._rt->begin(RenderTarget::defaultPolicy());
    renderCallback();
    _deferredBuffer._rt->end();
}

void DeferredShadingRenderer::secondPass(
    const SceneRenderState& sceneRenderState) {
    // Pass 2
    // Draw a 2D fullscreen quad with lighting shader applied and all generated
    // textures bound to that shader
    GFX::Scoped2DRendering scoped2D(true);

    _deferredBuffer._rt->bind(0, RTAttachment::Type::Colour, 0);
    _deferredBuffer._rt->bind(1, RTAttachment::Type::Colour, 1);
    _deferredBuffer._rt->bind(2, RTAttachment::Type::Colour, 2);
    _deferredBuffer._rt->bind(3, RTAttachment::Type::Colour, 3);
    _lightTexture->bind(4);

    GenericDrawCommand cmd;
    cmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));
    cmd.shaderProgram(_previewDeferredShader);
    if (_debugView) {
        _previewDeferredShader->Uniform("texDiffuse0", 4);
        if (_renderQuads[1]->onRender(GFX_DEVICE.getRenderStage())) {
            cmd.sourceBuffer(_renderQuads[1]->getGeometryVB());
            GFX_DEVICE.submitCommand(cmd);
        }
        _previewDeferredShader->Uniform("texDiffuse0", 1);
        if (_renderQuads[2]->onRender(GFX_DEVICE.getRenderStage())) {
            cmd.sourceBuffer(_renderQuads[2]->getGeometryVB());
            GFX_DEVICE.submitCommand(cmd);
        }
        _previewDeferredShader->Uniform("texDiffuse0", 2);
        if (_renderQuads[3]->onRender(GFX_DEVICE.getRenderStage())) {
            cmd.sourceBuffer(_renderQuads[3]->getGeometryVB());
            GFX_DEVICE.submitCommand(cmd);
        }
    }

    _deferredShader->Uniform("lightCount", (I32)_cachedLightCount);

    cmd.shaderProgram(_deferredShader);
    if (_renderQuads[_debugView ? 4 : 0]->onRender(GFX_DEVICE.getRenderStage())) {
        cmd.sourceBuffer(_renderQuads[_debugView ? 4 : 0]->getGeometryVB());
        GFX_DEVICE.submitCommand(cmd);
    }

    GUI& gui = GUI::instance();
    GUIElement* guiElement = gui.getGUIElement(0, _ID("FinalImage"));
    if (guiElement) {
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGUIElement(0, _ID("LightTexture"));
    if (guiElement) {
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGUIElement(0, _ID("PositionData"));
    if (guiElement) {
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGUIElement(0, _ID("NormalData"));
    if (guiElement) {
        guiElement->setVisible(_debugView);
    }
}

void DeferredShadingRenderer::updateResolution(U16 width, U16 height) {
    _deferredBuffer._rt->create(width, height);

    F32 widthF = to_float(width);
    F32 heightF = to_float(height);

    _renderQuads[0]->setDimensions(vec4<F32>(0.0f, 0.0f, widthF, heightF));

    _renderQuads[1]->setDimensions(vec4<F32>(widthF / 2, 0.0f, widthF, heightF / 2));

    _renderQuads[2]->setCorner(Quad3D::CornerLocation::TOP_LEFT,
                               vec3<F32>(0.0f, heightF, 0.0f));
    _renderQuads[2]->setCorner(Quad3D::CornerLocation::TOP_RIGHT,
                               vec3<F32>(widthF / 2, heightF, 0U));
    _renderQuads[2]->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT,
                               vec3<F32>(0.0f, heightF / 2, 0.0f));
    _renderQuads[2]->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT,
                               vec3<F32>(widthF / 2, heightF / 2, 0U));

    _renderQuads[3]->setCorner(Quad3D::CornerLocation::TOP_LEFT,
                               vec3<F32>(widthF / 2, heightF, 0.0f));
    _renderQuads[3]->setCorner(Quad3D::CornerLocation::TOP_RIGHT,
                               vec3<F32>(widthF, heightF, 0.0f));
    _renderQuads[3]->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT,
                               vec3<F32>(widthF / 2, heightF / 2, 0.0f));
    _renderQuads[3]->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT,
                               vec3<F32>(widthF, heightF / 2, 0.0f));
    // Using a separate, smaller render quad for debug view because it's faster
    // than resizing a quad back and forth -Ionut
    _renderQuads[4]->setDimensions(vec4<F32>(0.0f, 0.0f, widthF / 2, heightF / 2));
}
};