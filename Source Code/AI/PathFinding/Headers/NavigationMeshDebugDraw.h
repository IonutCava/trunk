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
#ifndef _NAV_MESH_DEBUG_DRAW_H_
#define _NAV_MESH_DEBUG_DRAW_H_

#include <DebugUtils/Include/DebugDraw.h>
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

class IMPrimitive;
class RenderStateBlock;

namespace Navigation {
class NavMeshDebugDraw : public duDebugDraw {
public:
      NavMeshDebugDraw();
      ~NavMeshDebugDraw();

      void depthMask(bool state);
      void texture(bool state);

      void overrideColor(U32 col);
      void cancelOverride();

      void begin(duDebugDrawPrimitives prim, F32 size = 1.0f);
      void vertex(const F32* pos, U32 color);
      void vertex(const F32 x, const F32 y, const F32 z, U32 color);
      void vertex(const F32* pos, U32 color, const F32* uv);
      void vertex(const F32 x, const F32 y, const F32 z, U32 color, const F32 u, const F32 v);
      void end();

protected:
    void prepareMaterial();
    void releaseMaterial();

private:
    RenderStateBlock*   _navMeshStateBlock;
    IMPrimitive*        _primitive;
    U32 _vertCount;
    U32 _color;
    PrimitiveType _primType;
    bool _overrideColor;
};
};
#endif