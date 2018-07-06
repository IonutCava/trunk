/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _GENERIC_DRAW_COMMAND_H_
#define _GENERIC_DRAW_COMMAND_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/Pipeline.h"

namespace Divide {

class VertexDataInterface;
enum class PrimitiveType : U32;

namespace GenericDrawCommandResults {
    struct QueryResult {
        U64 _primitivesGenerated = 0U;
        U32 _samplesPassed = 0U;
        U32 _anySamplesPassed = 0U;
    };

    extern hashMapImpl<I64, QueryResult> g_queryResults;
};

struct IndirectDrawCommand {
    IndirectDrawCommand();
    void set(const IndirectDrawCommand& other);
    bool operator==(const IndirectDrawCommand &other) const;

    U32 indexCount;    // 4  bytes
    U32 primCount;     // 8  bytes
    U32 firstIndex;    // 12 bytes
    U32 baseVertex;    // 16 bytes
    U32 baseInstance;  // 20 bytes
};

class GenericDrawCommand {
public:
    enum class RenderOptions : U32 {
        RENDER_GEOMETRY = toBit(1),
        RENDER_WIREFRAME = toBit(2),
        RENDER_BOUNDS_AABB = toBit(3),
        RENDER_BOUNDS_SPHERE = toBit(4),
        RENDER_NO_RASTERIZE = toBit(5),
        RENDER_INDIRECT = toBit(6),
        RENDER_TESSELLATED = toBit(7),
        QUERY_PRIMITIVE_COUNT = toBit(8),
        QUERY_SAMPLE_COUNT = toBit(9),
        QUERY_ANY_SAMPLE_RENDERED = toBit(10),
        COUNT = 9
    };

private:
    // state hash is not size_t to avoid any platform specific awkward typedefing
    IndirectDrawCommand _cmd;           // 48 bytes
    VertexDataInterface* _sourceBuffer; // 28 bytes
    PrimitiveType _type;                // 20 bytes
    U32 _commandOffset;                 // 16 bytes
    U32 _renderOptions;                 // 12 bytes
    U32 _patchVertexCount;              // 8  bytes
    U16 _drawCount;                     // 4  bytes
    U8  _drawToBuffer;                  // 2  bytes
    U8  _lodIndex;                      // 1  bytes

public:
    GenericDrawCommand();
    GenericDrawCommand(PrimitiveType type,
                       U32 firstIndex,
                       U32 indexCount,
                       U32 primCount = 1);

    void reset();
    void set(const GenericDrawCommand& base);
    bool compatible(const GenericDrawCommand& other) const;

    void renderMask(U32 mask);
    bool isEnabledOption(RenderOptions option) const;
    void enableOption(RenderOptions option);
    void disableOption(RenderOptions option);
    void toggleOption(RenderOptions option);
    void toggleOption(RenderOptions option, const bool state);

    inline U32  renderMask() const { return _renderOptions; }

    inline void LoD(U8 lod)                                           { _lodIndex = lod; }
    inline void drawCount(U16 count)                                  { _drawCount = count; }
    inline void drawToBuffer(U8 index)                                { _drawToBuffer = index; }
    inline void commandOffset(U32 offset)                             { _commandOffset = offset; }
    inline void patchVertexCount(U32 vertexCount)                     { _patchVertexCount = vertexCount; }
    inline void primitiveType(PrimitiveType type)                     { _type = type; }
    inline void sourceBuffer(VertexDataInterface* sourceBuffer)       { _sourceBuffer = sourceBuffer; }

    inline U8 LoD()                            const { return _lodIndex; }
    inline U16 drawCount()                     const { return _drawCount; }
    inline U8  drawToBuffer()                  const { return _drawToBuffer; }
    inline U32 commandOffset()                 const { return _commandOffset; }
    inline U32 patchVertexCount()              const { return _patchVertexCount; }
    inline IndirectDrawCommand& cmd()                { return _cmd; }
    inline PrimitiveType primitiveType()       const { return _type; }
    inline VertexDataInterface* sourceBuffer() const { return _sourceBuffer; }
    inline const IndirectDrawCommand& cmd()    const { return _cmd; }
};

typedef vectorImpl<GenericDrawCommand> GenericDrawCommands;
}; //namespace Divide

#endif //_GENERIC_DRAW_COMMAND_H_
