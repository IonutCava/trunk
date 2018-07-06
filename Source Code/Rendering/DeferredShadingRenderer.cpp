#include "stdafx.h"

#include "Headers/DeferredShadingRenderer.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"

namespace Divide {

DeferredShadingRenderer::DeferredShadingRenderer(PlatformContext& context, ResourceCache& cache)
    : Renderer(context, cache, RendererType::RENDERER_DEFERRED_SHADING), _cachedLightCount(0)
{

    WindowManager& winManager = context.app().windowManager();

    _lightTexture = _context.gfx().newPB();
    ResourceDescriptor deferred("DeferredShadingPass2");
    deferred.setThreadedLoading(false);
    _deferredShader = CreateResource<ShaderProgram>(cache, deferred);

    ResourceDescriptor deferredPreview("deferredPreview");
    deferredPreview.setThreadedLoading(false);
    _previewDeferredShader = CreateResource<ShaderProgram>(cache, deferredPreview);
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

    TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D,
                                      GFXImageFormat::DEPTH_COMPONENT,
                                      GFXDataFormat::UNSIGNED_INT);

    SamplerDescriptor depthSampler;
    depthSampler.setFilters(TextureFilter::NEAREST);
    depthSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    depthDescriptor.setSampler(depthSampler);

    vectorImpl<RTAttachmentDescriptor> att = {
        { depthDescriptor, RTAttachmentType::Depth },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "Deferred";
    desc._resolution = winManager.getActiveWindow().getDimensions();
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    for (U8 i = 0; i < 4; ++i) {
        att.push_back(RTAttachmentDescriptor{ gBuffer[i], RTAttachmentType::Colour, i, (i == 0 ? DefaultColours::BLACK : DefaultColours::WHITE) });
    }

    _deferredBuffer = _context.gfx().renderTargetPool().allocateRT(desc);

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
    _renderQuads.push_back(CreateResource<Quad3D>(cache, mrt));
    _renderQuads.push_back(CreateResource<Quad3D>(cache, mrt2));
    _renderQuads.push_back(CreateResource<Quad3D>(cache, mrt3));
    _renderQuads.push_back(CreateResource<Quad3D>(cache, mrt4));
    _renderQuads.push_back(CreateResource<Quad3D>(cache, mrtPreviewSmall));

    STUBBED("Shadow maps are currently disabled for Deferred Rendering! -Ionut")
    _context.gfx().shadowDetailLevel(RenderDetailLevel::OFF);

    RenderTarget& screenRT = _context.gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));

    U16 width = screenRT.getWidth();
    U16 height = screenRT.getHeight();

    updateResolution(width, height);

    RelativePosition2D position(RelativeValue(0.0f, 60.0f), RelativeValue(0.0f, 60.0f));

    GUI& gui = _context.gui();
    gui.addText("PositionData",                // Unique ID
                position,                      // Position
                Font::DIVIDE_DEFAULT,          // Font
                UColour(0, 64, 255, 255),      // Colour
                "POSITION DATA");              // Text

    position.d_x.d_offset = 60.0f + width / 2;
    gui.addText("NormalData",                  // Unique ID
                position,                      // Position
                Font::DIVIDE_DEFAULT,          // Font
                UColour(0, 64, 255, 255),      // Colour
                "NORMAL DATA");                // Text

    position.d_x.d_offset = 60.0f;
    position.d_y.d_offset = 60.0f + height/ 2;
    gui.addText("FinalImage",                   // Unique ID
                position,                       // Position
                Font::DIVIDE_DEFAULT,           // Font
                UColour(0, 64, 255, 255),       // Colour
                "FINAL IMAGE");                 // Text

    position.d_x.d_offset = 60.0f + width / 2;
    position.d_y.d_offset = 60.0f + height / 2;
    gui.addText("LightTexture",                 // Unique ID
                position,                       // Position
                Font::DIVIDE_DEFAULT,           // Font
                UColour(0, 64, 255, 255),       // Colour
                "LIGHT TEXTURE");               // Text
}

DeferredShadingRenderer::~DeferredShadingRenderer()
{
    Console::printfn(Locale::get(_ID("DEFERRED_RT_DELETE")));
    _renderQuads.clear();
    _context.gfx().renderTargetPool().deallocateRT(_deferredBuffer);
}

void DeferredShadingRenderer::preRender(RenderTarget& target,
                                        LightPool& lightPool,
                                        GFX::CommandBuffer& bufferInOut) {
    Renderer::preRender(target, lightPool, bufferInOut);

    Light::LightList& lights = lightPool.getLights(LightType::POINT);

    if (lights.size() != _cachedLightCount) {
        _cachedLightCount = (U16)lights.size();
        _lightTexture->create(2, _cachedLightCount);
    }

    GFX::BeginPixelBufferCommand beginPixelBufferCmd;
    beginPixelBufferCmd._buffer = _lightTexture;
    beginPixelBufferCmd._command = [&lights](bufferPtr data) -> void {
        U8 index = 0;
        F32* pixels = (F32*)data;
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
    };
    GFX::BeginPixelBuffer(bufferInOut, beginPixelBufferCmd);

    GFX::EndPixelBufferCommand endPixelBufferCmd;
    GFX::EndPixelBuffer(bufferInOut, endPixelBufferCmd);
}

void DeferredShadingRenderer::render(const DELEGATE_CBK<void, GFX::CommandBuffer&>& renderCallback,
                                     const SceneRenderState& sceneRenderState,
                                     GFX::CommandBuffer& bufferInOut) {
    firstPass(renderCallback, sceneRenderState, bufferInOut);
    secondPass(sceneRenderState, bufferInOut);
}

void DeferredShadingRenderer::firstPass(const DELEGATE_CBK<void, GFX::CommandBuffer&>& renderCallback,
                                        const SceneRenderState& sceneRenderState,
                                        GFX::CommandBuffer& bufferInOut) {
    // Pass 1
    // Draw the geometry, saving parameters into the buffer
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _deferredBuffer._targetID;
    beginRenderPassCmd._name = "DO_DEFERRED_RENDERING_PASS_1";
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

        renderCallback(bufferInOut);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
}

void DeferredShadingRenderer::secondPass(const SceneRenderState& sceneRenderState,
                                         GFX::CommandBuffer& bufferInOut) {
    // Pass 2
    // Draw a 2D fullscreen quad with lighting shader applied and all generated
    // textures bound to that shader
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._camera = Camera::utilityCamera(Camera::UtilityCamera::_2D);
    GFX::SetCamera(bufferInOut, setCameraCommand);

    TextureData texData0 = _deferredBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    TextureData texData1 = _deferredBuffer._rt->getAttachment(RTAttachmentType::Colour, 1).texture()->getData();
    TextureData texData2 = _deferredBuffer._rt->getAttachment(RTAttachmentType::Colour, 2).texture()->getData();
    TextureData texData3 = _deferredBuffer._rt->getAttachment(RTAttachmentType::Colour, 3).texture()->getData();
    TextureData lightData = _lightTexture->getData();
    
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.addTexture(texData0, 0);
    descriptorSetCmd._set._textureData.addTexture(texData1, 1);
    descriptorSetCmd._set._textureData.addTexture(texData2, 2);
    descriptorSetCmd._set._textureData.addTexture(texData3, 3);
    descriptorSetCmd._set._textureData.addTexture(lightData, 4);
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.gfx().getDefaultStateBlock(true);
    pipelineDescriptor._shaderProgram = _previewDeferredShader;

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = &_context.gfx().newPipeline(pipelineDescriptor);;
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;

    GenericDrawCommand cmd;
    if (_debugView) {
        pushConstantsCommand._constants.set("texDiffuse0", GFX::PushConstantType::UINT, 4);
        GFX::SendPushConstants(bufferInOut, pushConstantsCommand);
        cmd.sourceBuffer(_renderQuads[1]->getGeometryVB());
        {
            GFX::DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(cmd);
            GFX::AddDrawCommands(bufferInOut, drawCmd);
        }
        pushConstantsCommand._constants.set("texDiffuse0", GFX::PushConstantType::UINT, 1);
        GFX::SendPushConstants(bufferInOut, pushConstantsCommand);
        cmd.sourceBuffer(_renderQuads[2]->getGeometryVB());
        {
            GFX::DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(cmd);
            GFX::AddDrawCommands(bufferInOut, drawCmd);
        }
        pushConstantsCommand._constants.set("texDiffuse0", GFX::PushConstantType::UINT, 2);
        GFX::SendPushConstants(bufferInOut, pushConstantsCommand);
        cmd.sourceBuffer(_renderQuads[3]->getGeometryVB());
        {
            GFX::DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(cmd);
            GFX::AddDrawCommands(bufferInOut, drawCmd);
        }
    }

    pipelineDescriptor._shaderProgram = _deferredShader;
    pipelineCmd._pipeline = &_context.gfx().newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    pushConstantsCommand._constants.set("lightCount", GFX::PushConstantType::INT, (I32)_cachedLightCount);
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);
    cmd.sourceBuffer(_renderQuads[_debugView ? 4 : 0]->getGeometryVB());
    {
        GFX::DrawCommand drawCmd;
        drawCmd._drawCommands.push_back(cmd);
        GFX::AddDrawCommands(bufferInOut, drawCmd);
    }
    GUI& gui = _context.gui();
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
    _deferredBuffer._rt->resize(width, height);

    F32 widthF = to_F32(width);
    F32 heightF = to_F32(height);

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