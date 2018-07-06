#ifndef TEXMGR_H_
#define TEXMGR_H_

#include "Hardware/Video/Texture.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN( TextureManager )

public :
	TextureManager (void);
	~TextureManager (void);
	static void Destroy (void);
	I8 tgaSave(char *filename, U16 width, U16 height, U8 pixelDepth, U8 *imageData);
	I8 SaveSeries(char *filename, U16 width,U16 height,U8 pixelDepth,U8 *imageData);


SINGLETON_END()

#endif