#include "TextureManager.h"
#include "TextureManager/ImageTools.h"

// ===================================================================
TextureManager::TextureManager (void)
{
	Con::getInstance().printfn("Texture Manager Initialized!");

}

TextureManager::~TextureManager (void) {

}


void TextureManager::Destroy (void) {
	
		TextureManager::DestroyInstance();
}

// ===================================================================


I8 TextureManager::tgaSave(	char 		*filename, 
		U16	width, 
		U16	height, 
		U8	pixelDepth,
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
// release the memory
	delete imageData;
	imageData = NULL;
	return(0);
}

// saves a series of files with names "filenameX.tga"
I8 TextureManager::SaveSeries(char		*filename, 
			 U16		width, 
			 U16		height, 
			 U8	pixelDepth,
			 U8	*imageData) {
	static I8 savedImages=0;
	char *newFilename;
	I8 status;
	
// compute the new filename by adding the 
// series number and the extension
	newFilename = new char[strlen(filename)+8];

	sprintf_s(newFilename,strlen(filename)+8,"%s%d.tga",filename,savedImages);
	
// save the image
	status = tgaSave(newFilename,width,height,pixelDepth,imageData);
	
//increase the counter
	if (status == 0)
		savedImages++;
	delete[] newFilename;
	newFilename = NULL;
	return(status);
}

