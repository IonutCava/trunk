/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _NAV_MESH_DEBUG_DRAW_H_
#define _NAV_MESH_DEBUG_DRAW_H_

#include <ReCast/DebugUtils/Include/DebugDraw.h>
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class GFXDevice;
class IMPrimitive;
class RenderStateBlock;
class GenericDrawCommand;
namespace AI {
namespace Navigation {

/// Convert a Rcast colour integer to RGBA components.
inline void rcCol(U32 col, U8& r, U8& g, U8& b, U8& a) {
    r = col % 256;
    col /= 256;
    g = col % 256;
    col /= 256;
    b = col % 256;
    col /= 256;
    a = col % 256;
}

class NavMeshDebugDraw : public duDebugDraw {
   public:
    NavMeshDebugDraw(GFXDevice& context);
    ~NavMeshDebugDraw();

    void paused(bool state);
    void overrideColour(U32 col);
    void beginBatch();
    void endBatch();

    void begin(duDebugDrawPrimitives prim, F32 size = 1.0f);
    void vertex(const F32 x, const F32 y, const F32 z, U32 colour);
    void end();

    inline void setDirty(bool state) { _dirty = state; }
    inline bool isDirty() const { return _dirty; }
    inline bool paused() const { return _paused; }
    inline void cancelOverride() { _overrideColour = false; }
    inline void texture(bool state) {}
    inline void vertex(const F32* pos, U32 colour) {
        vertex(pos[0], pos[1], pos[2], colour);
    }
    inline void vertex(const F32* pos, U32 colour, const F32* uv) {
        vertex(pos[0], pos[1], pos[2], colour);
    }
    inline void vertex(const F32 x, const F32 y, const F32 z, U32 colour,
                       const F32 u, const F32 v) {
        vertex(x, y, z, colour);
    }

    GenericDrawCommand toDrawCommand() const;

   private:
    GFXDevice& _context;
    PrimitiveType _primType;
    IMPrimitive* _primitive;
    size_t _navMeshStateBlockHash;
    U32 _vertCount;
    U32 _colour;
    bool _overrideColour;
    bool _dirty;
    bool _paused;
};
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide

#endif
