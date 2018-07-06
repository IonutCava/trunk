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

#ifndef _GL_ENUM_TABLE_H_
#define _GL_ENUM_TABLE_H_

#include "Hardware/Video/Headers/RenderAPIEnums.h"

namespace GL_ENUM_TABLE
{
   ///Populate enumaration tables with apropriate API values
   void fill();
};

extern unsigned int glBlendTable[BlendProperty_PLACEHOLDER];
extern unsigned int glBlendOpTable[BlendOperation_PLACEHOLDER];
extern unsigned int glCompareFuncTable[ComparisonFunction_PLACEHOLDER];
extern unsigned int glStencilOpTable[StencilOperation_PLACEHOLDER];
extern unsigned int glCullModeTable[CullMode_PLACEHOLDER];
extern unsigned int glFillModeTable[FillMode_PLACEHOLDER];
extern unsigned int glTextureTypeTable[TextureType_PLACEHOLDER];
extern unsigned int glImageFormatTable[IMAGE_FORMAT_PLACEHOLDER];
extern unsigned int glPrimitiveTypeTable[PrimitiveType_PLACEHOLDER];
extern unsigned int glDataFormat[GDF_PLACEHOLDER];
extern unsigned int glWrapTable[TextureWrap_PLACEHOLDER];
extern unsigned int glTextureFilterTable[TextureFilter_PLACEHOLDER];

#endif