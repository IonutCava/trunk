#include "Headers/Texture.h"

#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Texture::Texture(TextureType type, const bool flipped) : HardwareResource("temp_texture"),
                                                        _textureType(type),
                                                        _flipped(flipped),
                                                        _handle(0),
                                                        _bitDepth(0),
                                                        _numLayers(1),
                                                        _samplerDirty(true),
                                                        _mipMapsDirty(true),
                                                        _hasTransparency(false)
{
}

Texture::~Texture()
{
}

/// Load texture data using the specified file name
bool Texture::generateHWResource(const stringImpl& name) {
    // Make sure we have a valid file path
    if (getResourceLocation().empty() || getResourceLocation().compare("default") == 0) {
        return false;
    }
    // Each texture type is loaded differently
    if (_textureType == TEXTURE_2D) {
        // 2D Textures are loaded directly
        if (!LoadFile(_textureType, getResourceLocation())) {
            return false;
        }
    // Cube maps and texture arrays need to load each face/layer separately
    } else if (_textureType == TEXTURE_CUBE_MAP || _textureType == TEXTURE_2D_ARRAY) {
        // Each texture face/layer must be in a comma separated list
        std::stringstream textureLocationList( getResourceLocation().c_str() );
        // We loop over every texture in the above list and store it in this temporary string
        std::string currentTexture;
        U8 idx = 0;
        while (std::getline(textureLocationList, currentTexture, ',')) {
            // Skip invalid entries
            if (!currentTexture.empty()) {
                // Attempt to load the current entry
                if (!LoadFile(idx, stringAlg::toBase(currentTexture))) {
                    // Invalid texture files are not handled yet, so stop loading
                    return false;
                }
                idx++;
            }
        }
        // Cube maps have exactly 6 faces, so make sure all of them are loaded
        if (idx != 6 && _textureType == TEXTURE_CUBE_MAP) {
            ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"), getResourceLocation().c_str());
            return false;
        }
        // Same logic applies to texture arrays
        if (idx != _numLayers && _textureType == TEXTURE_2D_ARRAY) {
            ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT"), getResourceLocation().c_str());
            return false;
        }
    } else {
        // We don't support other texture types
        return false;
    }
    setResourceLocation(name);
    // When we finish loading the texture, we must update the mipmaps
    updateMipMaps();
    
    return HardwareResource::generateHWResource(name);
}

/// Use DevIL to load a file into a Texture Object
bool Texture::LoadFile(U32 target, const stringImpl& name) {
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(_flipped);
    // Save file contents in  the "img" object
    img.create(name);
    // Validate data
    if (!img.data()) {
        ERROR_FN(Locale::get("ERROR_TEXTURE_LOAD"), name.c_str());
        // Missing texture fallback.
        ParamHandler& par = ParamHandler::getInstance();
        img.flip(false);
        // missing_texture.jpg must be something that really stands out
		img.create(par.getParam<stringImpl>("assetsLocation", "assets") + "/" +
			       par.getParam<stringImpl>("defaultTextureLocation", "textures") + "/" +
                   "missing_texture.jpg");
    }

    // Extract width, height and bitdepth
    U16 width = img.dimensions().width;
    U16 height = img.dimensions().height;
    _bitDepth = img.bpp();
    // If we have an alpha channel, we must check for translucency
    if (img.alpha()) {
        // Each pixel is independent so this is a brilliant place to parallelize work
        bool abort = false;
        #pragma omp parallel for
        for (I32 i = 0; i < width; i++) {
            #pragma omp flush (abort)
            if (!abort) {
                // We process one column per thread
                for (I32 j = 0; j < height; j++) {
                    // Check alpha value
                    if (img.getColor(i, j).a < 250) {
                        // If the pixel is transparent, toggle translucency flag
                        #pragma omp critical
                        {
                            // Should be thread-safe
                            _hasTransparency = true;
                            abort = true;
                            #pragma omp flush (abort)
                        }
                    }
                }
            }
        }
    }

    // Create a new Rendering API-dependent texture object    
    GFXImageFormat internalFormat = RGB8;
	// Select the proper color space internal format
bool srgb = _samplerDescriptor.srgb();
	// We only support 8 bit per pixel, 1/2/3/4 channel textures
	switch (img.format()) {
		case RED : internalFormat = RED8;  break;
		case RG  : internalFormat = RG8;   break;
		case RGB : internalFormat = srgb ? SRGB8  : RGB8;  break;
		case RGBA: internalFormat = srgb ? SRGBA8 : RGBA8; break;
	}

    // Uploading to the GPU dependents on the rendering API
    loadData(target, img.data(), img.dimensions(), 
             vec2<U16>(0, (U16)floorf(log2f(fmaxf(width, height)))),
             img.format(), internalFormat, true);

    // We will always return true because we load the "missing_texture.jpg" in case of errors
    return true;
}

};