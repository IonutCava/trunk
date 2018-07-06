/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _GENERIC_DRAW_COMMAND_H_
#define _GENERIC_DRAW_COMMAND_H_

#include "Pipeline.h"
#include "Platform/Headers/PlatformDefines.h"

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
        RENDER_NO_RASTERIZE = toBit(3),
        RENDER_INDIRECT = toBit(4),
        RENDER_TESSELLATED = toBit(5),
        QUERY_PRIMITIVE_COUNT = toBit(6),
        QUERY_SAMPLE_COUNT = toBit(7),
        QUERY_ANY_SAMPLE_RENDERED = toBit(8),
        COUNT = 8
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

    GenericDrawCommand(const GenericDrawCommand& other);

    const GenericDrawCommand& operator= (const GenericDrawCommand& other);

    void reset();
    bool compatible(const GenericDrawCommand& other) const;

    bool isEnabledOption(RenderOptions option) const;
    void enableOption(RenderOptions option);
    void disableOption(RenderOptions option);
    void toggleOption(RenderOptions option);
    void toggleOption(RenderOptions option, const bool state);

    inline U32  renderMask() const;

    inline void LoD(U8 lod);
    inline void drawCount(U16 count);
    inline void drawToBuffer(U8 index);
    inline void commandOffset(U32 offset);
    inline void patchVertexCount(U32 vertexCount);
    inline void primitiveType(PrimitiveType type);
    inline void sourceBuffer(VertexDataInterface* sourceBuffer);

    inline U8 LoD() const;
    inline U16 drawCount() const;
    inline U8  drawToBuffer() const;
    inline U32 commandOffset() const;
    inline U32 patchVertexCount() const;
    inline IndirectDrawCommand& cmd();
    inline PrimitiveType primitiveType() const;
    inline VertexDataInterface* sourceBuffer() const;
    inline const IndirectDrawCommand& cmd() const;
};

}; //namespace Divide

#endif //_GENERIC_DRAW_COMMAND_H_

#include "GenericDrawCommand.inl"