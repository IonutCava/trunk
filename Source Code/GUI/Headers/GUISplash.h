/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _GUI_SPLASH_H
#define _GUI_SPLASH_H

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <string>

class Quad3D;
class Texture;
template<class T>
class vec2;
typedef Texture Texture2D;
class GUISplash {
public:
	GUISplash(const std::string& splashImageName,const vec2<U16>& dimensions);
	~GUISplash();
	void render();

private:
	Quad3D* 	_renderQuad;
	Texture2D*	_splashImage;
};

#endif