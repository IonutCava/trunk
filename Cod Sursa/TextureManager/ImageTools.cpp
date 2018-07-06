#include "resource.h"
#include "ImageTools.h"

#undef _UNICODE
#include "IL/il.h"
#include "IL/ilu.h"
#include "IL/ilut.h"
#include "Hardware/Video/GFXDevice.h"

namespace ImageTools {

unsigned char* OpenImagePPM(const string& filename, U32& w, U32& h, U32& d, U32& t,bool flip)
{

	char head[70];
	int i,j;
	U8 * img = NULL;
	FILE* f;
	f = fopen(filename.c_str(),"rb");

	if(f==NULL){
		return 0;
	}
	fgets(head,70,f);

	if(!strncmp(head, "P6", 2)){
		i=0;
		j=0;
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
				i += sscanf(head, "%d", &d);
		}

		img = new U8[(size_t)(w) * (size_t)(h) * 3];
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
	return img;
}

unsigned char* OpenImageDevIL(const string& filename, U32& w, U32& h, U32& d, U32& t,bool flip)
{
	static bool first = true;


	if(first) {
		first = false;
		ilInit();
	}
	if(flip) ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	else ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);
		ilEnable(IL_TYPE_SET);
		ilTypeFunc(IL_UNSIGNED_BYTE);
	//}

	

    ILuint ilTexture;
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);

    
	if (!ilLoadImage(filename.c_str()))
		return false;

   
	w = ilGetInteger(IL_IMAGE_WIDTH);
	h = ilGetInteger(IL_IMAGE_HEIGHT);
	d = ilGetInteger(IL_IMAGE_BPP);
	t = ilGetInteger(IL_IMAGE_TYPE);


	if(d==4) ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

   
    const U8* Pixels = ilGetData();

	unsigned char* img = new unsigned char[(size_t)(w) * (size_t)(h) * (size_t)(d)];
	memcpy(img, Pixels, (size_t)(w) * (size_t)(h) * (size_t)(d));

    ilBindImage(0);
    ilDeleteImages(1, &ilTexture);

	return img;
}


unsigned char* OpenImage(const string& filename, U32& w, U32& h, U32& d, U32& t,bool flip)
{
	if(filename.find(".ppm") != string::npos)
		return OpenImagePPM(filename, w, h, d,t,flip);
	else 
		return OpenImageDevIL(filename,w,h,d,t,flip);
	return NULL;
}

void OpenImage(const string& filename, ImageData& img)
{
	img.name = filename;
	img.data = OpenImage(filename, img.w, img.h, img.d, img.type,img._flip);
}


void ImageData::Destroy()
{
	if(data) {
		delete [] data;
		data = NULL;
	}
}

ivec3 ImageData::getColor(U32 x, U32 y) const
{
	int idx = (y * w + x) * d;
	return ivec3( data[idx+0], data[idx+1], data[idx+2]);
}

}

