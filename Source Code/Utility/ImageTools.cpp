#include "stdafx.h"

#include "Headers/ImageTools.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "nv_dds.h"

#define IL_STATIC_LIB
#undef _UNICODE
#include <IL/il.h>
#include <IL/ilu.h>

namespace Divide {
namespace ImageTools {

std::mutex ImageDataInterface::_loadingMutex;

ImageData::ImageData() noexcept 
    : _compressed(false),
      _flip(false),
      _16Bit(false),
      _isHDR(false),
      _alpha(false),
      _bpp(0)

{
    _format = GFXImageFormat::COUNT;
    _dataType = GFXDataFormat::COUNT;
    _compressedTextureType = TextureType::COUNT;
}

ImageData::~ImageData()
{
}

bool ImageData::create(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename) {
    _name = filename;

    if (Util::CompareIgnoreCase(_name.substr(_name.find_last_of('.') + 1), "DDS")) {
        std::lock_guard<std::mutex> lock(ImageDataInterface::_loadingMutex);
        return loadDDS_IL(srgb, refWidth, refHeight, filename);
    }

    stbi_set_flip_vertically_on_load(_flip ? TRUE : FALSE);

    I32 width = 0, height = 0, comp = 0;
    U8* data = nullptr;
    U16* data16 = nullptr;
    F32* dataf = nullptr;
    _isHDR = stbi_is_hdr(_name.c_str()) == TRUE;
    if (_isHDR) {
        set16Bit(false);
        dataf = stbi_loadf(_name.c_str(), &width, &height, &comp, 0);
    } else {
        if (is16Bit()) {
            data16 = stbi_load_16(_name.c_str(), &width, &height, &comp, 0);
        } else {
            data = stbi_load(_name.c_str(), &width, &height, &comp, 0);
        }
    }

    if ((_isHDR && dataf == nullptr) || (!_isHDR && data == nullptr && data16 == nullptr)) {
        Console::errorfn(Locale::get(_ID("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE")), _name.c_str());
        return false;
    }

    _data.resize(1);
    ImageLayer& image = _data.front();

    switch (comp) {
        case 1 : {
            _format = GFXImageFormat::RED;
            _bpp = 8;
        } break;
        case 2: {
            _format = GFXImageFormat::RG;
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
            DIVIDE_UNEXPECTED_CALL("Invalid file format!");
            break;
    };

    _dataType = is16Bit() ? GFXDataFormat::UNSIGNED_SHORT : isHDR() ? GFXDataFormat::FLOAT_32 : GFXDataFormat::UNSIGNED_BYTE;

    if (_isHDR) {
        _bpp *= 4;
    } else if (is16Bit()) {
        _bpp *= 2;
    }

    vector<U8> data2 = {};
    vector<U16> data162 = {};
    vector<F32> dataF2 = {};

    if (refWidth != 0 && refHeight != 0 && (refWidth != width || refHeight != height)) {
        I32 ret = 0;
        if (is16Bit()) {
            data162.resize(refWidth * refHeight * comp, 0);
            ret = stbir_resize_uint16_generic(data16, width, height, 0,
                                              data162.data(), refWidth, refHeight, 0,
                                              comp, -1, 0,
                                              STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR,
                                              NULL);
            if (ret == 1) {
                stbi_image_free(data16);
                data16 = nullptr;
            }
        } else if (isHDR()) {
            dataF2.resize(refWidth * refHeight * comp, 0);
            ret = stbir_resize_float(dataf, width, height, 0, dataF2.data(), refWidth, refHeight, 0, comp);
            if (ret == 1) {
                stbi_image_free(dataf);
                dataf = nullptr;
            }
        } else {
            data2.resize(refWidth * refHeight * comp, 0);
            if (srgb) {
                ret = stbir_resize_uint8_srgb(data, width, height, 0, data2.data(), refWidth, refHeight, 0, comp, -1, 0);
            } else {
                ret = stbir_resize_uint8(data, width, height, 0, data2.data(), refWidth, refHeight, 0, comp);
            }
            if (ret == 1) {
                stbi_image_free(data);
                data = nullptr;
            }
        }
        if (ret == 1) {
            width = refWidth;
            height = refHeight;
        }
    }

    image._dimensions.set(width, height, 1);
    // most formats do not have an alpha channel
    _alpha = comp % 2 == 0;
    _compressed = false;
    image._size = width * height * _bpp / 8;

    if (_isHDR) {
        image.writeData(dataf != nullptr ? dataf : dataF2.data(), to_U32(image._size / 4));
        if (dataf != nullptr) {
            stbi_image_free(dataf);
        }
    } else if (is16Bit()) {
        image.writeData(data16 != nullptr ? data16 : data162.data(), to_U32(image._size / 2));
        if (data16 != nullptr) {
            stbi_image_free(data16);
        }
    } else {
        image.writeData(data != nullptr ? data : data2.data(), to_U32(image._size));
        if (data != nullptr) {
            stbi_image_free(data);
        }
    }

    return true;
}

bool ImageData::loadDDS_IL(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename) {
    ACKNOWLEDGE_UNUSED(srgb);

    U32 ilTexture = 0;
    ilInit();
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);
    
    ilEnable(IL_TYPE_SET);

    //ilEnable(IL_ORIGIN_SET);
    //ilOriginFunc(_flip ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
    ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
    ilTypeFunc(IL_UNSIGNED_BYTE);

    if (ilLoadImage(filename.c_str()) == IL_FALSE) {
        Console::errorfn(Locale::get(_ID("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE")), _name.c_str());
        ilDeleteImage(ilTexture);
        return false;
    }

    ILinfo imageInfo;
    iluGetImageInfo(&imageInfo);
    if (imageInfo.Origin == IL_ORIGIN_UPPER_LEFT) {
        iluFlipImage();
    }

    const I32 numMips = ilGetInteger(IL_NUM_MIPMAPS) + 1;
    assert(ilGetInteger(IL_IMAGE_TYPE) == IL_UNSIGNED_BYTE);

    const I32 dxtc = ilGetInteger(IL_DXTC_DATA_FORMAT);
    _bpp = to_U8(ilGetInteger(IL_IMAGE_BPP));

    if (ilGetInteger(IL_IMAGE_CUBEFLAGS) > 0) {
        _compressedTextureType = TextureType::TEXTURE_CUBE_MAP;
    } else {
        _compressedTextureType = ilGetInteger(IL_IMAGE_DEPTH) > 1
                                     ? TextureType::TEXTURE_3D
                                     : TextureType::TEXTURE_2D;
    }

    _compressed = ((dxtc == IL_DXT1) ||
                   (dxtc == IL_DXT2) ||
                   (dxtc == IL_DXT3) ||
                   (dxtc == IL_DXT4) ||
                   (dxtc == IL_DXT5));

    const I32 channelCount = ilGetInteger(IL_IMAGE_CHANNELS);

    _alpha = false;
    if (_compressed) {
        switch (dxtc) {
            case IL_DXT1: {
                _format = channelCount == 3 ? GFXImageFormat::COMPRESSED_RGB_DXT1 : GFXImageFormat::COMPRESSED_RGBA_DXT1;
                _alpha = channelCount == 4;
            }  break;
            case IL_DXT3: {
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT3;
                _alpha = true;
            } break;
            case IL_DXT5: {
                _format = GFXImageFormat::COMPRESSED_RGBA_DXT5;
                _alpha = true;
            } break;
            default: {
                DIVIDE_UNEXPECTED_CALL("Unsupported compressed format!");
                ilDeleteImage(ilTexture);
                return false;
            }
        };
    } else {
        const I32 format = ilGetInteger(IL_IMAGE_FORMAT);
        switch (format) {
            case IL_BGR:
                _format = GFXImageFormat::BGR;
                break;
            case IL_RGB:
                _format = GFXImageFormat::RGB;
                break;
            case IL_BGRA:
                _format = GFXImageFormat::BGRA;
                break;
            case IL_RGBA: {
                _format = GFXImageFormat::RGBA;
            } break;
            case IL_LUMINANCE: {
                assert(false && "LUMINANCE image format is no longer supported!");
            } break;
            default: {
                DIVIDE_UNEXPECTED_CALL("Unsupported image format!");
                ilDeleteImage(ilTexture);
                return false;
            }
        };
    }

    _dataType = GFXDataFormat::UNSIGNED_BYTE;
    _data.resize(numMips);

    if (_compressed && _alpha) {
        const ILint width = ilGetInteger(IL_IMAGE_WIDTH);
        const ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
        const ILint depth = ilGetInteger(IL_IMAGE_DEPTH);
        _decompressedData.resize(width * height * depth * 4);

        ilCopyPixels(0, 0, 0, width, height, depth, IL_RGBA, IL_UNSIGNED_BYTE, _decompressedData.data());
        assert(ilGetError() == IL_NO_ERROR);
    }

    for (U8 i = 0; i < numMips; ++i) {
        ilBindImage(ilTexture);
        ilActiveMipmap(i);

        const ILint width = ilGetInteger(IL_IMAGE_WIDTH);
        const ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
        const ILint depth = ilGetInteger(IL_IMAGE_DEPTH);

        ImageLayer& layer = _data[i];
        layer._dimensions.set(width, height, depth);

        const I32 size = _compressed ? ilGetDXTCData(nullptr, 0, dxtc)
                                     : width * height * depth * _bpp;

        const I32 numImagePasses = _compressedTextureType == TextureType::TEXTURE_CUBE_MAP ? 6 : 1;
        layer._size = to_size(size) * numImagePasses;
        layer._data.resize(layer._size);

        for (I32 j = 0, offset = 0; j < numImagePasses; ++j, offset += size) {
            if (_compressedTextureType == TextureType::TEXTURE_CUBE_MAP) {
                ilBindImage(ilTexture);
                ilActiveImage(j);
                ilActiveMipmap(i);
            }
            if (_compressed) {
                ilGetDXTCData(&layer._data[0] + offset, size, dxtc);
            } else {
                memcpy(&layer._data[0] + offset, ilGetData(), size);
            }
        }
    }
    
    ilDeleteImage(ilTexture);
    return true;
}

bool ImageData::loadDDS_NV(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename) {
    ACKNOWLEDGE_UNUSED(srgb);

    nv_dds::CDDSImage image;
    image.load(filename.c_str(), _flip);
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
            DIVIDE_UNEXPECTED_CALL("Unsupported texture type!");
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
            case nv_dds::Format::BGR:
            case nv_dds::Format::RGB: {
                _bpp = 24;
                _format = GFXImageFormat::RGB;
            } break;
            case nv_dds::Format::BGRA:
            case nv_dds::Format::RGBA: {
                _format = GFXImageFormat::RGBA;
                _bpp = 32;
            } break;
            case nv_dds::Format::LUMINANCE: {
                assert(false && "LUMINANCE image format is no longer supported!");
            } break;
            default:
                DIVIDE_UNEXPECTED_CALL("Unsupported image format!");
                break;
        };
    }
    _dataType = GFXDataFormat::UNSIGNED_BYTE;

    const U32 numMips = image.get_num_mipmaps();
    _data.resize(numMips + 1);
    ImageLayer& base = _data[0];
    base._dimensions.set(image.get_width(),
                         image.get_height(),
                         image.get_depth());
    base._size = image.get_size();
    base.writeData(image, base._dimensions.width * base._dimensions.height);

    for (U8 i = 0; i < numMips; ++i) {
        ImageLayer& layer = _data[i + 1];
        const nv_dds::CSurface& mipMap = image.get_mipmap(i);

        layer._dimensions.set(mipMap.get_width(),
                              mipMap.get_height(),
                              mipMap.get_depth());
        layer._size = mipMap.get_size();
        layer.writeData(mipMap, layer._dimensions.width * layer._dimensions.height);
    }

    image.clear();
    return true;
}

UColour ImageData::getColour(I32 x, I32 y, U32 mipLevel) const {
    UColour returnColour;
    getColour(x, y, returnColour.r, returnColour.g, returnColour.b, returnColour.a, mipLevel);
    return returnColour;
}

void ImageData::getRed(I32 x, I32 y, U8& r, U32 mipLevel) const {
    assert(!_compressed || mipLevel == 0);

    const I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8);
    r = _compressed ? _decompressedData[idx + 0] : _data[mipLevel]._data[idx + 0];
}

void ImageData::getGreen(I32 x, I32 y, U8& g, U32 mipLevel) const {
    assert(!_compressed || mipLevel == 0);

    const I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8);
    g = _compressed ? _decompressedData[idx + 1] : _data[mipLevel]._data[idx + 1];
}

void ImageData::getBlue(I32 x, I32 y, U8& b, U32 mipLevel) const {
    assert(!_compressed || mipLevel == 0);

    const I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8);
    b = _compressed ? _decompressedData[idx + 2] : _data[mipLevel]._data[idx + 2];
}

void ImageData::getAlpha(I32 x, I32 y, U8& a, U32 mipLevel) const {
    assert(!_compressed || mipLevel == 0);

    a = 255;
    if (_alpha) {
        const I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8);
        a = _compressed ? _decompressedData[idx + 3] : _data[mipLevel]._data[idx + 3];
    }
}

void ImageData::getColour(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a, U32 mipLevel) const {
    assert(!_compressed || mipLevel == 0);

    const I32 idx = (y * _data[mipLevel]._dimensions.width + x) * (_bpp / 8);
    const U8* src = _compressed ? _decompressedData.data() : _data[mipLevel]._data.data();

    r = src[idx + 0]; g = src[idx + 1]; b = src[idx + 2]; a = _alpha ? src[idx + 3] : 255;
}


void ImageDataInterface::CreateImageData(const stringImpl& filename, U16 refWidth, U16 refHeight, bool srgb, ImageData& imgOut) {
    if (fileExists(filename.c_str())) {
        //std::lock_guard<std::mutex> lock(_loadingMutex);
        imgOut.create(srgb, refWidth, refHeight, filename);
    }
}

I8 SaveToTGA(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData) noexcept {
    U8 cGarbage = 0;
    I16 iGarbage = 0;
    const U16 width = dimensions.width;
    const U16 height = dimensions.height;

    // open file and check for errors
    FILE* file = fopen(filename.c_str(), "wb");
    if (file == nullptr) {
        return (-1);
    }

    // compute image type: 2 for RGB(A), 3 for greyscale
    U8 mode = pixelDepth / 8;
    U8 type = ((pixelDepth == 24) || (pixelDepth == 32)) ? 2 : 3;

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
            const U8 aux = imageData[i];
            imageData[i] = imageData[i + 2];
            imageData[i + 2] = aux;
        }

    // save the image data
    fwrite(imageData, sizeof(U8), width * height * mode, file);
    fclose(file);
    return 0;
}

/// saves a series of files with names "filenameX.tga"
I8 SaveSeries(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData) {
    static I32 savedImages = 0;
    // compute the new filename by adding the
    // series number and the extension
    stringImpl newFilename(Util::StringFormat("%s_%d.tga", filename.c_str(), savedImages));

    // save the image
    const I8 status = SaveToTGA(newFilename, dimensions, pixelDepth, imageData);

    // increase the counter
    if (status == 0) {
        savedImages++;
    }

    return status;
}
};  // namespace ImageTools
};  // namespace Divide
