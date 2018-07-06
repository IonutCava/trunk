#ifndef TEXMGR_H_
#define TEXMGR_H_

#include "TextureManager/Texture.h"
#include "Utility/Headers/Singleton.h"

#define TEXMANAGER	TextureManager::getInstance()
#define DESTROY_TEXMANAGER	TextureManager::Destroy();

#define INITIAL_SIZE	32	// initial size of the TexID array
#define TEXTURE_STEP	8	// how much the array grows by each time

SINGLETON_BEGIN( TextureManager )

public :
	TextureManager (void);
	~TextureManager (void);

private :	
	GLint gl_max_texture_size;
public :
	static void Destroy (void);

public :	// Usage / Implementation
	U32 LoadTexture (const std::string& szFilename, U32 nTextureID = -1);
	int LoadTextureFromMemory (UBYTE *pData, int nWidth, int nHeight, int nBPP, int nTextureID = -1);
	U32 LoadDetailedTexture (const std::string& filename, bool alpha, bool repeat, bool filtering, bool mipmaps, F32 *texcoordwidth, F32 *texcoordheight);

	void FreeTexture (int nID);
	void FreeAll (void);

public :	// Debug / Utilitarian
	char *GetErrorMessage (void);
	
	int	GetNumTextures (void);
	int GetAvailableSpace (void);
	int GetTexID (int nIndex);
	int tgaSave(char *filename,short int width, short int height, UBYTE pixelDepth, UBYTE *imageData);
	int SaveSeries(char *filename,short int width,short int height,UBYTE	pixelDepth,UBYTE *imageData);
	bool LoadTGAImage(TextureInfo * texture,const char *filename);
	void tgaRGBtoGreyscale(TextureInfo *info);
	UBYTE *LoadTargaFile (const std::string& filename, int &nWidth, int &nHeight, int &nBPP);
private :

	UBYTE *LoadBitmapFile (const std::string& filename, int &nWidth, int &nHeight, int &nBPP);
	

	bool LoadCompressedTGA(TextureInfo * texture, const char * filename, FILE * fTGA);
	bool LoadUncompressedTGA(TextureInfo * texture, const char * filename, FILE * fTGA);
	int GetNewTextureID (int nPossibleTextureID);	// get a new one if one isn't provided
	bool CheckSize (int nDimension);

private :
	char szErrorMessage [80];	
	int nNumTextures;
	int nAvailable;	
	int *nTexIDs;

	SINGLETON_END()

#endif