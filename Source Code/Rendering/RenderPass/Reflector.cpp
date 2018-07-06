#include "Headers/Reflector.h"

#include "Core/Headers/Application.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

Reflector::Reflector(ReflectorType type, const vec2<U16>& resolution) : FrameListener(),
                                                                     _type(type),
                                                                     _resolution(resolution),
                                                                     _updateTimer(0),
                                                                     _updateInterval(45), /// 45 milliseconds?
                                                                     _reflectedTexture(nullptr),
                                                                     _createdFB(false),
                                                                     _updateSelf(false),
                                                                     _planeDirty(true),
                                                                     _excludeSelfReflection(true),
                                                                     _previewReflection(false)
{
    REGISTER_FRAME_LISTENER(this, 3);
    ResourceDescriptor reflectionPreviewShader("fbPreview");
    reflectionPreviewShader.setThreadedLoading(false);
    _previewReflectionShader = CreateResource<ShaderProgram>(reflectionPreviewShader);

    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&Reflector::previewReflection, this), 2);
}

Reflector::~Reflector()
{
    UNREGISTER_FRAME_LISTENER(this);
    SAFE_DELETE(_reflectedTexture);
    RemoveResource(_previewReflectionShader);
}

/// Returning false in any of the FrameListener methods will exit the entire application!
bool Reflector::framePreRenderEnded(const FrameEvent& evt){
    // Do not update the reflection too often so that we can improve speed
    if(evt._currentTime  - _updateTimer < _updateInterval ){
      return true;
    }
    _updateTimer += _updateInterval;
    if(!_createdFB){
        if(!build()){
            // Something wrong. Exit application!
            ERROR_FN(Locale::get("ERROR_REFLECTOR_INIT_FB"));
            return false;
        }
    }
    //We should never have an invalid FB
    assert(_reflectedTexture != nullptr);
    // mark ourselves as reflection target only if we do not wish to reflect ourself back
    _updateSelf = !_excludeSelfReflection;
    // recompute the plane equation
    if(_planeDirty) {
        updatePlaneEquation();
        _planeDirty = false;
    }
    // generate reflection texture
    updateReflection();
    // unmark from reflection target
    _updateSelf = true;
    return true;
}

bool Reflector::build(){
    PRINT_FN(Locale::get("REFLECTOR_INIT_FB"),_resolution.x,_resolution.y );
    SamplerDescriptor reflectionSampler;
    reflectionSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    reflectionSampler.toggleMipMaps(false);

    TextureDescriptor reflectionDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE); //Less precision for reflections

    reflectionDescriptor.setSampler(reflectionSampler);

    _reflectedTexture = GFX_DEVICE.newFB();
    _reflectedTexture->AddAttachment(reflectionDescriptor, TextureDescriptor::Color0);
    _reflectedTexture->toggleDepthBuffer(true);
    _createdFB = _reflectedTexture->Create(_resolution.x, _resolution.y);

    return _createdFB;
}

bool Reflector::previewReflection() {
#   ifdef _DEBUG
        if (_previewReflection) {
            F32 height = _resolution.y * 0.333f;
            _reflectedTexture->Bind(ShaderProgram::TEXTURE_UNIT0);
            vec4<I32> viewport(0, Application::getInstance().getResolution().y - height, _resolution.x  * 0.333f, height);
            GFX_DEVICE.renderInViewport(viewport, DELEGATE_BIND(&GFXDevice::drawPoints, 
                                                                DELEGATE_REF(GFX_DEVICE),
                                                                1,
                                                                GFX_DEVICE.getDefaultStateBlock(true),
                                                                _previewReflectionShader));
        }
#   endif
    return true;
}