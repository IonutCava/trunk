#ifndef TEXMGR_H_
#define TEXMGR_H_

#include "Hardware/Video/Texture.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN( TextureManager )

public :
	TextureManager (void);
	~TextureManager (void);
	static void Destroy (void);
	int tgaSave(char *filename,short int width, short int height, U8 pixelDepth, U8 *imageData);
	int SaveSeries(char *filename,short int width,short int height,U8 pixelDepth,U8 *imageData);


SINGLETON_END()

#endif