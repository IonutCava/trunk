#ifndef _D3D_ENUM_TABLE_H_
#define _D3D_ENUM_TABLE_H_

/*�Copyright 2009-2012 DIVIDE-Studio�*/
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

#include "Hardware/Video/RenderAPIEnums.h"
#include "d3dResources.h"

namespace D3D_ENUM_TABLE
{
   void fill();
};
extern unsigned int d3dTextureTypeTable[TEXTURE_TYPE_PLACEHOLDER];
#endif