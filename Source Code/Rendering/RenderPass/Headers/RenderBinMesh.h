/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

#ifndef _RENDER_BIN_MESH_H_
#define _RENDER_BIN_MESH_H_

#include "RenderBin.h"
///This particular Rendering Bin handles imported geometry, be it skinned or otherwise
class RenderBinMesh : public RenderBin {
public:
	RenderBinMesh(const RenderBinType& rbType,const RenderingOrder::List& renderOrder, D32 drawKey);
	~RenderBinMesh();
};

#endif