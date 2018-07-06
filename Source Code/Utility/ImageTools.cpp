#include "Headers/ImageTools.h"

#define IL_STATIC_LIB
#undef _UNICODE
#include <IL/il.h>
#include <IL/ilu.h>

namespace ImageTools {
U8* OpenImagePPM(const std::string& filename, U16& w, U16& h, U8& d, U32& t, U32& ilTexture,const bool flip) {
	char head[70];
	I8 i;
	U8 * img = NULL;
	FILE* f;
	f = fopen(filename.c_str(),"rb");

	if(f==NULL){
		return 0;
	}
	fgets(head,70,f);

	if(!strncmp(head, "P6", 2)){
		i=0;
		while(i<3){
			fgets(head,70,f);

			if(head[0] == '#'){
				continue;
			}

			if(i==0)
				i += sscanf_s(head, "%d %d %d", &w, &h, &d);
			else if(i==1)
				i += sscanf_s(head, "%d %d", &h, &d);
			else if(i==2)
				i += sscanf_s(head, "%d", &d);
		}

		img = New U8[(size_t)(w) * (size_t)(h) * 3];
		if(img==NULL) {
			fclose(f);
			return 0;
		}

		fread(img, sizeof(U8), (size_t)w*(size_t)h*3,f);
		fclose(f);
	}
	else{
		fclose(f);
	}
	t = 0;
	ilTexture = 0;
	return img;
}

U8* OpenImageDevIL(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture, bool& alpha,const bool flip) {
	static bool first = true;
	WriteLock w_lock(_loadingMutex);
	if(first) {
		first = false;
		ilInit();
		ilEnable(IL_TYPE_SET);
		ilTypeFunc(IL_UNSIGNED_BYTE);
	}

	ilOriginFunc(flip ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);

	if (!ilLoadImage(filename.c_str()))	return false;

	w = ilGetInteger(IL_IMAGE_WIDTH);
	h = ilGetInteger(IL_IMAGE_HEIGHT);
	d = ilGetInteger(IL_IMAGE_BPP);
	t = ilGetInteger(IL_IMAGE_TYPE);
	I32 format = ilGetInteger(IL_IMAGE_FORMAT);

	switch(format) {
      case IL_COLOR_INDEX:
         switch(ilGetInteger(IL_PALETTE_TYPE)) {
            case IL_PAL_NONE:
               ilDeleteImages(1, &ilTexture);
               break;

            case IL_PAL_BGRA32:
               ilConvertPal(IL_PAL_RGBA32);

            case IL_PAL_RGBA32:
               alpha = true;
               break;

            case IL_PAL_BGR24:
            case IL_PAL_RGB32:
            case IL_PAL_BGR32:
               ilConvertPal(IL_PAL_RGB24);

            case IL_PAL_RGB24:
               alpha = false;
               break;
         }
         break;

      case IL_LUMINANCE:
      case IL_BGR:
      case IL_RGB:
         if(format != IL_RGB || ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)
            ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
         alpha = false;
         break;

      case IL_LUMINANCE_ALPHA:
      case IL_BGRA:
      case IL_RGBA:
         if(format != IL_RGBA || ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)
            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
         alpha = true;
         break;
   }

    const U8* Pixels = ilGetData();

	U8* img = New U8[(size_t)(w) * (size_t)(h) * (size_t)(d)];
	memcpy(img, Pixels, (size_t)(w) * (size_t)(h) * (size_t)(d));

    ilBindImage(0);
    ilDeleteImages(1, &ilTexture);

	return img;
}

U8* OpenImage(const std::string& filename, U16& w, U16& h, U8& d, U32& t,U32& ilTexture,bool& alpha,const bool flip) {
	if(filename.find(".ppm") != std::string::npos){
		alpha = false;
		return OpenImagePPM(filename, w, h, d,t,ilTexture,flip);
    }

	return OpenImageDevIL(filename,w,h,d,t,ilTexture,alpha,flip);
}

void OpenImage(const std::string& filename, ImageData& img, bool& alpha){
	img.name = filename;
	img.data = OpenImage(filename, img.w, img.h, img.d, img.type,img.ilTexture,alpha,img._flip);
}

void ImageData::Destroy(){
	SAFE_DELETE_ARRAY(data);
}

vec3<I32> ImageData::getColor(U16 x, U16 y) const {
	I32 idx = (y * w + x) * d;
	return vec3<I32>( data[idx+0], data[idx+1], data[idx+2]);
}

void ImageData::resize(U16 width, U16 height){
	ilBindImage(ilTexture);
	iluImageParameter(ILU_FILTER,ILU_SCALE_BELL);
	iluScale(width,height,d);
	ilBindImage(0);
}

I8 saveToTGA(char *filename,
			 U16 width,
			 U16 height,
			 U8	 pixelDepth,
			 U8	*imageData) {
	U8 cGarbage = 0, type,mode,aux;
	I16 iGarbage = 0;
	I32 i;
	FILE *file;

// open file and check for errors
	file = fopen(filename, "wb");
	if (file == NULL) {
		return(-1);
	}

// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

// write the header
	fwrite(&cGarbage, sizeof(U8), 1, file);
	fwrite(&cGarbage, sizeof(U8), 1, file);

	fwrite(&type, sizeof(U8), 1, file);

	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&cGarbage, sizeof(U8), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);

	fwrite(&width, sizeof(I16), 1, file);
	fwrite(&height, sizeof(I16), 1, file);
	fwrite(&pixelDepth, sizeof(U8), 1, file);

	fwrite(&cGarbage, sizeof(U8), 1, file);

// convert the image data from RGB(a) to BGR(A)
	if (mode >= 3)
	for (i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

// save the image data
	fwrite(imageData, sizeof(U8),
			width * height * mode, file);
	fclose(file);
	/// release the memory
	SAFE_DELETE(imageData);
	return(0);
}

/// saves a series of files with names "filenameX.tga"
I8 SaveSeries(char *filename,
			  U16 width,
			  U16 height,
			  U8  pixelDepth,
			  U8 *imageData) {
	static I8 savedImages=0;
	char *newFilename;
	I8 status;

// compute the new filename by adding the
// series number and the extension
	newFilename = new char[strlen(filename)+8];

	sprintf_s(newFilename,strlen(filename)+8,"%s%d.tga",filename,savedImages);

// save the image
	status = saveToTGA(newFilename,width,height,pixelDepth,imageData);

//increase the counter
	if (status == 0)
		savedImages++;
	SAFE_DELETE_ARRAY(newFilename);
	return(status);
}
}