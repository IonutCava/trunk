#include "TextureManager.h"
#include "TextureManager/ImageTools.h"

// ===================================================================
TextureManager::TextureManager (void)
{
	
	szErrorMessage [0] = '\0';
	nNumTextures	   = 0;
	nAvailable     	   = 0;
	nTexIDs            = 0;
	cout << "Texture Manager Initialized!" << endl;
	nNumTextures = 0;
	nAvailable   = INITIAL_SIZE;
	nTexIDs      = new int [INITIAL_SIZE];
	for (int i = 0; i < nAvailable; i++) {
		nTexIDs [i] = -1;
	}
}

TextureManager::~TextureManager (void) {

}


void TextureManager::Destroy (void) {
		delete [] TEXMANAGER.nTexIDs;
		TEXMANAGER.nTexIDs = 0;
	
		TextureManager::DestroyInstance();
}

// ===================================================================

U32 TextureManager::LoadTexture (const std::string& szFilename, GLuint nTextureID) {

	U32 nWidth = 0, nHeight = 0, nBPP = 0;
	UBYTE *pData = 0;
	
	// Determine the type and actually load the file
	// ===========================================================================================
	char szCapFilename [80];
	int nLen = szFilename.length();
	for (int c = 0; c <= nLen; c++)	// <= to include the NULL as well
		szCapFilename [c] = toupper (szFilename [c]);


	pData = ImageTools::OpenImage(szFilename, nWidth, nHeight, nBPP,false);
	if (pData == 0)		return -1;

	// Assign a valid Texture ID (if one wasn't specified)
	// ===========================================================================================
	int nNewTextureID = GetNewTextureID (nTextureID);	// Also increases nNumTextures!

	// ===========================================================================================

	// Register and upload the texture in OpenGL
	glBindTexture (GL_TEXTURE_2D, nNewTextureID);

	// NOTE : Making some assumptions about texture parameters
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	gluBuild2DMipmaps (GL_TEXTURE_2D,
					   nBPP, nWidth, nHeight,
					   (nBPP == 3 ? GL_RGB : GL_RGBA),
					   GL_UNSIGNED_BYTE,
					   pData);

	delete [] pData;

	cout << "Loaded [" << szFilename  << "] W/O a hitch!"<< endl;
	return nNewTextureID;
}

int TextureManager::LoadTextureFromMemory (UBYTE *pData, int nWidth, int nHeight, int nBPP, int nTextureID) {

	// First we do ALOT of error checking on the data...
	if (!CheckSize (nWidth) || !CheckSize (nHeight)) {
		cout << "ERROR : Improper Dimension" << endl;
		return -1;
	}
	if (nBPP != 3 && nBPP != 4) {
		cout << "ERROR : Unsuported Color Depth" << endl;
		return -1;
	}

	// I guess were good to go...
	// ---------------------------------------------------------------------
	int nNewTextureID = GetNewTextureID (nTextureID);	// Also increases nNumTextures!

	// Register and upload the texture in OpenGL
	glBindTexture (GL_TEXTURE_2D, nNewTextureID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  //{Texture blends with object background}

	// NOTE : Making some assumptions about texture parameters
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gluBuild2DMipmaps (GL_TEXTURE_2D,
					   nBPP, nWidth, nHeight,
					   (nBPP == 3 ? GL_RGB : GL_RGBA),
					   GL_UNSIGNED_BYTE,
					   pData);
	// ---------------------------------------------------------------------

	 // delete [] pData;	// Leave memory clearing upto the caller...

	cout << "Loaded Some Memory Perfectly!" << endl;
	return nNewTextureID;
}

void TextureManager::FreeTexture (int nID) {
	int nIndex = -1;
	for (int i = 0; i < TEXMANAGER.nAvailable; i++) {
		if (TEXMANAGER.nTexIDs [i] == nID) {
			TEXMANAGER.nTexIDs [i] = -1;
			nIndex = i;	// to indicate a match was found
			break;		// their _should_ only be one instance of nID (if any)
		}
	}

	if (nIndex != -1) {
		U32 uiGLID = (U32) nID;
		glDeleteTextures (1, &uiGLID);
	}
}

void TextureManager::FreeAll (void) {
	
	// copy the ids to an U32eger array, so GL will like it ;)
	U32 *pUIIDs = new U32 [TEXMANAGER.nNumTextures];
	int i, j;
	for (i = 0, j = 0; i < TEXMANAGER.nNumTextures; i++) {
		if (TEXMANAGER.nTexIDs [i] != -1) {
			pUIIDs [j] = TEXMANAGER.nTexIDs [i];
			j++;
		}
	}

	glDeleteTextures (TEXMANAGER.nNumTextures, pUIIDs);

	delete [] pUIIDs;
	delete [] TEXMANAGER.nTexIDs;
	TEXMANAGER.nTexIDs = new int [INITIAL_SIZE];
	TEXMANAGER.nAvailable = INITIAL_SIZE;
	for (i = 0; i < INITIAL_SIZE; i++)
		TEXMANAGER.nTexIDs [i] = -1;
	
	TEXMANAGER.nNumTextures = 0;
}

int TextureManager::GetNewTextureID (int nPossibleTextureID) {

	// First check if the possible textureID has already been
	// used, however the default value is -1, err that is what
	// this method is passed from LoadTexture ()
	if (nPossibleTextureID != -1) {
		for (int i = 0; i < TEXMANAGER.nAvailable; i++) {
			if (TEXMANAGER.nTexIDs [i] == nPossibleTextureID) {
				FreeTexture (nPossibleTextureID);	// sets nTexIDs [i] to -1...
				TEXMANAGER.nNumTextures--;		// since we will add the ID again...
				break;
			}
		}
	}

	// Actually look for a new one
	int nNewTextureID;
	if (nPossibleTextureID == -1) {
		U32 nGLID;	
		glGenTextures (1, &nGLID);
		nNewTextureID = (int) nGLID;
	}
	else	// If the user is handle the textureIDs
		nNewTextureID = nPossibleTextureID;
	
	// find an empty slot in the TexID array
	int nIndex = 0;
	while (TEXMANAGER.nTexIDs [nIndex] != -1 && nIndex < TEXMANAGER.nAvailable)
		nIndex++;

	// all space exaused, make MORE!
	if (nIndex >= TEXMANAGER.nAvailable) {
		int *pNewIDs = new int [TEXMANAGER.nAvailable + TEXTURE_STEP];
		int i;
		
		// copy the old
		for (i = 0; i < TEXMANAGER.nAvailable; i++)
			pNewIDs [i] = TEXMANAGER.nTexIDs [i];
		// set the last increment to the newest ID
		pNewIDs [TEXMANAGER.nAvailable] = nNewTextureID;
		// set the new to '-1'
		for (i = 1; i < TEXTURE_STEP; i++)
			pNewIDs [i + TEXMANAGER.nAvailable] = -1;

		TEXMANAGER.nAvailable += TEXTURE_STEP;
		delete [] TEXMANAGER.nTexIDs;
		TEXMANAGER.nTexIDs = pNewIDs;
	}
	else
		TEXMANAGER.nTexIDs [nIndex] = nNewTextureID;

	// Welcome to our Texture Array!
	TEXMANAGER.nNumTextures++;
	return nNewTextureID;
}

// ===================================================================

char *TextureManager::GetErrorMessage (void) {
	return TEXMANAGER.szErrorMessage;
}

bool TextureManager::CheckSize (int nDimension) {
	// Portability issue, check your endian...

	int i = 1;
	while (i < nDimension) {
		i <<= 1;
		if (i == nDimension)
			return true;
	}

	return false;
}

int	TextureManager::GetNumTextures (void) {
	return TEXMANAGER.nNumTextures;
}

int TextureManager::GetAvailableSpace (void) {
	return TEXMANAGER.nAvailable;
}

int TextureManager::GetTexID (int nIndex) {
	if (nIndex >= 0 && nIndex < TEXMANAGER.nAvailable)
		return TEXMANAGER.nTexIDs [nIndex];

	// else
	return 0;
}

U32 TextureManager::LoadDetailedTexture(const std::string& filename, bool alpha, bool repeat, bool filtering, bool mipmaps, F32 *texcoordwidth, F32 *texcoordheight)
{
    GLuint tex = GetNewTextureID(-1);
    U32 width, height, pixelsize,type;
    int filter_min, filter_mag;
    GLubyte *data/*, *rdata*/;
    int xSize2, ySize2;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
	string file = "assets/texturi/" + filename;

	data = ImageTools::OpenImage(file,width,height,type);
	
    if(data == NULL) 
		cout << "TextureManager: Could not load texture [" << file  << "] !" << endl;
 
	switch(type)
	{
	    case GL_LUMINANCE:
			pixelsize = 1;
			break;
		case GL_RGB:
		case GL_BGR:
			pixelsize = 3;
			break;
		case GL_RGBA:
		case GL_BGRA:
			cout << "Image [" <<  file << "] is using mode GL_RGBA!" << endl;
			pixelsize = 4;
			break;
		default:
			cout << "glmLoadTexture(): unknown type 0x" << type << endl;
			pixelsize = 0;
			break;
    }


    if((pixelsize*width) % 4 == 0)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    else
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    xSize2 = width;
	ySize2 = height;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);   
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  //{Texture blends with object background}
	
    if(filtering) 
	{
		filter_min = (mipmaps) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
		filter_mag = GL_LINEAR;
    }
    else 
	{
		filter_min = (mipmaps) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
		filter_mag = GL_NEAREST;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);
   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (repeat) ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (repeat) ? GL_REPEAT : GL_CLAMP);

	
    if(mipmaps)		
			gluBuild2DMipmaps(GL_TEXTURE_2D, type, xSize2, ySize2, type, GL_UNSIGNED_BYTE, data);        
	else
			glTexImage2D(GL_TEXTURE_2D, 0, type, xSize2, ySize2, 0, type, GL_UNSIGNED_BYTE, data);   
 
    delete data;

	*texcoordwidth = (GLfloat)xSize2;		// size of texture coords 
	*texcoordheight = (GLfloat)ySize2;

    return tex;
}


int TextureManager::tgaSave(	char 		*filename, 
		short int	width, 
		short int	height, 
		UBYTE	pixelDepth,
		UBYTE	*imageData) {

	UBYTE cGarbage = 0, type,mode,aux;
	short int iGarbage = 0;
	int i;
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
	fwrite(&cGarbage, sizeof(UBYTE), 1, file);
	fwrite(&cGarbage, sizeof(UBYTE), 1, file);

	fwrite(&type, sizeof(UBYTE), 1, file);

	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&cGarbage, sizeof(UBYTE), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);

	fwrite(&width, sizeof(short int), 1, file);
	fwrite(&height, sizeof(short int), 1, file);
	fwrite(&pixelDepth, sizeof(UBYTE), 1, file);

	fwrite(&cGarbage, sizeof(UBYTE), 1, file);

// convert the image data from RGB(a) to BGR(A)
	if (mode >= 3)
	for (i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

// save the image data
	fwrite(imageData, sizeof(UBYTE), 
			width * height * mode, file);
	fclose(file);
// release the memory
	delete imageData;

	return(0);
}

// saves a series of files with names "filenameX.tga"
int TextureManager::SaveSeries(char		*filename, 
			 short int		width, 
			 short int		height, 
			 UBYTE	pixelDepth,
			 UBYTE	*imageData) {
	static int savedImages=0;
	char *newFilename;
	int status;
	
// compute the new filename by adding the 
// series number and the extension
	newFilename = new char[strlen(filename)+8];

	sprintf(newFilename,"%s%d.tga",filename,savedImages);
	
// save the image
	status = tgaSave(newFilename,width,height,pixelDepth,imageData);
	
//increase the counter
	if (status == 0)
		savedImages++;
	free(newFilename);
	return(status);
}

// converts RGB to greyscale
void TextureManager::tgaRGBtoGreyscale(TextureInfo *info)
{
	U32 mode,i,j;
	UBYTE *newImageData;

	if (info->bpp == 8)		return;

	mode = info->bpp / 8;
	newImageData = new UBYTE[info->height * info->width];
	if (newImageData == NULL) {
		return;
	}

// convert pixels: greyscale = o.30 * R + 0.59 * G + 0.11 * B
	for (i = 0,j = 0; j < (U32)info->width * info->height; i +=mode, j++)
		newImageData[j] =	
			(UBYTE)(0.30 * info->imageData[i] + 
					0.59 * info->imageData[i+1] +
					0.11 * info->imageData[i+2]);

	delete info->imageData;
	info->bpp = 8;
	info->type = 3;
	info->imageData = newImageData;
}
