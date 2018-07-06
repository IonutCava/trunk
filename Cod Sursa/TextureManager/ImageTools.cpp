#include "resource.h"
#include "ImageTools.h"

#undef _UNICODE
#include "il.h"


namespace ImageTools {

GLubyte* OpenImagePPM(const std::string& filename, U32& w, U32& h, U32& d)
{

	char head[70];
	int i,j;
	GLubyte * img = NULL;

	FILE * f = fopen(filename.c_str(), "rb");

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
				i += sscanf(head, "%d %d %d", &w, &h, &d);
			else if(i==1)
				i += sscanf(head, "%d %d", &h, &d);
			else if(i==2)
				i += sscanf(head, "%d", &d);
		}

		img = new GLubyte[(size_t)(w) * (size_t)(h) * 3];
		if(img==NULL) {
			fclose(f);
			return 0; 
		}

		fread(img, sizeof(GLubyte), (size_t)w*(size_t)h*3,f);
		fclose(f);
	}
	else{
		fclose(f);
	}
	
	return img;
}


GLubyte* OpenImageDevIL(const std::string& filename, U32& w, U32& h, U32& d)
{
	static bool first = true;
	if(first) {
		first = false;
		ilInit();
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);
		ilEnable(IL_TYPE_SET);
		ilTypeFunc(IL_UNSIGNED_BYTE);
	}

    
    ILuint ilTexture;
    ilGenImages(1, &ilTexture);
    ilBindImage(ilTexture);

    
	if (!ilLoadImage(filename.c_str()))
		return false;


   
	w = ilGetInteger(IL_IMAGE_WIDTH);
	h = ilGetInteger(IL_IMAGE_HEIGHT);
	d = ilGetInteger(IL_IMAGE_BPP);


	if(d==4)
		ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

   
    const UBYTE* Pixels = ilGetData();

	GLubyte* img = new GLubyte[(size_t)(w) * (size_t)(h) * (size_t)(d)];
	memcpy(img, Pixels, (size_t)(w) * (size_t)(h) * (size_t)(d));

   
    ilBindImage(0);
    ilDeleteImages(1, &ilTexture);

	return img;
}


GLubyte* OpenImage(const std::string& filename, U32& w, U32& h, U32& d,bool terrain)
{
	
	if(filename.find(".ppm") != std::string::npos){
		return OpenImagePPM("assets/misc_images/"+filename, w, h, d);
	}
	else {
		if(terrain)
		{
			cout << "Loading image: " << "assets/teren/"+filename << endl;
			return OpenImageDevIL("assets/teren/"+filename,w,h,d);
		}
		else
			return OpenImageDevIL("assets/misc_images/"+filename, w, h, d);
	}
	std::cout << "Erreur de chargement de l'image " << filename << std::endl;
	return NULL;
}

void OpenImage(const std::string& filename, ImageData& img)
{
	img.data = OpenImage(filename, img.w, img.h, img.d);
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

