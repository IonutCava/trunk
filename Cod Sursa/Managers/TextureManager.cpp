#include "TextureManager.h"

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

	int nWidth = 0, nHeight = 0, nBPP = 0;
	UBYTE *pData = 0;
	
	// Determine the type and actually load the file
	// ===========================================================================================
	char szCapFilename [80];
	int nLen = szFilename.length();
	for (int c = 0; c <= nLen; c++)	// <= to include the NULL as well
		szCapFilename [c] = toupper (szFilename [c]);
	
	if (strcmp (szCapFilename + (nLen - 3), "BMP") == 0 ||	strcmp (szCapFilename + (nLen - 3), "TGA") == 0) 
	{
		// Actually load them
		if (strcmp (szCapFilename + (nLen - 3), "BMP") == 0) {
			pData = LoadBitmapFile (szFilename, nWidth, nHeight, nBPP);
			if (pData == 0)
				return -1;
		}
		if (strcmp (szCapFilename + (nLen - 3), "TGA") == 0) {
			pData = LoadTargaFile (szFilename, nWidth, nHeight, nBPP);
			if (pData == 0)
				return -1;
		}
	}
	else {
		cout << "ERROR : Unable to load extension [" << szCapFilename + (nLen - 3) << "]" << endl;
		return -1;
	}
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

// ===================================================================

UBYTE *TextureManager::LoadBitmapFile (const std::string& filename, int &nWidth, int &nHeight, int &nBPP) {
	

	// These are both defined in Windows.h
	BITMAPFILEHEADER	BitmapFileHeader;
	BITMAPINFOHEADER	BitmapInfoHeader;
	
	// Old Skool C-style code	
	FILE	*pFile;
	UBYTE	*pImage;			// bitmap image data
	UBYTE	tempRGB;				// swap variable

	// open filename in "read binary" mode
	pFile = fopen(filename.c_str(), "rb");
	if (pFile == 0) {
		
		cout << "ERROR : [" << filename << "] File Not Found!" << endl;	
		return 0;
	}

	// Header
	fread (&BitmapFileHeader, sizeof (BITMAPFILEHEADER), 1, pFile);
	if (BitmapFileHeader.bfType != 'MB') {
		
		cout << "ERROR : [" << filename << "] Is not a valid Bitmap!" << endl;
		fclose (pFile);
		return 0;
	}

	// Information
	fread (&BitmapInfoHeader, sizeof (BITMAPINFOHEADER), 1, pFile);

	if (!CheckSize (BitmapInfoHeader.biWidth) || !CheckSize (BitmapInfoHeader.biHeight)) {

		cout << "ERROR : Improper Dimension" << endl;;
		fclose (pFile);
		return 0;
	}

	
	fseek (pFile, BitmapFileHeader.bfOffBits, SEEK_SET);
	pImage = new UBYTE [BitmapInfoHeader.biSizeImage];
	if (!pImage) {
		delete [] pImage;
		
		cout << "ERROR : Out Of Memory!" << endl;

		fclose (pFile);
		return 0;
	}
	fread (pImage, 1, BitmapInfoHeader.biSizeImage, pFile);

	// Turn BGR to RBG
	for (int i = 0; i < (int) BitmapInfoHeader.biSizeImage; i += 3) {
		tempRGB = pImage [i];
		pImage [i + 0] = pImage [i + 2];
		pImage [i + 2] = tempRGB;
	}

	fclose(pFile);

	// THIS IS CRUCIAL!  The only way to relate the size information to the
	// OpenGL functions back in ::LoadTexture ()
	nWidth  = BitmapInfoHeader.biWidth;
	nHeight = BitmapInfoHeader.biHeight;
	nBPP    = 3;	// Only load 24-bit Bitmaps

	return pImage;
}

UBYTE *TextureManager::LoadTargaFile (const std::string& filename, int &nWidth, int &nHeight, int &nBPP) {

	cout << "Opening TGA: " << filename << endl;
	// Get those annoying data structures out of the way...
	struct {
		UBYTE imageTypeCode;
		short int imageWidth;
		short int imageHeight;
		UBYTE bitCount;
	} TgaHeader;

	// Let 'er rip!
	FILE	*pFile;
	UBYTE	uCharDummy;
	short	sIntDummy;
	UBYTE	colorSwap;	// swap variable
	UBYTE	*pImage;	// the TGA data

	// open the TGA file
	pFile = fopen (filename.c_str(), "rb");
	if (!pFile) {
	
		cout << "ERROR : [" << filename << "] File Not Found!" << endl;
		return 0;
	}

	// Ignore the first two bytes
	fread (&uCharDummy, sizeof (UBYTE), 1, pFile);
	fread (&uCharDummy, sizeof (UBYTE), 1, pFile);

	// Pop in the header
	fread(&TgaHeader.imageTypeCode, sizeof (UBYTE), 1, pFile);

	// Only loading RGB and RGBA types
	if ((TgaHeader.imageTypeCode != 2) && (TgaHeader.imageTypeCode != 3))
	{

		cout << "ERROR : Unsuported Image Type (Color Depth or Compression)" << endl;
		
		fclose (pFile);
		return 0;
	}

	// More data which isn't important for now
	fread (&uCharDummy, sizeof (UBYTE), 1, pFile);
	fread (&sIntDummy,  sizeof (short), 1, pFile);
	fread (&sIntDummy,  sizeof (short), 1, pFile);
	fread (&sIntDummy,  sizeof (short), 1, pFile);
	fread (&sIntDummy,  sizeof (short), 1, pFile);

	// Get some rather important data
	fread (&TgaHeader.imageWidth,  sizeof (short int), 1, pFile);
	fread (&TgaHeader.imageHeight, sizeof (short int), 1, pFile);
	fread (&TgaHeader.bitCount, sizeof (UBYTE), 1, pFile);

	// Skip past some more
	fread (&uCharDummy, sizeof (UBYTE), 1, pFile);

	// THIS IS CRUCIAL
	nBPP    = TgaHeader.bitCount / 8;
	nWidth  = TgaHeader.imageWidth;
	nHeight = TgaHeader.imageHeight;
	
   
	if (!CheckSize (nWidth) || !CheckSize (nHeight)) {

		cout << "ERROR : Improper Dimension" << endl;
		fclose (pFile);
		return 0;
	}


	int nImageSize = nWidth * nHeight * nBPP;
	pImage = new UBYTE [nImageSize];
	if (pImage == 0) {
		
		cout << "ERROR : Out Of Memory" << endl;
		return 0;
	}

	// actually read it (finally!)
	fread (pImage, sizeof (UBYTE), nImageSize, pFile);

	// BGR to RGB
	for (int i = 0; i < nImageSize; i += nBPP) {
		colorSwap = pImage [i + 0];
		pImage [i + 0] = pImage [i + 2];
		pImage [i + 2] = colorSwap;
	}
	fclose (pFile);

	return pImage;
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
    int width, height,pixelsize;
    int type;
    int filter_min, filter_mag;
    GLubyte *data/*, *rdata*/;
    int xSize2, ySize2;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
	std::string file = "assets/texturi/"+filename;
	const char *numefis = file.c_str();
	
    while (*numefis==' ') numefis++;
	TextureInfo ttt;
	LoadTGAImage(&ttt,numefis);
	data = ttt.imageData;
	width = ttt.width;
	height = ttt.height;
	type = ttt.mode;
	
    if(data == NULL) 
	{
		char err[80];
		cout << "Nu am putut incarca textura " << numefis  << "!" << endl;
		MessageBoxA(NULL, err, "ERROR", NULL);
    }


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
			cout << "Image: " <<  numefis << " is using mode GL_RGBA!" << endl;
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


bool TextureManager::LoadTGAImage(TextureInfo * texture, const char* filename)				// Load a TGA file
{
	 TGAHeader tgaheader;									// TGA header
     GLubyte uTGAcompare[12] = {0,0,2,0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
     GLubyte cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};	// Compressed TGA Header
	 GLubyte tTGAcompare[12] = {0,0,3,0,0,0,0,0,0,0,0,0};   // Terrain TGA Header (8bpp / grayscale)

	FILE * fTGA;		
	/*
	 *	Create a file pointer called fp and attempt to open the file
	 */
	FILE *fp = NULL;
	cout << "Openning texture: "<< filename << endl;
	
    fopen_s(&fTGA, filename, "rb");								// Open file for reading

	if(fTGA == NULL)											// If it didn't open....
	{		
		MessageBoxA(NULL, "Could not open texture file", filename, MB_OK);	// Display an error message
		texture->status = -1;
		return false;														// Exit function
	}
   
	if(fread(&tgaheader, sizeof(TGAHeader), 1, fTGA) == 0)					// Attempt to read 12 byte header from file
	{
		MessageBoxA(NULL, "Could not read file header", filename, MB_OK);		// If it fails, display an error message 
		texture->status = -2;
		if(fTGA != NULL)													// Check to seeiffile is still open
		{
			fclose(fTGA);													// If it is, close it
		}
		return false;														// Exit function
	}

	if(memcmp(uTGAcompare, &tgaheader, sizeof(tgaheader)) == 0)				// See if header matches the predefined header of 
	{																		// an Uncompressed TGA image
		LoadUncompressedTGA(texture, filename, fTGA);						// If so, jump to Uncompressed TGA loading code
	}
	else if(memcmp(cTGAcompare, &tgaheader, sizeof(tgaheader)) == 0)		// See if header matches the predefined header of
	{																		// an RLE compressed TGA image
		LoadCompressedTGA(texture, filename, fTGA);							// If so, jump to Compressed TGA loading code
	}
	else if(memcmp(tTGAcompare,&tgaheader,sizeof(tgaheader)) == 0)
	{
		LoadUncompressedTGA(texture,filename,fTGA);
	}
	else																	// If header matches neither type
	{
		MessageBoxA(NULL, "TGA file must be type 2, 3 or type 10 ", filename, MB_OK);	// Display an error
		texture->status = -3;
		fclose(fTGA);
		return false;																// Exit function
	}
	return true;															// All went well, continue on
}

bool TextureManager::LoadUncompressedTGA(TextureInfo * texture, const char * filename, FILE * fTGA)	// Load an uncompressed TGA (note, much of this code is based on NeHe's 
{																			// TGA Loading code nehe.gamedev.net)
	TextureInfo tga;										             		// TGA image data
	if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)					// Read TGA header
	{										
		MessageBoxA(NULL, "Could not read info header", "ERROR", MB_OK);		// Display error
		if(fTGA != NULL)													// if file is still open
		{
			fclose(fTGA);													// Close it
		}
		return false;														// Return failular
	}	
	texture->width  = tga.header[1] * 256 + tga.header[0];					// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = tga.header[3] * 256 + tga.header[2];					// Determine The TGA Height	(highbyte*256+lowbyte)
	texture->bpp	= tga.header[4];										// Determine the bits per pixel
	tga.width	= texture->width;										// Copy width into local structure						
	tga.height	= texture->height;										// Copy height into local structure
	tga.bpp		= texture->bpp;											// Copy BPP into local structure
	
    
	if((texture->width <= 0) || (texture->height <= 0) || ((texture->bpp != 24) && (texture->bpp !=32) && (texture->bpp != 8)))	// Make sure all information is valid
	{
		MessageBoxA(NULL, "Invalid texture information", filename, MB_OK);	// Display Error
		if(fTGA != NULL)													// Check if file is still open
		{
			fclose(fTGA);													// If so, close it
		}
		return false;														// Return failed
	}

	if(texture->bpp == 24)													// If the BPP of the image is 24...
		texture->mode	= GL_RGB;											// Set Image type to GL_RGB
	else if(texture->bpp == 32)												// Else if its 32 BPP
		texture->mode	= GL_RGBA;											// Set image type to GL_RGBA
	else
		texture->mode = GL_LUMINANCE;

	tga.bpp         	= (tga.bpp / 8);								// Compute the number of BYTES per pixel
	tga.imageSize		= (tga.bpp * tga.width * tga.height);		    // Compute the total amout ofmemory needed to store data
	texture->imageData	= new GLubyte[tga.imageSize];					// Allocate that much memory

	if(texture->imageData == NULL)											// If no space was allocated
	{
		MessageBoxA(NULL, "Could not allocate memory for image", filename, MB_OK);	// Display Error
		fclose(fTGA);														// Close the file
		return false;														// Return failed
	}

	if(fread(texture->imageData, 1, tga.imageSize, fTGA) != tga.imageSize)	// Attempt to read image data
	{
		MessageBoxA(NULL, "Could not read image data", "ERROR", MB_OK);		// Display Error
		if(texture->imageData != NULL)										// If imagedata has data in it
		{
			delete texture->imageData;										// Delete data from memory
		}
		fclose(fTGA);														// Close file
		return false;														// Return failed
	}

	// Byte Swapping Optimized By Steve Thomas
	for(GLuint cswap = 0; cswap < (int)tga.imageSize; cswap += tga.bpp)
	{
		texture->imageData[cswap] ^= texture->imageData[cswap+2] ^=
		texture->imageData[cswap] ^= texture->imageData[cswap+2];
	}

	fclose(fTGA);															// Close file
	texture->status = 0;
	return true;															// Return success
}

bool TextureManager::LoadCompressedTGA(TextureInfo * texture, const char * filename, FILE * fTGA)		// Load COMPRESSED TGAs
{ 
	TextureInfo tga;												// TGA image data
	if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)					// Attempt to read header
	{
		//MessageBox(NULL, "Could not read info header", "ERROR", MB_OK);		// Display Error
		if(fTGA != NULL)													// If file is open
		{
			fclose(fTGA);													// Close it
		}
		return false;														// Return failed
	}

	texture->width  = tga.header[1] * 256 + tga.header[0];					// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = tga.header[3] * 256 + tga.header[2];					// Determine The TGA Height	(highbyte*256+lowbyte)
	texture->bpp	= tga.header[4];										// Determine Bits Per Pixel
    tga.width		= texture->width;										// Copy width to local structure
	tga.height		= texture->height;										// Copy width to local structure
	tga.bpp			= texture->bpp;											// Copy width to local structure
	if((texture->width <= 0) || (texture->height <= 0) || ((texture->bpp != 24) && (texture->bpp !=32)))	//Make sure all texture info is ok
	{
		//MessageBox(NULL, "Invalid texture information", "ERROR", MB_OK);	// If it isnt...Display error
		if(fTGA != NULL)													// Check if file is open
		{
			fclose(fTGA);													// Ifit is, close it
		}
		return false;														// Return failed
	}

	if(texture->bpp == 24)													// If the BPP of the image is 24...
		texture->mode	= GL_RGB;											// Set Image type to GL_RGB
	else																	// Else if its 32 BPP
		texture->mode	= GL_RGBA;											// Set image type to GL_RGBA

	tga.bpp	= (tga.bpp / 8);							            		// Compute BYTES per pixel
	tga.imageSize		= (tga.bpp * tga.width * tga.height);		        // Compute amout of memory needed to store image
	texture->imageData	= new GLubyte[tga.imageSize];					// Allocate that much memory

	if(texture->imageData == NULL)											// If it wasnt allocated correctly..
	{
		MessageBoxA(NULL, "Could not allocate memory for image", filename, MB_OK);	// Display Error
		fclose(fTGA);														// Close file
		return false;														// Return failed
	}

	GLuint pixelcount	= tga.height * tga.width;							// Nuber of pixels in the image
	GLuint currentpixel	= 0;												// Current pixel being read
	GLuint currentbyte	= 0;												// Current byte 
	GLubyte * colorbuffer = new GLubyte[tga.bpp];							// Storage for 1 pixel

	do
	{
		GLubyte chunkheader = 0;											// Storage for "chunk" header

		if(fread(&chunkheader, sizeof(GLubyte), 1, fTGA) == 0)				// Read in the 1 byte header
		{
			MessageBoxA(NULL, "Could not read RLE header", filename, MB_OK);	// Display Error
			if(fTGA != NULL)												// If file is open
			{
				fclose(fTGA);												// Close file
			}
			if(texture->imageData != NULL)									// If there is stored image data
			{
				delete texture->imageData;									// Delete image data
			}
			return false;													// Return failed
		}

		if(chunkheader < 128)												// If the ehader is < 128, it means the that is the number of RAW color packets minus 1
		{																	// that follow the header
			chunkheader++;													// add 1 to get number of following color values
			for(short counter = 0; counter < chunkheader; counter++)		// Read RAW color values
			{
				if(fread(colorbuffer, 1, tga.bpp, fTGA) != tga.bpp) // Try to read 1 pixel
				{
					MessageBoxA(NULL, "Could not read image data", filename, MB_OK);		// IF we cant, display an error

					if(fTGA != NULL)													// See if file is open
					{
						fclose(fTGA);													// If so, close file
					}

					if(colorbuffer != NULL)												// See if colorbuffer has data in it
					{
						free(colorbuffer);												// If so, delete it
					}

					if(texture->imageData != NULL)										// See if there is stored Image data
					{
						delete texture->imageData;										// If so, delete it too
					}

					return false;														// Return failed
				}
																						// write to memory
				texture->imageData[currentbyte		] = colorbuffer[2];				    // Flip R and B vcolor values around in the process 
				texture->imageData[currentbyte + 1	] = colorbuffer[1];
				texture->imageData[currentbyte + 2	] = colorbuffer[0];

				if(tga.bpp == 4)												// if its a 32 bpp image
				{
					texture->imageData[currentbyte + 3] = colorbuffer[3];				// copy the 4th byte
				}
 
				currentbyte += tga.bpp;										            // Increase thecurrent byte by the number of bytes per pixel
				currentpixel++;											 				// Increase current pixel by 1

				if(currentpixel > pixelcount)											// Make sure we havent read too many pixels
				{
					MessageBoxA(NULL, "Too many pixels read", filename, NULL);			// if there is too many... Display an error!

					if(fTGA != NULL)													// If there is a file open
					{
						fclose(fTGA);													// Close file
					}	

					if(colorbuffer != NULL)												// If there is data in colorbuffer
					{
						free(colorbuffer);												// Delete it
					}

					if(texture->imageData != NULL)										// If there is Image data
					{
						free(texture->imageData);										// delete it
					}

					return false;														// Return failed
				}
			}
		}
		else																			// chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
		{
			chunkheader -= 127;															// Subteact 127 to get rid of the ID bit
			if(fread(colorbuffer, 1, tga.bpp, fTGA) != tga.bpp)		                    // Attempt to read following color values
			{	
				MessageBoxA(NULL, "Could not read from file", filename, MB_OK);			// If attempt fails.. Display error (again)

				if(fTGA != NULL)														// If thereis a file open
				{
					fclose(fTGA);														// Close it
				}

				if(colorbuffer != NULL)													// If there is data in the colorbuffer
				{
					free(colorbuffer);													// delete it
				}

				if(texture->imageData != NULL)											// If thereis image data
				{
					free(texture->imageData);											// delete it
				}

				return false;															// return failed
			}

			for(short counter = 0; counter < chunkheader; counter++)					// copy the color into the image data as many times as dictated 
			{																			// by the header
				texture->imageData[currentbyte		] = colorbuffer[2];					// switch R and B bytes areound while copying
				texture->imageData[currentbyte + 1	] = colorbuffer[1];
				texture->imageData[currentbyte + 2	] = colorbuffer[0];

				if(tga.bpp == 4)												         // If TGA images is 32 bpp
				{
					texture->imageData[currentbyte + 3] = colorbuffer[3];				// Copy 4th byte
				}

				currentbyte += tga.bpp;									                // Increase current byte by the number of bytes per pixel
				currentpixel++;															// Increase pixel count by 1

				if(currentpixel > pixelcount)											// Make sure we havent written too many pixels
				{
					MessageBoxA(NULL, "Too many pixels read", filename, NULL);			// if there is too many... Display an error!

					if(fTGA != NULL)													// If there is a file open
					{
						fclose(fTGA);													// Close file
					}	

					if(colorbuffer != NULL)												// If there is data in colorbuffer
					{
						free(colorbuffer);												// Delete it
					}

					if(texture->imageData != NULL)										// If there is Image data
					{
						free(texture->imageData);										// delete it
					}

					return false;														// Return failed
				}
			}
		}
	}

	while(currentpixel < pixelcount);													// Loop while there are still pixels left
	fclose(fTGA);																		// Close the file
	return true;																		// return success
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
