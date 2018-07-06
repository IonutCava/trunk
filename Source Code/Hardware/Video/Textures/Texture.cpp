#include "Headers/Texture.h"

#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

Texture::Texture(const bool flipped) : HardwareResource(),
                                       _flipped(flipped),
                                       _handle(0),
                                       _hasTransparency(false)
{
}

/// Use DevIL to load a file intro a Texture Object
bool Texture::LoadFile(U32 target, const std::string& name){
    setResourceLocation(name);
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(_flipped);
    // Save file contents in  the "img" object
    img.create(name);
    _hasTransparency = img.alpha();
    // validate data
    if(!img.data()) {
        ERROR_FN(Locale::get("ERROR_TEXTURE_LOAD"), name.c_str());
        ///Missing texture fallback
        ParamHandler& par = ParamHandler::getInstance();
        img.flip(false);
        img.create(par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+"missing_texture.jpg");
    }

    // Get width
    _width = img.dimensions().width;
    // Get height
    _height = img.dimensions().height;
    // Get bitdepth
    _bitDepth = img.bpp();
    GFXImageFormat texture_format = img.format();
    // Create a new API-dependent texture object
    loadData(target, img.data(), img.dimensions(), _bitDepth, texture_format);
    // Unload file data - ImageData destruction handles this
    //img.Destroy();
    return true;
}

void Texture::resize(U16 width, U16 height){
    //_img.resize(width,height);
}

