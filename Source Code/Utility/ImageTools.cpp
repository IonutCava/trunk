#include "Headers/ImageTools.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

#define IL_STATIC_LIB
#undef _UNICODE
#include <IL/il.h>
#include <IL/ilu.h>

namespace ImageTools {
	inline GFXImageFormat textureFormatDevIL(ILint format)
	{
		switch(format)	{
			case IL_RGB   : return RGB;
			case IL_ALPHA : return ALPHA;
			case IL_RGBA  : return RGBA;
			case IL_BGR   : return BGR;
			case IL_BGRA  : return BGRA;
			case IL_LUMINANCE : return LUMINANCE;
			case IL_LUMINANCE_ALPHA : return LUMINANCE_ALPHA;
		};

		return RGB;
	}

	void initialize() {
		// used to play nice with DevIL (DevIL acts like OpenGL - a state machine)
		static bool first = true;
		if(!first)
			return;

		first = false;
		ilInit();
		ilEnable(IL_TYPE_SET);
		ilTypeFunc(IL_UNSIGNED_BYTE);
	}

	void ImageData::throwLoadError(const std::string& fileName){
		ERROR_FN(Locale::get("ERROR_IMAGETOOLS_INVALID_IMAGE_FILE"),fileName.c_str());
		ILenum error;
		while((error = ilGetError()) != IL_NO_ERROR) {
			ERROR_FN(Locale::get("ERROR_IMAGETOOLS_DEVIL"), iluErrorString(error));
		}

		ilDeleteImages(1, &_ilTexture);
		_ilTexture = 0;
	}

	bool ImageData::prepareInternalData() {
		initialize();

		ilOriginFunc(_flip ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);
		ilGenImages(1, &_ilTexture);
		ilBindImage(_ilTexture);
		return true;
	}

	bool ImageData::setInternalData() {
		assert(ilGetInteger(IL_CUR_IMAGE) != 0);
		_dimensions.set(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
		_bpp    = ilGetInteger(IL_IMAGE_BPP);
		ILint format = ilGetInteger(IL_IMAGE_FORMAT);

		// we determine the target format and use a flag to determine if we should convert or not
		ILint targetFormat = format;

		switch(format){
			//palette types
			case IL_COLOR_INDEX: {
				switch(ilGetInteger(IL_PALETTE_TYPE)) {
					default:
					case IL_PAL_NONE: {
						throwLoadError(_name);
						return false;
					}
					case IL_PAL_RGB24:
					case IL_PAL_RGB32:
					case IL_PAL_BGR24:
					case IL_PAL_BGR32:  targetFormat = IL_RGB;  break;
					case IL_PAL_BGRA32:
					case IL_PAL_RGBA32:	targetFormat = IL_RGBA; break;
				}
			} break;
			case IL_BGRA: targetFormat = IL_RGBA; break;
			case IL_BGR:  targetFormat = IL_RGB; break;
		}

		// if the image's format is not desired or the image's data type is not in unsigned byte format, we should convert it
		if(format != targetFormat || (ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)){
			if(ilConvertImage(targetFormat, IL_UNSIGNED_BYTE) != IL_TRUE){
				throwLoadError(_name);
				return false;
			}
			format = targetFormat;
		}

		// most formats do not have an alpha channel
		_alpha = (format == IL_RGBA || format == IL_LUMINANCE_ALPHA || format == IL_ALPHA);
		_format = textureFormatDevIL(format);
		_imageSize = (size_t)(_dimensions.width) * (size_t)(_dimensions.height) * (size_t)(_bpp);

        _data = New U8[_imageSize];
        memcpy(_data, ilGetData(), _imageSize);

		ilBindImage(0);
		return true;
	}

	bool ImageData::create(const void* ptr, U32 size) {
		WriteLock w_lock(_loadingMutex);
		prepareInternalData();
		_name = "[buffer offset file]";
		if (ilLoadL(IL_TYPE_UNKNOWN, ptr, size) == IL_FALSE)	{
			throwLoadError(_name);
			return false;
		}

		return setInternalData();
	}

	bool ImageData::create(const std::string& filename) {
		WriteLock w_lock(_loadingMutex);
		prepareInternalData();
		_name = filename;
		if (ilLoadImage(filename.c_str()) == IL_FALSE)	{
			throwLoadError(_name);
			return false;
		}

		return setInternalData();
	}

	void ImageData::destroy(){
		ilDeleteImages(1, &_ilTexture);
		SAFE_DELETE_ARRAY(_data);
	}

	vec4<U8> ImageData::getColor(U16 x, U16 y) const {
		I32 idx = (y * _dimensions.width + x) * _bpp;
        return vec4<U8>(_data[idx + 0], _data[idx + 1], _data[idx + 2], _alpha ? _data[idx + 3] : 255);
	}

	void ImageData::resize(U16 width, U16 height) {
		ilBindImage(_ilTexture);
		iluImageParameter(ILU_FILTER,ILU_SCALE_BELL);
		iluScale(width,height,_bpp);
		_dimensions.set(width,height);
		ilBindImage(0);
	}

	I8 SaveToTGA(const char *filename, const vec2<U16>& dimensions, U8 pixelDepth, U8 *imageData) {
		U8 cGarbage = 0, type,mode,aux;
		I16 iGarbage = 0;
		U16 width = dimensions.width;
		U16 height = dimensions.height;

		// open file and check for errors
		FILE *file = fopen(filename, "wb");
		if (file == nullptr) {
			return(-1);
		}

		// compute image type: 2 for RGB(A), 3 for greyscale
		mode = pixelDepth / 8;
		type = ((pixelDepth == 24) || (pixelDepth == 32)) ?  2 : 3;

		// write the header
		fwrite(&cGarbage, sizeof(U8), 1, file);
		fwrite(&cGarbage, sizeof(U8), 1, file);
		fwrite(&type,     sizeof(U8), 1, file);

		fwrite(&iGarbage, sizeof(I16), 1, file);
		fwrite(&iGarbage, sizeof(I16), 1, file);
		fwrite(&cGarbage, sizeof(U8),  1, file);
		fwrite(&iGarbage, sizeof(I16), 1, file);
		fwrite(&iGarbage, sizeof(I16), 1, file);

		fwrite(&width,      sizeof(U16), 1, file);
		fwrite(&height,     sizeof(U16), 1, file);
		fwrite(&pixelDepth, sizeof(U8),  1, file);
		fwrite(&cGarbage,   sizeof(U8),  1, file);

		// convert the image data from RGB(a) to BGR(A)
		if (mode >= 3)
		for (I32 i=0; i < width * height * mode ; i+= mode) {
			aux = imageData[i];
			imageData[i] = imageData[i+2];
			imageData[i+2] = aux;
		}

		// save the image data
		fwrite(imageData, sizeof(U8), width * height * mode, file);
		fclose(file);
		return 0;
	}

	/// saves a series of files with names "filenameX.tga"
	I8 SaveSeries(char *filename, const vec2<U16>& dimensions, U8 pixelDepth, U8 *imageData) {
		static I32 savedImages = 0;
        std::string newFilename(filename);
		// compute the new filename by adding the
		// series number and the extension
		newFilename += Util::toString(savedImages) + ".tga";

		// save the image
		I8 status = SaveToTGA(newFilename.c_str(),dimensions,pixelDepth,imageData);

		//increase the counter
		if (status == 0) savedImages++;

		return status;
	}
} ///namespace ImageTools