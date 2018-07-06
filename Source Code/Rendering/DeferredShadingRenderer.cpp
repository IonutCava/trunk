#include "Headers/DeferredShadingRenderer.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Managers/Headers/RenderPassManager.h"

DeferredShadingRenderer::DeferredShadingRenderer() : Renderer(RENDERER_DEFERRED_SHADING),
                                                    _debugView(false),
                                                    _cachedLightCount(0)
{
    _lightTexture = GFX_DEVICE.newPB();
    ResourceDescriptor deferred("DeferredShadingPass2");
    deferred.setThreadedLoading(false);
    _deferredShader = CreateResource<ShaderProgram>(deferred);
    _deferredBuffer = GFX_DEVICE.newFB();

    ResourceDescriptor deferredPreview("deferredPreview");
    deferredPreview.setThreadedLoading(false);
    _previewDeferredShader = CreateResource<ShaderProgram>(deferredPreview);
    SamplerDescriptor gBufferSampler;
    gBufferSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    gBufferSampler.setFilters(TEXTURE_FILTER_NEAREST);
    gBufferSampler.toggleMipMaps(false);

    TextureDescriptor gBuffer[4]; /// 4 Gbuffer elements (mipmaps are ignored for deferredBuffers)
    //Albedo
    gBuffer[0] = TextureDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE);  //R8G8B8A8, 32bit format for diffuse
    //Position
    gBuffer[1] = TextureDescriptor(TEXTURE_2D, RGBA32F, FLOAT_32); //R32G32B32A32, HDR 128bit format for position data
    //Normals
    gBuffer[2] = TextureDescriptor(TEXTURE_2D, RGBA16F, FLOAT_32); //R16G16B16A16, 64bit format for normals
    //Blend (for transparent objects - unused for now)
    gBuffer[3] = TextureDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE); //R8G8B8A8, 32bit format for blend

    for(U8 i = 0; i < 4; i++) gBuffer[i].setSampler(gBufferSampler);

    _deferredBuffer->AddAttachment(gBuffer[0],TextureDescriptor::Color0);
    _deferredBuffer->AddAttachment(gBuffer[1],TextureDescriptor::Color1);
    _deferredBuffer->AddAttachment(gBuffer[2],TextureDescriptor::Color2);
    _deferredBuffer->AddAttachment(gBuffer[3],TextureDescriptor::Color3);
    _deferredBuffer->toggleDepthBuffer(true);
    _deferredBuffer->setClearColor(DefaultColors::BLACK());
    ResourceDescriptor mrtPreviewSmall("MRT RenderQuad SmallPreview");
    mrtPreviewSmall.setFlag(true); //no default material
    ResourceDescriptor mrt("MRT RenderQuad");
    mrt.setFlag(true); //no default material
    ResourceDescriptor mrt2("MRT RenderQuad2");
    mrt2.setFlag(true); //no default material
    ResourceDescriptor mrt3("MRT RenderQuad3");
    mrt3.setFlag(true); //no default material
    ResourceDescriptor mrt4("MRT RenderQuad4");
    mrt4.setFlag(true); //no default material
    _renderQuads.push_back(CreateResource<Quad3D>(mrt));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt2));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt3));
    _renderQuads.push_back(CreateResource<Quad3D>(mrt4));
    _renderQuads.push_back(CreateResource<Quad3D>(mrtPreviewSmall));

    ParamHandler& par = ParamHandler::getInstance();
    STUBBED("Shadow maps are currently disabled for Deferred Rendering! -Ionut")
    par.setParam("rendering.enableShadows",false);
    F32 width  = Application::getInstance().getResolution().width;
    F32 height = Application::getInstance().getResolution().height;
    
    updateResolution(width, height);

    GUI& gui = GUI::getInstance();
    gui.addText("PositionData",           //Unique ID
                vec2<I32>(60,60),          //Position
                Font::DIVIDE_DEFAULT,		    //Font
                vec3<F32>(0.0f,0.2f, 1.0f),  //Color
                "POSITION DATA",0);    //Text and arguments
    gui.addText("NormalData",           //Unique ID
                vec2<I32>(60+width/2,60),          //Position
                Font::DIVIDE_DEFAULT,		    //Font
                vec3<F32>(0.0f,0.2f, 1.0f),  //Color
                "NORMAL DATA",0);    //Text and arguments
    gui.addText("FinalImage",           //Unique ID
                vec2<I32>(60,60+height/2),  //Position
                Font::DIVIDE_DEFAULT,		    //Font
                vec3<F32>(0.0f,0.2f, 1.0f),  //Color
                "FINAL IMAGE",0);    //Text and arguments
    gui.addText("LightTexture",           //Unique ID
            vec2<I32>(60+width/2,60+height/2),          //Position
            Font::DIVIDE_DEFAULT,		    //Font
            vec3<F32>(0.0f,0.2f, 1.0f),  //Color
            "LIGHT TEXTURE",0);    //Text and arguments
}

DeferredShadingRenderer::~DeferredShadingRenderer()
{
    PRINT_FN(Locale::get("DEFERRED_RT_DELETE"));
    RemoveResource(_renderQuads[0]);
    RemoveResource(_renderQuads[1]);
    RemoveResource(_renderQuads[2]);
    RemoveResource(_renderQuads[3]);
    RemoveResource(_renderQuads[4]);
    _renderQuads.clear();
    SAFE_DELETE(_deferredBuffer);
    if(_deferredShader != nullptr){
        RemoveResource(_deferredShader);
    }
    RemoveResource(_previewDeferredShader);
    SAFE_DELETE(_lightTexture);
}

void DeferredShadingRenderer::render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState) {
    GFX_DEVICE.setRenderStage(DEFERRED_STAGE);

    Light::LightMap& lights = LightManager::getInstance().getLights();
    if(lights.size() != _cachedLightCount){
        _cachedLightCount = (U16)lights.size();
        _lightTexture->Create(2,_cachedLightCount);
    }
    U8 index = 0;
    F32* pixels = (F32*)_lightTexture->Begin();
    for(U8 row=0; row<3; row++){
        for(U8 col=0; col < lights.size()/3; col++){
            U8 i = row*10+col;
            //Light Position
            pixels[index + 0]  = lights[i]->getPosition().x;
            pixels[index + 1]  = lights[i]->getPosition().y;
            pixels[index + 2]  = lights[i]->getPosition().z;
            //Light Color
            pixels[index + 3]  = lights[i]->getDiffuseColor().r;
            pixels[index + 4]  = lights[i]->getDiffuseColor().g;
            pixels[index + 5]  = lights[i]->getDiffuseColor().b;
            index += 6;
        }
    }
    _lightTexture->End();
    firstPass(renderCallback, sceneRenderState);
    secondPass(sceneRenderState);
}

void DeferredShadingRenderer::firstPass(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState){
    //Pass 1
    //Draw the geometry, saving parameters into the buffer
    _deferredBuffer->Begin(Framebuffer::defaultPolicy());
        renderCallback();
    _deferredBuffer->End();
}

void DeferredShadingRenderer::secondPass(const SceneRenderState& sceneRenderState){
    //Pass 2
    //Draw a 2D fullscreen quad that has the lighting shader applied to it and all generated textures bound to that shader
    GFX_DEVICE.toggle2D(true);

    _deferredBuffer->Bind(0, TextureDescriptor::Color0);
    _deferredBuffer->Bind(1, TextureDescriptor::Color1);
    _deferredBuffer->Bind(2, TextureDescriptor::Color2);
    _deferredBuffer->Bind(3, TextureDescriptor::Color3);
    _lightTexture->Bind(4);

    GenericDrawCommand cmd;
    cmd.setStateHash(GFX_DEVICE.getDefaultStateBlock(true));
    cmd.setDrawID(GFX_DEVICE.getDrawID(0));
    cmd.setShaderProgram(_previewDeferredShader);
    if(_debugView){
        _previewDeferredShader->bind();
        _previewDeferredShader->UniformTexture("tex",4);
        if(_renderQuads[1]->onDraw(nullptr, GFX_DEVICE.getRenderStage()))
            GFX_DEVICE.submitRenderCommand(_renderQuads[1]->getGeometryVB(), cmd);
        _previewDeferredShader->UniformTexture("tex",1);
        if(_renderQuads[2]->onDraw(nullptr, GFX_DEVICE.getRenderStage()))
            GFX_DEVICE.submitRenderCommand(_renderQuads[2]->getGeometryVB(), cmd);
        _previewDeferredShader->UniformTexture("tex",2);
        if(_renderQuads[3]->onDraw(nullptr, GFX_DEVICE.getRenderStage()))
            GFX_DEVICE.submitRenderCommand(_renderQuads[3]->getGeometryVB(), cmd);
    }
    _deferredShader->bind();
        _deferredShader->Uniform("lightCount",(I32)_cachedLightCount);

    cmd.setShaderProgram(_deferredShader);
    if(_renderQuads[ _debugView ? 4 : 0]->onDraw(nullptr, GFX_DEVICE.getRenderStage()))
        GFX_DEVICE.submitRenderCommand(_renderQuads[ _debugView ? 4 : 0]->getGeometryVB(), cmd);

    GFX_DEVICE.toggle2D(false);
    GUI& gui = GUI::getInstance();
    GUIElement* guiElement = gui.getGuiElement("FinalImage");
    if(guiElement){
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGuiElement("LightTexture");
    if(guiElement){
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGuiElement("PositionData");
    if(guiElement){
        guiElement->setVisible(_debugView);
    }
    guiElement = gui.getGuiElement("NormalData");
    if(guiElement){
        guiElement->setVisible(_debugView);
    }
}

void DeferredShadingRenderer::updateResolution(U16 width, U16 height){
    _deferredBuffer->Create(width, height);

    _renderQuads[0]->setDimensions(vec4<F32>(0, 0, width, height));
    _renderQuads[1]->setDimensions(vec4<F32>(width / 2, 0, width, height / 2));
    _renderQuads[2]->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, height, 0));
    _renderQuads[2]->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width / 2, height, 0));
    _renderQuads[2]->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0, height / 2, 0));
    _renderQuads[2]->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width / 2, height / 2, 0));
    _renderQuads[3]->setCorner(Quad3D::TOP_LEFT, vec3<F32>(width / 2, height, 0));
    _renderQuads[3]->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width, height, 0));
    _renderQuads[3]->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(width / 2, height / 2, 0));
    _renderQuads[3]->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width, height / 2, 0));
    //Using a separate, smaller render quad for debug view because it's faster than resizing a quad back and forth -Ionut
    _renderQuads[4]->setDimensions(vec4<F32>(0, 0, width / 2, height / 2));
}

void DeferredShadingRenderer::toggleDebugView(){
    _debugView = !_debugView;
}