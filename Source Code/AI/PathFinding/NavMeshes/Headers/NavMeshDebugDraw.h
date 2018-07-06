/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _NAV_MESH_DEBUG_DRAW_H_
#define _NAV_MESH_DEBUG_DRAW_H_

#include <DebugUtils/Include/DebugDraw.h>
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

class IMPrimitive;
class RenderStateBlock;

namespace Navigation {
   /// Convert a Rcast color integer to RGBA components.
   inline void rcCol(U32 col, U8 &r, U8 &g, U8 &b, U8 &a)
   {
      r = col % 256; col /= 256;
      g = col % 256; col /= 256;
      b = col % 256; col /= 256;
      a = col % 256;
   }

    class NavMeshDebugDraw : public duDebugDraw {
    public:
          NavMeshDebugDraw();
          ~NavMeshDebugDraw();

          void depthMask(bool state);
          void texture(bool state);

          void overrideColor(U32 col);
          void cancelOverride();
          void beginBatch();
          void endBatch();

          void begin(duDebugDrawPrimitives prim, F32 size = 1.0f);
          void vertex(const F32* pos, U32 color);
          void vertex(const F32 x, const F32 y, const F32 z, U32 color);
          void vertex(const F32* pos, U32 color, const F32* uv);
          void vertex(const F32 x, const F32 y, const F32 z, U32 color, const F32 u, const F32 v);
          void end();

          inline void setDirty(const bool state)       {_dirty = state;}
          inline bool isDirty()                  const {return _dirty; }

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
        bool _dirty;
    };
};
#endif