#include "Headers/ImageTools.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

#define IL_STATIC_LIB
#undef _UNICODE
#include <IL/ilu.h>

namespace Divide {
namespace ImageTools {

namespace {
    inline GFXImageFormat textureFormatDevIL(ILint format) {
        switch (format) {
        case IL_RGB:
            return GFXImageFormat::RGB;
        case IL_ALPHA:
            return GFXImageFormat::ALPHA;
        case IL_RGBA:
            return GFXImageFormat::RGBA;
        case IL_BGR:
            return GFXImageFormat::BGR;
        case IL_BGRA:
            return GFXImageFormat::BGRA;
        case IL_LUMINANCE:
            return GFXImageFormat::LUMINANCE;
        case IL_LUMINANCE_ALPHA:
            return GFXImageFormat::LUMINANCE_ALPHA;
        };

        return GFXImageFormat::RGB;
    }
};

ImageData::ImageData()
   :_compressed(false),
    _flip(false),
    _alpha(false),
    _bpp(0),
    _imageSize(0)

{
    _format = GFXImageFormat::COUNT;
    _dimensions.set(0, 0);
}

ImageData::~ImageData()
{
}

void ImageData::throwLoadError(const stringImpl& fileName) {
    Console::errorfn(Locale::get("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE"),
                     fileName.c_str());
    ILenum error;
    while ((error = ilGetError()) != IL_NO_ERROR) {
        Console::errorfn(Locale::get("ERROR_IMAGETOOLS_DEVIL"), error);
    }
}

bool ImageData::create(const stringImpl& filename) {
    _name = filename;
    U32 ilTexture = 0;

    ilInit();
    ilEnable(IL_TYPE_SET);
    ilTypeFunc(IL_UNSIGNED_BYTE);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(_flip ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);

    if (ilLoadImage(filename.c_str()) == IL_FALSE) {
        throwLoadError(_name);
        return false;
    }

    _dimensions.set(ilGetInteger(IL_IMAGE_WIDTH),
                    ilGetInteger(IL_IMAGE_HEIGHT));
    _bpp = to_ubyte(ilGetInteger(IL_IMAGE_BPP));
    ILint format = ilGetInteger(IL_IMAGE_FORMAT);

    // we determine the target format and use a flag to determine if we should
    // convert or not
    ILint targetFormat = format;

    switch (format) {
        // palette types
    case IL_COLOR_INDEX: {
        switch (ilGetInteger(IL_PALETTE_TYPE)) {
        default:
        case IL_PAL_NONE: {
            throwLoadError(_name);
            return false;
        }
        case IL_PAL_RGB24:
        case IL_PAL_RGB32:
        case IL_PAL_BGR24:
        case IL_PAL_BGR32:
            targetFormat = IL_RGB;
            break;
        case IL_PAL_BGRA32:
        case IL_PAL_RGBA32:
            targetFormat = IL_RGBA;
            break;
        }
    } break;
    case IL_BGRA:
        targetFormat = IL_RGBA;
        break;
    case IL_BGR:
        targetFormat = IL_RGB;
        break;
    }

    // if the image's format is not desired or the image's data type is not in
    // unsigned byte format, we should convert it
    if (format != targetFormat ||
        (ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)) {
        if (ilConvertImage(targetFormat, IL_UNSIGNED_BYTE) != IL_TRUE) {
            throwLoadError(_name);
            return false;
        }
        format = targetFormat;
    }

    // most formats do not have an alpha channel
    _alpha = (format == IL_RGBA || 
              format == IL_LUMINANCE_ALPHA ||
              format == IL_ALPHA);
    _format = textureFormatDevIL(format);
    _imageSize = static_cast<size_t>(_dimensions.width *
                                     _dimensions.height *
                                     _bpp);
    ILubyte* data = ilGetData();
    _data.assign(data, data + _imageSize);

    ilBindImage(0);
    ilDeleteImage(ilTexture);

    return true;
}

vec4<U8> ImageData::getColor(I32 x, I32 y) const {
    vec4<U8> returnColor;
    getColor(x, y, returnColor.r, returnColor.g, returnColor.b, returnColor.a);
    return returnColor;
}

void ImageData::getColor(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a) const {
    I32 idx = (y * _dimensions.width + x) * _bpp;
    r = _data[idx + 0];
    g = _data[idx + 1];
    b = _data[idx + 2];
    a = _alpha ? _data[idx + 3] : 255;
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
