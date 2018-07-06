#include "Headers/ParallelSplitShadowMaps.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Buffers/PixelBufferObject/Headers/PixelBufferObject.h"

#define JITTER_SIZE  16 

PSShadowMaps::PSShadowMaps(Light* light) : ShadowMap(light)
{
	_maxResolution = 0;
	_resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
	CLAMP<F32>(_resolutionFactor,0.001f, 1.0f);
	PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FBO"), light->getId(), "PSSM");
	///Initialize the FBO's with a variable resolution
	PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());
	
	std::stringstream ss;
	ss << "Light " << (U32)light->getId() << " viewport ";
	_renderQuad = New Quad3D();
	_renderQuad->setName(ss.str());
	ResourceDescriptor shadowPreviewShader("fboPreview.Layered");
	_previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
	_jitterTexture = GFX_DEVICE.newPBO(PBO_TEXTURE_3D);
	createJitterTexture(JITTER_SIZE,8,8);

	_depthMap = GFX_DEVICE.newFBO(FBO_2D_ARRAY_DEPTH);

	TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY, 
										 DEPTH_COMPONENT,
										 DEPTH_COMPONENT,
										 UNSIGNED_BYTE); ///Default filters, LINEAR is OK for this
	depthMapDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	depthMapDescriptor._useRefCompare = true; //< Use compare function
	depthMapDescriptor._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapDescriptor._depthCompareMode = LUMINANCE;
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false);
}

void PSShadowMaps::createJitterTexture(I32 size, I32 samples_u, I32 samples_v) {
	_jitterTexture->Create(size, size, samples_u * samples_v / 2, RGBA4, RGBA, UNSIGNED_BYTE);
	U8* data = (U8*)_jitterTexture->Begin();
  
    for (I32 i = 0; i<size; i++) {   
        for (I32 j = 0; j<size; j++) {   
            for (I32 k = 0; k<samples_u*samples_v/2; k++) {   
   
                I32 x, y;   
                vec4<F32> v;   
   
                x = k % (samples_u / 2);   
                y = (samples_v - 1) - k / (samples_u / 2);   
   
                // generate points on a regular samples_u x samples_v rectangular grid   
                v[0] = (F32)(x * 2 + 0.5f) / samples_u;   
                v[1] = (F32)(y + 0.5f) / samples_v;   
                v[2] = (F32)(x * 2 + 1 + 0.5f) / samples_u;   
                v[3] = v[1];   
                   
                // jitter position   
                v[0] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_u);   
                v[1] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_v);   
                v[2] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_u);   
                v[3] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_v);   
   
                // warp to disk   
                vec4<F32> d;   
                d[0] = sqrtf(v[1]) * cosf(2 * M_PI * v[0]);   
                d[1] = sqrtf(v[1]) * sinf(2 * M_PI * v[0]);   
                d[2] = sqrtf(v[3]) * cosf(2 * M_PI * v[2]);   
                d[3] = sqrtf(v[3]) * sinf(2 * M_PI * v[2]);   
   
                data[(k * size * size + j * size + i) * 4 + 0] = (U8)(d[0] * 127);   
                data[(k * size * size + j * size + i) * 4 + 1] = (U8)(d[1] * 127);   
                data[(k * size * size + j * size + i) * 4 + 2] = (U8)(d[2] * 127);   
                data[(k * size * size + j * size + i) * 4 + 3] = (U8)(d[3] * 127);   
            }   
        }   
    }   
	_jitterTexture->End();
}

PSShadowMaps::~PSShadowMaps()
{
	RemoveResource(_previewDepthMapShader);
	SAFE_DELETE(_renderQuad);
	SAFE_DELETE(_jitterTexture);
}

void PSShadowMaps::resolution(U16 resolution,SceneRenderState* sceneRenderState){
	_numSplits = _light->getShadowMapInfo()->_numSplits;
	U8 resolutionFactorTemp = sceneRenderState->shadowMapResolutionFactor();
	CLAMP<U8>(resolutionFactorTemp, 1, 4);
	U16 maxResolutionTemp = resolution;
	if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
		_resolutionFactor = resolutionFactorTemp;
		_maxResolution = maxResolutionTemp;
		///Initialize the FBO's with a variable resolution
		PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());
		U16 shadowMapDimension = _maxResolution/_resolutionFactor;
		_depthMap->Create(shadowMapDimension,shadowMapDimension,_numSplits);
	}
	ShadowMap::resolution(resolution,sceneRenderState);
	_renderQuad->setDimensions(vec4<F32>(0,0,_depthMap->getWidth(), _depthMap->getHeight()));
}

void PSShadowMaps::render(SceneRenderState* sceneRenderState, boost::function0<void> sceneRenderFunction){
	///Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
		return;
	}
	
	_callback = sceneRenderFunction;
	renderInternal(sceneRenderState);
}

void PSShadowMaps::renderInternal(SceneRenderState* renderState) const {
	GFXDevice& gfx = GFX_DEVICE;
	///Get our eye view
	vec3<F32> eyePos = renderState->getCamera()->getEye();
	///For every depth map
	///Lock our projection matrix so no changes will be permanent during the rest of the frame
	gfx.lockProjection();
	///Lock our model view matrix so no camera transforms will be saved beyond this light's scope
	gfx.lockModelView();
	///Set the camera to the light's view
	_light->setCameraToLightView(eyePos);
	
	///bind the associated depth map
	_depthMap->Begin();
	///For each depth pass
	for(U8 i = 0; i < _numSplits; i++){
		///Set the appropriate projection
		_light->renderFromLightView(i);
		_depthMap->DrawToLayer(0,i);
			///draw the scene
			GFX_DEVICE.render(_callback, GET_ACTIVE_SCENE()->renderState());
	}
	_depthMap->End();
	///get all the required information (light MVP matrix for example) 
	///and restore to the proper camera view
	_light->setCameraToSceneView();
	///Undo all modifications to the Projection Matrix
	gfx.releaseProjection();
	///Undo all modifications to the ModelView Matrix
	gfx.releaseModelView();
	///Restore our view frustum
	Frustum::getInstance().Extract(eyePos);
}


void PSShadowMaps::previewShadowMaps(){

	_previewDepthMapShader->bind();
	_previewDepthMapShader->UniformTexture("tex",0);
	_depthMap->Bind(0,1);
	GFX_DEVICE.toggle2D(true);
		_previewDepthMapShader->Uniform("layer",0);
		GFX_DEVICE.renderInViewport(vec4<F32>(0,0,256,256),
								    boost::bind(&GFXDevice::renderModel,
									            boost::ref(GFX_DEVICE),
												_renderQuad));
		_previewDepthMapShader->Uniform("layer",1);
		GFX_DEVICE.renderInViewport(vec4<F32>(260,0,256,256),
			                        boost::bind(&GFXDevice::renderModel,
									             boost::ref(GFX_DEVICE),
												 _renderQuad));
		_previewDepthMapShader->Uniform("layer",2);
		GFX_DEVICE.renderInViewport(vec4<F32>(520,0,256,256),
			                        boost::bind(&GFXDevice::renderModel,
									            boost::ref(GFX_DEVICE),
												_renderQuad));
	GFX_DEVICE.toggle2D(false);
	_depthMap->Unbind(0);
	
}

	
