#include "Headers/GUISplash.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

GUISplash::GUISplash(const std::string& splashImageName,const vec2<U16>& dimensions)
{
	ResourceDescriptor mrt("SplashScreen RenderQuad");
	mrt.setFlag(true); //No default Material;
	_renderQuad = CreateResource<Quad3D>(mrt);
	assert(_renderQuad);
	_renderQuad->setDimensions(vec4<F32>(0,0,dimensions.width,dimensions.height));
    //as the splash screen is loaded before any actual rendering, the draw stage may be undefined. So, a simple VBO should suffice
    //_renderQuad->optimizeForDepth(false);
	ResourceDescriptor splashImage("SplashScreen Texture");
	splashImage.setFlag(true);
	std::string splashImageLocation = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/misc_images/";
	splashImageLocation += splashImageName;
	splashImage.setResourceLocation(splashImageLocation);
	_splashImage = CreateResource<Texture>(splashImage);

}

GUISplash::~GUISplash()
{
	RemoveResource(_renderQuad);
	RemoveResource(_splashImage);
}

void GUISplash::render(){

	GFX_DEVICE.toggle2D(true);
	_splashImage->Bind(0,true);
		GFX_DEVICE.renderModel(_renderQuad);
	_splashImage->Unbind(0);
	GFX_DEVICE.toggle2D(false);
}