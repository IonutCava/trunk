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

#ifndef _D3D_RENDER_STATE_BLOCK_H
#define _D3D_RENDER_STATE_BLOCK_H

#include "Hardware/Video/Headers/RenderStateBlock.h"

class d3dRenderStateBlock : public RenderStateBlock {
public:

   d3dRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
   virtual ~d3dRenderStateBlock();

   void activate(d3dRenderStateBlock* oldState);

   inline I64 getGUID() const {return _hashValue;}

   inline RenderStateBlockDescriptor& getDescriptor() {return _descriptor;}

private:
   RenderStateBlockDescriptor _descriptor;
   I64 _hashValue;
};
#endif