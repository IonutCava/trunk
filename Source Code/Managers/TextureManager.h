/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

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