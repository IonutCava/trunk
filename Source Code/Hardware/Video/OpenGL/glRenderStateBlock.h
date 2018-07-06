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

#ifndef _GL_RENDER_STATE_BLOCK_H
#define _GL_RENDER_STATE_BLOCK_H

#include "Hardware/Video/RenderStateBlock.h"

class glRenderStateBlock : public RenderStateBlock {   

public:

   glRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
   virtual ~glRenderStateBlock();

   void activate(glRenderStateBlock* oldState);

   inline U32 getHash() const {return _hashValue;}

   inline const RenderStateBlockDescriptor& getDescriptor() {return _descriptor;}   


private:
   RenderStateBlockDescriptor _descriptor;
   U32 _hashValue;
};
#endif