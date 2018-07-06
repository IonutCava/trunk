#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Environment/Water/Headers/Water.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

WaterPlane* ImplResourceLoader<WaterPlane>::operator()(){

	WaterPlane* ptr = New WaterPlane();

	assert(ptr != NULL);
	if(!load(ptr, _descriptor.getName())) return NULL;

	return ptr;
}

template<>
bool ImplResourceLoader<WaterPlane>::load(WaterPlane* const res, const std::string& name) {

	ResourceDescriptor waterMaterial("waterMaterial");
	ResourceDescriptor waterShader("water");
	ResourceDescriptor waterPlane("waterPlane");
	ResourceDescriptor waterTexture("waterTexture");
	waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
	waterPlane.setFlag(true); //No default material

	Texture2D* waterNM = CreateResource<Texture2D>(waterTexture);
	assert(waterNM != NULL);
	ShaderProgram* waterShaderProgram = CreateResource<ShaderProgram>(waterShader);
	assert(waterShaderProgram != NULL);
	res->setMaterial(CreateResource<Material>(waterMaterial));
	//The material is responsible for the destruction of the textures and shaders it receives!!!!
	res->setWaterNormalMap(waterNM);
	res->setShaderProgram(waterShaderProgram);
	res->setGeometry(CreateResource<Quad3D>(waterPlane));

	assert(res->getMaterial() != NULL);

	res->getMaterial()->setTexture(Material::TEXTURE_BASE, waterNM);
	res->getMaterial()->setShaderProgram(waterShaderProgram->getName());
	vec3<F32> waterDiffuse = res->getMaterial()->getMaterialMatrix().getCol(1);
	res->getMaterial()->getMaterialMatrix().setCol(1,vec4<F32>(waterDiffuse, 0.5f)); ///HACK: ToDo: add proper water alpha controls
	RenderStateBlockDescriptor waterMatDesc = res->getMaterial()->getRenderState(FINAL_STAGE)->getDescriptor();
	waterMatDesc.setCullMode(CULL_MODE_NONE);
	waterMatDesc.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
	res->getMaterial()->setRenderStateBlock(waterMatDesc,FINAL_STAGE);

	return res->setInitialData(name);
}