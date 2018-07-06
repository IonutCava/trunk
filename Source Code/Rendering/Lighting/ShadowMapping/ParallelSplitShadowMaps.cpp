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

	ResourceDescriptor shadowPreviewShader("fboPreview.Layered");
	_previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);

	std::stringstream ss;
	ss << "Light " << (U32)light->getId() << " viewport ";
	ResourceDescriptor mrt(ss.str());
	mrt.setFlag(true); //No default Material;
	_renderQuad = CreateResource<Quad3D>(mrt);
    _renderQuad->setCustomShader(_previewDepthMapShader);
	_renderQuad->renderInstance()->draw2D(true);
	_jitterTexture = GFX_DEVICE.newPBO(PBO_TEXTURE_3D);

	createJitterTexture(JITTER_SIZE,8,8);

	_depthMap = GFX_DEVICE.newFBO(FBO_2D_ARRAY_DEPTH);

	SamplerDescriptor depthMapSampler;
	depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	depthMapSampler.toggleMipMaps(false);
	depthMapSampler._useRefCompare = true; //< Use compare function
	depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapSampler._depthCompareMode = INTENSITY;

	TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY,
										 DEPTH_COMPONENT,
										 DEPTH_COMPONENT24,
										 UNSIGNED_BYTE); ///Default filters, LINEAR is OK for this
	depthMapDescriptor.setSampler(depthMapSampler);
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false); //<do not draw colors
    _depthMap->toggleDepthBuffer(false); //<do not create a depth buffer
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
	RemoveResource(_renderQuad);
	SAFE_DELETE(_jitterTexture);
}

void PSShadowMaps::resolution(U16 resolution, const SceneRenderState& sceneRenderState){
    vec2<F32> zPlanes = Frustum::getInstance().getZPlanes();
    calculateSplitPoints( _light->getShadowMapInfo()->_numSplits, zPlanes.x,zPlanes.y);
    setOptimalAdjustFactor(0, 5);
	setOptimalAdjustFactor(1, 1);
    setOptimalAdjustFactor(2, 0);

	U8 resolutionFactorTemp = sceneRenderState.shadowMapResolutionFactor();
	CLAMP<U8>(resolutionFactorTemp, 1, 4);
	U16 maxResolutionTemp = resolution;
	if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
		_resolutionFactor = resolutionFactorTemp;
		_maxResolution = maxResolutionTemp;
		//Initialize the FBO's with a variable resolution
		PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());
		U16 shadowMapDimension = _maxResolution/_resolutionFactor;
		_depthMap->Create(shadowMapDimension,shadowMapDimension,_numSplits);
		_renderQuad->setDimensions(vec4<F32>(0,0,shadowMapDimension,shadowMapDimension));
	}
	ShadowMap::resolution(resolution,sceneRenderState);
}

void PSShadowMaps::render(const SceneRenderState& renderState, boost::function0<void> sceneRenderFunction){
	//Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
		return;
	}
    F32 ortho = GET_ACTIVE_SCENEGRAPH()->getRoot()->getBoundingBox().getWidth() * 0.5f;
	CLAMP<F32>(ortho, 1.0f, (F32)Config::DIRECTIONAL_LIGHT_DISTANCE);
    for(U8 i = 0; i < _numSplits; i++){
        _orthoPerPass[_numSplits - i - 1] = ortho / ((1.0f + i) * (1.0f + i) + i);
    }
	_callback = sceneRenderFunction;
	renderInternal(renderState);
}

void PSShadowMaps::setOptimalAdjustFactor(U8 index, F32 value){
    assert(index < _numSplits);
    if(_optAdjustFactor.size() <= index) _optAdjustFactor.push_back(value);
    else _optAdjustFactor[index] = value;
}

void PSShadowMaps::calculateSplitPoints(U8 splitCount, F32 nearDist, F32 farDist, F32 lambda){
    if (splitCount < 2){
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SPLIT_COUNT"), splitCount);
    }
	_numSplits = splitCount;
    _orthoPerPass.resize(_numSplits);
    _splitPadding = 5;
    _splitPoints.resize(splitCount + 1);
 	_splitPoints[0] = nearDist;
	for (U8 i = 1; i < _numSplits; i++){
	    F32 fraction = (F32)i / (F32)_numSplits;
        F32 splitPoint = lambda * nearDist * std::pow(farDist / nearDist, fraction) +
                         (1.0f - lambda) * (nearDist + fraction * (farDist - nearDist));

		_splitPoints[i] = splitPoint;
	}
	_splitPoints[splitCount] = farDist;
}

void PSShadowMaps::renderInternal(const SceneRenderState& renderState) const {
	Scene* activeScene = GET_ACTIVE_SCENE();
	boost::function0<void> renderFunction = _callback.empty() ? SCENE_GRAPH_UPDATE(activeScene->getSceneGraph()) : _callback;
	GFXDevice& gfx = GFX_DEVICE;
	//Get our eye view
	const vec3<F32>& eyePos = renderState.getCameraConst().getEye();
	//For every depth map
	//Lock our projection matrix so no changes will be permanent during the rest of the frame
    //Lock our model view matrix so no camera transforms will be saved beyond this light's scope
    gfx.lockMatrices();
	//Set the camera to the light's view
	_light->setCameraToLightView(eyePos);
	//bind the associated depth map
	_depthMap->Begin();
	//For each depth pass
	for(U8 i = 0; i < _numSplits; i++){
		//Set the appropriate projection
		_light->renderFromLightView(i, _orthoPerPass[i]);
		_depthMap->DrawToLayer(0,i);
			//draw the scene
			GFX_DEVICE.render(renderFunction, renderState);
	}
	_depthMap->End();
	//get all the required information (light MVP matrix for example)
	//and restore to the proper camera view
	_light->setCameraToSceneView();
	//Undo all modifications to the Projection Matrix
    //Undo all modifications to the ModelView Matrix
	gfx.releaseMatrices();
	//Restore our view frustum
	Frustum::getInstance().Extract(eyePos);
}

void PSShadowMaps::previewShadowMaps(){
    _renderQuad->onDraw(DISPLAY_STAGE);

	_previewDepthMapShader->bind();
	_previewDepthMapShader->UniformTexture("tex",0);
	_depthMap->Bind(0,1);
	GFX_DEVICE.toggle2D(true);
		_previewDepthMapShader->Uniform("layer",0);
		GFX_DEVICE.renderInViewport(vec4<U32>(0,0,128,128),
								    DELEGATE_BIND(&GFXDevice::renderInstance,
									            DELEGATE_REF(GFX_DEVICE),
												DELEGATE_REF(_renderQuad->renderInstance())));
		_previewDepthMapShader->Uniform("layer",1);
		GFX_DEVICE.renderInViewport(vec4<U32>(130,0,128,128),
			                        DELEGATE_BIND(&GFXDevice::renderInstance,
									             DELEGATE_REF(GFX_DEVICE),
												 DELEGATE_REF(_renderQuad->renderInstance())));
		_previewDepthMapShader->Uniform("layer",2);
		GFX_DEVICE.renderInViewport(vec4<U32>(260,0,128,128),
			                        DELEGATE_BIND(&GFXDevice::renderInstance,
									            DELEGATE_REF(GFX_DEVICE),
												DELEGATE_REF(_renderQuad->renderInstance())));
	GFX_DEVICE.toggle2D(false);
	_depthMap->Unbind(0);
}