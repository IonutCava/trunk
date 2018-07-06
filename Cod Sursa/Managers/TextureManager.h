#ifndef TEXMGR_H_
#define TEXMGR_H_

#include "TextureManager/Texture.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN( TextureManager )

public :
	TextureManager (void);
	~TextureManager (void);
	static void Destroy (void);
	int tgaSave(char *filename,short int width, short int height, UBYTE pixelDepth, UBYTE *imageData);
	int SaveSeries(char *filename,short int width,short int height,UBYTE	pixelDepth,UBYTE *imageData);


SINGLETON_END()

#endif