#include "Headers/Reflector.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

Reflector::Reflector(ReflectorType type, const vec2<U16>& resolution) : FrameListener(),
																	 _type(type),
												 					 _resolution(resolution),
																	 _updateTimer(0),
																	 _updateInterval(45), /// 45 milliseconds?
																	 _reflectedTexture(NULL),
																	 _createdFBO(false),
																	 _updateSelf(false),
																	 _planeDirty(true),
																	 _excludeSelfReflection(true),
                                                                     _previewReflection(false)
{
	REGISTER_FRAME_LISTENER(this);
	ResourceDescriptor mrt("reflectorPreviewQuad");
	mrt.setFlag(true); //No default Material;
	_renderQuad = CreateResource<Quad3D>(mrt);
    ResourceDescriptor reflectionPreviewShader("fboPreview");
	_previewReflectionShader = CreateResource<ShaderProgram>(reflectionPreviewShader);
    _renderQuad->setCustomShader(_previewReflectionShader);
	_renderQuad->renderInstance()->preDraw(true);
}

Reflector::~Reflector(){
	SAFE_DELETE(_reflectedTexture);
    RemoveResource(_previewReflectionShader);
	RemoveResource(_renderQuad);
}

/// Returning false in any of the FrameListener methods will exit the entire application!
bool Reflector::framePreRenderEnded(const FrameEvent& evt){
    /// Do not update the reflection too often so that we can improve speed
    if(evt._currentTime  - _updateTimer < _updateInterval ){
      return true;
    }
	_updateTimer += _updateInterval;
	if(!_createdFBO){
		if(!build()){
			/// Something wrong. Exit application!
			ERROR_FN(Locale::get("ERROR_REFLECTOR_INIT_FBO"));
			return false;
		}
	}
	///We should never have an invalid FBO
	assert(_reflectedTexture != NULL);
	/// mark ourselves as reflection target only if we do not wish to reflect ourself back
	_updateSelf = !_excludeSelfReflection;
	/// recompute the plane equation
	if(_planeDirty) {
		updatePlaneEquation();
		_planeDirty = false;
	}
	/// generate reflection texture
	updateReflection();
	/// unmark from reflection target
	_updateSelf = true;
	return true;
}

bool Reflector::build(){
	PRINT_FN(Locale::get("REFLECTOR_INIT_FBO"),_resolution.x,_resolution.y );
	SamplerDescriptor reflectionSampler;
	reflectionSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	reflectionSampler.toggleMipMaps(false);

    TextureDescriptor reflectionDescriptor(TEXTURE_2D,
								 	       RGBA,
									       RGBA8,
									       UNSIGNED_BYTE); ///Less precision for reflections

	reflectionDescriptor.setSampler(reflectionSampler);

	_reflectedTexture = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	_reflectedTexture->AddAttachment(reflectionDescriptor,TextureDescriptor::Color0);
	_reflectedTexture->toggleDepthBuffer(true);
	if(!_reflectedTexture->Create(_resolution.x, _resolution.y)){
		return false;
	}
	_renderQuad->setDimensions(vec4<F32>(0,0,_resolution.x, _resolution.y));
	_createdFBO = true;
	return true;
}

void Reflector::previewReflection(){
    if(!_previewReflection || (_previewReflection && !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))) return;

	_previewReflectionShader->bind();
	_previewReflectionShader->UniformTexture("tex",0);
	_reflectedTexture->Bind(0);
	GFX_DEVICE.toggle2D(true);
		GFX_DEVICE.renderInViewport(vec4<U32>(128,128,256,256),
								    DELEGATE_BIND(&GFXDevice::renderInstance,
									            DELEGATE_REF(GFX_DEVICE),
												DELEGATE_REF(_renderQuad->renderInstance())));
	GFX_DEVICE.toggle2D(false);
	_reflectedTexture->Unbind(0);
}