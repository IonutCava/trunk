#include "Headers/Texture.h"

#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

Texture::Texture(TextureType type, const bool flipped) : HardwareResource("temp_texture"),
                                       _textureType(type),
                                       _flipped(flipped),
                                       _handle(0),
                                       _bitDepth(0),
                                       _mipMapsDirty(true),
                                       _hasTransparency(false),
                                       _numLayers(1)
{
}

bool Texture::generateHWResource(const std::string& name) {
    if(!getResourceLocation().empty() && getResourceLocation().compare("default") != 0){
        if(_textureType == TEXTURE_2D){
            if(!LoadFile(_textureType, getResourceLocation()))	return false;
        }else if (_textureType == TEXTURE_CUBE_MAP || _textureType == TEXTURE_2D_ARRAY){
            U8 i = 0;
            std::stringstream ss( getResourceLocation() );
            std::string it;
            while(std::getline(ss, it, ' ')) {
                if (!it.empty()){
                    if(!LoadFile(i, it)) return false;
                    i++;
                }
            }
            if (i != 6 && _textureType == TEXTURE_CUBE_MAP){
                ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"), getResourceLocation().c_str());
                return false;
            }
        }
    
        updateMipMaps();
    }
    return HardwareResource::generateHWResource(name);
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
    // validate data
    if(!img.data()) {
        ERROR_FN(Locale::get("ERROR_TEXTURE_LOAD"), name.c_str());
        ///Missing texture fallback
        ParamHandler& par = ParamHandler::getInstance();
        img.flip(false);
        img.create(par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+"missing_texture.jpg");
    }

    // Get width
    U16 width = img.dimensions().width;
    // Get height
    U16 height = img.dimensions().height;
    _bitDepth = img.bpp();

    if(img.alpha()){
        #pragma omp parallel for
        for(I32 i = 0; i < width; i++){
            for (I32 j = 0; j < height; j++){
                vec4<U8> color = img.getColor(i, j);
                if (color.a < 250){
                    #pragma omp critical
                    {
                        _hasTransparency = true;
                        goto foundAlpha;
                    }
                }
            }
        }
    }
    // Create a new API-dependent texture object
    foundAlpha:
    
    GFXImageFormat internalFormat = RGB8;
    switch(img.format()){
        case RED : internalFormat = RED8;  break;
        case RG  : internalFormat = RG8;   break;
        case RGB : internalFormat = RGB8;  break;
        case RGBA: internalFormat = RGBA8; break;
    }

    loadData(target, img.data(), img.dimensions(), vec2<U16>(0, (U16)floorf(log2f(fmaxf(width, height)))), img.format(), internalFormat, true);

    return true;
}

void Texture::resize(U16 width, U16 height){
    //_img.resize(width,height);
}

