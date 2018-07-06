#include "Headers/ImageTools.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "nv_dds.h"

#define IL_STATIC_LIB
#undef _UNICODE
#include <IL/ilu.h>

namespace Divide {
namespace ImageTools {

ImageData::ImageData() : _compressed(false),
                         _flip(false),
                         _alpha(false),
                         _bpp(0)

{
    _format = GFXImageFormat::COUNT;
    _compressedTextureType = TextureType::COUNT;
}

ImageData::~ImageData()
{
}

bool ImageData::create(const stringImpl& filename) {
    _name = filename;

    if (Util::CompareIgnoreCase(_name.substr(_name.find_last_of('.') + 1), "DDS")) {
        return loadDDS_IL(filename);
    }

    stbi_set_flip_vertically_on_load(_flip ? TRUE : FALSE);

    I32 width = 0, height = 0, comp = 0;
    U8* data = nullptr;
    F32* dataf = nullptr;
    bool isHDR = stbi_is_hdr(_name.c_str()) == TRUE;
    if (isHDR) {
        dataf = stbi_loadf(_name.c_str(), &width, &height, &comp, 0);
    } else {
        data = stbi_load(_name.c_str(), &width, &height, &comp, 0);
    }

    if ((isHDR && dataf == NULL) || (!isHDR && data == NULL)) {
        Console::errorfn(Locale::get(_ID("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE")), _name.c_str());
        return false;
    }

    _data.resize(1);
    ImageLayer& image = _data.front();

    switch (comp) {
        case 1 : {
            _format = GFXImageFormat::LUMINANCE;
            _bpp = 8;
        } break;
        case 2: {
            _format = GFXImageFormat::LUMINANCE_ALPHA;
            _bpp = 16;
        } break;
        case 3: {
            _format = GFXImageFormat::RGB;
            _bpp = 24;
        } break;
        case 4: {
            _format = GFXImageFormat::RGBA;
            _bpp = 32;
        } break;
        default:
            assert(false && "invalid file format");
            break;
    };

    if (isHDR) {
        _bpp *= 4;
    }

    image._dimensions.set(width, height, 1);
    // most formats do not have an alpha channel
    _alpha = comp % 2 == 0;
    _compressed = false;
    image._size = static_cast<size_t>(std::ceil(width * height * (_bpp / 8.0f)));
    if (isHDR) {
        image.setData(dataf);
        stbi_image_free(dataf);
    } else {
        image.setData(data);
        stbi_image_free(data);
    }

    return true;
}

bool ImageData::loadDDS_IL(const stringImpl& filename) {
    U32 ilTexture = 0;
    ilInit();
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);


    ilEnable(IL_TYPE_SET);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(_flip ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
    ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
    ilTypeFunc(IL_UNSIGNED_BYTE);

    if (ilLoadImage(filename.c_str()) == IL_FALSE) {
        Console::errorfn(Locale::get(_ID("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE")), _name.c_str());
        ilDeleteImage(ilTexture);
        return false;
    }

    I32 numMips = ilGetInteger(IL_NUM_MIPMAPS) + 1;
    assert(ilGetInteger(IL_IMAGE_TYPE) == IL_UNSIGNED_BYTE);

    I32 dxtc = ilGetInteger(IL_DXTC_DATA_FORMAT);
    _bpp = to_const_ubyte(ilGetInteger(IL_IMAGE_BPP));

    if (ilGetInteger(IL_IMAGE_CUBEFLAGS) > 0) {
        _compressedTextureType = TextureType::TEXTURE_CUBE_MAP;
    } else {
        _compressedTextureType = ilGetInteger(IL_IMAGE_DEPTH) > 1
                                     ? TextureType::TEXTURE_3D
                                     : TextureType::TEXTURE_2D;
    }

    bool compressed = ((dxtc == IL_DXT1) || 
                       (dxtc == IL_DXT2) ||
                       (dxtc == IL_DXT3) ||
                       (dxtc == IL_DXT4) ||
                       (dxtc == IL_DXT5));

    if (compressed) {
        switch (dxtc) {
            case IL_DXT1: {
                _format = 
                    ilGetInteger(IL_IMAGE_CHANNELS) == 3
                                                    ? GFXImageFormat::COMPRESSED_RGB_DXT1
                                                    : GFXImageFormat::COMPRESSED_RGBA_DXT1;
                 
            }  break;
            case IL_DXT3: {
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT3;
            } break;
            case IL_DXT5: {
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT5;
            } break;
            default: {
                assert(false && "unsupported compressed format!");
                ilDeleteImage(ilTexture);
                return false;
            }
        };
    } else {
        switch (ilGetInteger(IL_IMAGE_FORMAT)) {
            case IL_BGR: {
                _format = GFXImageFormat::BGR;
            } break;
            case IL_RGB: {
                _format = GFXImageFormat::RGB;
            } break;
            case IL_BGRA: {
                _format = GFXImageFormat::BGRA;
            } break;
            case IL_RGBA: {
                _format = GFXImageFormat::RGBA;
            } break;
            case IL_LUMINANCE: {
                _format = GFXImageFormat::LUMINANCE;
            } break;
            case IL_LUMINANCE_ALPHA: {
                _format = GFXImageFormat::LUMINANCE_ALPHA;
            } break;
            default: {
                assert(false && "unsupported image format");
                ilDeleteImage(ilTexture);
                return false;
            }
        };
    }

    _data.resize(numMips);
    for (U8 i = 0; i < numMips; ++i) {
        ilBindImage(ilTexture);
        ilActiveMipmap(i);

        ILint width = ilGetInteger(IL_IMAGE_WIDTH);
        ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
        ILint depth = ilGetInteger(IL_IMAGE_DEPTH);

        ImageLayer& layer = _data[i];
        layer._dimensions.set(width, height, depth);

        I32 size = compressed ? ilGetDXTCData(NULL, 0, dxtc)
                              : width * height * depth * _bpp;

        I32 numImagePasses = _compressedTextureType == TextureType::TEXTURE_CUBE_MAP ? 6 : 1;
        layer._size = size * numImagePasses;
        layer._data.resize(layer._size);

        for (I32 j = 0, offset = 0; j < numImagePasses; ++j, offset += size) {
            if (_compressedTextureType == TextureType::TEXTURE_CUBE_MAP) {
                ilBindImage(ilTexture);
                ilActiveImage(j);
                ilActiveMipmap(i);
            }
            if (compressed) {
                ilGetDXTCData(&layer._data[0] + offset, size, dxtc);
            } else {
                memcpy(&layer._data[0] + offset, ilGetData(), size);
            }
        }
    }
    
    ilDeleteImage(ilTexture);
    return true;
}

bool ImageData::loadDDS_NV(const stringImpl& filename) {
    nv_dds::CDDSImage image;
    image.load(filename, _flip);
    _alpha = image.get_components() == 4;
    switch(image.get_type()) {
        case nv_dds::TextureFlat:
            _compressedTextureType = image.get_height() == 0 ? TextureType::TEXTURE_1D
                                                             : TextureType::TEXTURE_2D;
            break;
        case nv_dds::Texture3D:
            _compressedTextureType = TextureType::TEXTURE_3D;
            break;
        case nv_dds::TextureCubemap:
            _compressedTextureType = TextureType::TEXTURE_CUBE_MAP;
            break;
        default:
            assert(false && "unsupported texture type");
            break;
    };

    _compressed = image.is_compressed();
    if (_compressed) {
        _bpp = image.get_format() == nv_dds::Format::DXT1 ? 8 : 16;
        switch(image.get_format()) {
            case nv_dds::Format::DXT1:
                _format = GFXImageFormat::COMPRESSED_RGB_DXT1;
                break;
            case nv_dds::Format::DXT3:
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT3;
                break;
            case nv_dds::Format::DXT5:
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT5;
                break;
        };
    } else {
        switch(image.get_format()) {
            case nv_dds::Format::BGR: {
                _bpp = 24;   
                _format = GFXImageFormat::BGR;
            } break;
            case nv_dds::Format::RGB: {
                _bpp = 24;
                _format = GFXImageFormat::RGB;
            } break;
            case nv_dds::Format::BGRA: {
                _bpp = 32;
                _format = GFXImageFormat::BGRA;
            } break;
            case nv_dds::Format::RGBA: {
                _format = GFXImageFormat::RGBA;
                _bpp = 32;
            } break;
            case nv_dds::Format::LUMINANCE: {
                _format = GFXImageFormat::LUMINANCE;
                _bpp = 8;
            } break;
            default:
                assert(false && "unsupported image format");
                break;
        };
    }

    U32 numMips = image.get_num_mipmaps();
    _data.resize(numMips + 1);
    ImageLayer& base = _data[0];
    base._dimensions.set(image.get_width(),
                         image.get_height(),
                         image.get_depth());
    base._size = image.get_size();
    base.setData(image);

    for (U8 i = 0; i < numMips; ++i) {
        ImageLayer& layer = _data[i + 1];
        const nv_dds::CSurface& mipMap = image.get_mipmap(i);

        layer._dimensions.set(mipMap.get_width(),
                              mipMap.get_height(),
                              mipMap.get_depth());
        layer._size = mipMap.get_size();
        layer.setData(mipMap);
    }

    image.clear();
    return true;
}

vec4<U8> ImageData::getColor(I32 x, I32 y, U32 mipLevel) const {
    vec4<U8> returnColor;
    getColor(x, y, returnColor.r, returnColor.g, returnColor.b, returnColor.a, mipLevel);
    return returnColor;
}

void ImageData::getColor(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a, U32 mipLevel) const {
    I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8.0f);
    r = _data[mipLevel]._data[idx + 0];
    g = _data[mipLevel]._data[idx + 1];
    b = _data[mipLevel]._data[idx + 2];
    a = _alpha ? _data[mipLevel]._data[idx + 3] : 255;
}

std::mutex ImageDataInterface::_loadingMutex;
void ImageDataInterface::CreateImageData(const stringImpl& filename, ImageData& imgOut) {
    std::lock_guard<std::mutex> lock(_loadingMutex);
    imgOut.create(filename);
}

I8 SaveToTGA(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth,
             U8* imageData) {
    U8 cGarbage = 0, type, mode, aux;
    I16 iGarbage = 0;
    U16 width = dimensions.width;
    U16 height = dimensions.height;

    // open file and check for errors
    FILE* file = fopen(filename.c_str(), "wb");
    if (file == nullptr) {
        return (-1);
    }

    // compute image type: 2 for RGB(A), 3 for greyscale
    mode = pixelDepth / 8;
    type = ((pixelDepth == 24) || (pixelDepth == 32)) ? 2 : 3;

    // write the header
    fwrite(&cGarbage, sizeof(U8), 1, file);
    fwrite(&cGarbage, sizeof(U8), 1, file);
    fwrite(&type, sizeof(U8), 1, file);

    fwrite(&iGarbage, sizeof(I16), 1, file);
    fwrite(&iGarbage, sizeof(I16), 1, file);
    fwrite(&cGarbage, sizeof(U8), 1, file);
    fwrite(&iGarbage, sizeof(I16), 1, file);
    fwrite(&iGarbage, sizeof(I16), 1, file);

    fwrite(&width, sizeof(U16), 1, file);
    fwrite(&height, sizeof(U16), 1, file);
    fwrite(&pixelDepth, sizeof(U8), 1, file);
    fwrite(&cGarbage, sizeof(U8), 1, file);

    // convert the image data from RGB(a) to BGR(A)
    if (mode >= 3)
        for (I32 i = 0; i < width * height * mode; i += mode) {
            aux = imageData[i];
            imageData[i] = imageData[i + 2];
            imageData[i + 2] = aux;
        }

    // save the image data
    fwrite(imageData, sizeof(U8), width * height * mode, file);
    fclose(file);
    return 0;
}

/// saves a series of files with names "filenameX.tga"
I8 SaveSeries(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth,
              U8* imageData) {
    static I32 savedImages = 0;
    // compute the new filename by adding the
    // series number and the extension
    stringImpl newFilename(Util::StringFormat("%s_%d.tga", filename.c_str(), savedImages));

    // save the image
    I8 status = SaveToTGA(newFilename, dimensions, pixelDepth, imageData);

    // increase the counter
    if (status == 0) {
        savedImages++;
    }

    return status;
}
};  // namespace ImageTools
};  // namespace Divide
