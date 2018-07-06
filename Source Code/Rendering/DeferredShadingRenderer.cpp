#include "Headers/DeferredShadingRenderer.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "Scenes/Headers/SceneState.h"
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
    _deferredBuffer = GFX_DEVICE.newFB(FB_2D_DEFERRED);

    ResourceDescriptor deferredPreview("deferredPreview");
    deferredPreview.setThreadedLoading(false);
    _previewDeferredShader = CreateResource<ShaderProgram>(deferredPreview);
    SamplerDescriptor gBufferSampler;
    gBufferSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    gBufferSampler.setFilters(TEXTURE_FILTER_NEAREST);
    gBufferSampler.toggleMipMaps(false);

    TextureDescriptor gBuffer[4]; /// 4 Gbuffer elements (mipmaps are ignored for deferredBuffers)
    //Albedo
    gBuffer[0] = TextureDescriptor(TEXTURE_2D,
                                   RGBA,
                                   RGBA8,  //R8G8B8A8, 32bit format for diffuse
                                   UNSIGNED_BYTE);

    //Position
    gBuffer[1] = TextureDescriptor(TEXTURE_2D,
                                   RGBA,
                                   RGBA32F, //R32G32B32A32, HDR 128bit format for position data
                                   FLOAT_32);
    //Normals
    gBuffer[2] = TextureDescriptor(TEXTURE_2D,
                                   RGBA,
                                   RGBA16F, //R16G16B16A16, 64bit format for normals
                                   FLOAT_32);
    //Blend (for transparent objects - unused for now)
    gBuffer[3] = TextureDescriptor(TEXTURE_2D,
                                   RGBA,
                                   RGBA8, //R8G8B8A8, 32bit format for blend
                                   UNSIGNED_BYTE);

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
    FOR_EACH(Quad3D* renderQuad, _renderQuads){
        assert(renderQuad);
        renderQuad->setCustomShader(_previewDeferredShader);
        renderQuad->renderInstance()->preDraw(true);
    }
    _renderQuads[0]->setCustomShader(_deferredShader);
    _renderQuads[4]->setCustomShader(_deferredShader);

    ParamHandler& par = ParamHandler::getInstance();
    STUBBED("Shadow maps are currently disabled for Deferred Rendering! -Ionut")
    par.setParam("rendering.enableShadows",false);
    F32 width  = (F32)par.getParam<U16>("runtime.resolutionWidth");
    F32 height = (F32)par.getParam<U16>("runtime.resolutionHeight");
    _deferredBuffer->Create(width,height);

    _renderQuads[0]->setDimensions(vec4<F32>(0,0,width,height));
    _renderQuads[1]->setDimensions(vec4<F32>(width/2,0,width,height/2));
    _renderQuads[2]->setCorner(Quad3D::TOP_LEFT, vec3<F32>(0, height, 0));
    _renderQuads[2]->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width/2, height, 0));
    _renderQuads[2]->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(0,height/2,0));
    _renderQuads[2]->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width/2, height/2, 0));
    _renderQuads[3]->setCorner(Quad3D::TOP_LEFT, vec3<F32>(width/2, height, 0));
    _renderQuads[3]->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(width, height, 0));
    _renderQuads[3]->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(width/2,height/2,0));
    _renderQuads[3]->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(width, height/2, 0));
    //Using a separate, smaller render quad for debug view because it's faster than resizing a quad back and forth -Ionut
    _renderQuads[4]->setDimensions(vec4<F32>(0,0,width/2,height/2));
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
    SET_DEFAULT_STATE_BLOCK();
    LightManager::LightMap& lights = LightManager::getInstance().getLights();
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
    _deferredBuffer->Begin(FrameBuffer::defaultPolicy());
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
        if(_debugView){
            _previewDeferredShader->bind();
            _previewDeferredShader->UniformTexture("tex",4);
            GFX_DEVICE.renderInstance(_renderQuads[1]->renderInstance());
            _previewDeferredShader->UniformTexture("tex",1);
            GFX_DEVICE.renderInstance(_renderQuads[2]->renderInstance());
            _previewDeferredShader->UniformTexture("tex",2);
            GFX_DEVICE.renderInstance(_renderQuads[3]->renderInstance());
        }
        _deferredShader->bind();
            _deferredShader->UniformTexture("albedoTexture",0);
            _deferredShader->UniformTexture("positionTexture",1);
            _deferredShader->UniformTexture("normalTexture",2);
            _deferredShader->UniformTexture("blendTexture",3);
            _deferredShader->UniformTexture("lightTexture",4);
            _deferredShader->Uniform("lightCount",(I32)_cachedLightCount);

            GFX_DEVICE.renderInstance(_renderQuads[ _debugView ? 4 : 0]->renderInstance());

    _lightTexture->Unbind(4);
    _deferredBuffer->Unbind(3);
    _deferredBuffer->Unbind(2);
    _deferredBuffer->Unbind(1);
    _deferredBuffer->Unbind(0);

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

void DeferredShadingRenderer::toggleDebugView(){
    _debugView = !_debugView;
}