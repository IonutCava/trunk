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
																	 _excludeSelfReflection(true),
                                                                     _previewReflection(false)
{
	REGISTER_FRAME_LISTENER(this);
    _renderQuad = New Quad3D();
	_renderQuad->setName("reflectorPreviewQuad");
    ResourceDescriptor reflectionPreviewShader("fboPreview");
	_previewReflectionShader = CreateResource<ShaderProgram>(reflectionPreviewShader);
}

Reflector::~Reflector(){
	SAFE_DELETE(_reflectedTexture);
    RemoveResource(_previewReflectionShader);
	SAFE_DELETE(_renderQuad);
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
	/// generate reflection texture
	updateReflection();
	/// unmark from reflection target
	_updateSelf = true;
	return true;
}

bool Reflector::build(){
	PRINT_FN(Locale::get("REFLECTOR_INIT_FBO"),_resolution.x,_resolution.y );
    TextureDescriptor reflectionDescriptor(TEXTURE_2D, 
								 	       RGBA,
									       RGBA8,
									       UNSIGNED_BYTE); ///Less precision for reflections

	reflectionDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	reflectionDescriptor._generateMipMaps = false; 
	
	_reflectedTexture = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	_reflectedTexture->AddAttachment(reflectionDescriptor,TextureDescriptor::Color0);

	if(!_reflectedTexture->Create(_resolution.x, _resolution.y)){
		return false;
	}
	_createdFBO = true;
	return true;
}

void Reflector::previewReflection(){

    if(!_previewReflection || (_previewReflection && !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))) return;

	_previewReflectionShader->bind();
	_previewReflectionShader->UniformTexture("tex",0);
	_reflectedTexture->Bind(0);
	GFX_DEVICE.toggle2D(true);
		GFX_DEVICE.renderInViewport(vec4<F32>(128,128,512,512),
								    boost::bind(&GFXDevice::renderModel,
									            boost::ref(GFX_DEVICE),
												_renderQuad));
	GFX_DEVICE.toggle2D(false);
	_reflectedTexture->Unbind(0);
	
}