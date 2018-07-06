/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _GL_ENUM_TABLE_H_
#define _GL_ENUM_TABLE_H_

#include "Hardware/Video/RenderAPIEnums.h"

namespace GL_ENUM_TABLE
{
   ///Populate enumaration tables with apropriate API values
   void fill();
};

extern unsigned int glBlendTable[BLEND_PROPERTY_PLACEHOLDER];
extern unsigned int glBlendOpTable[BLEND_OPERATION_PLACEHOLDER];
extern unsigned int glCompareFuncTable[COMPARE_FUNC_PLACEHOLDER];
extern unsigned int glStencilOpTable[STENCIL_OPERATION_PLACEHOLDER];
extern unsigned int glCullModeTable[CULL_MODE_PLACEHOLDER];
extern unsigned int glFillModeTable[FILL_MODE_PLACEHOLDER];
extern unsigned int glTextureTypeTable[TEXTURE_TYPE_PLACEHOLDER];

#endif