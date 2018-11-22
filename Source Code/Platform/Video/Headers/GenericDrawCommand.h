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
enum class PrimitiveType : U8;

namespace GenericDrawCommandResults {
    struct QueryResult {
        U64 _primitivesGenerated = 0U;
        U32 _samplesPassed = 0U;
        U32 _anySamplesPassed = 0U;
    };

    extern hashMap<I64, QueryResult> g_queryResults;
};

struct IndirectDrawCommand {
    U32 indexCount = 0;    // 4  bytes
    U32 primCount = 1;     // 8  bytes
    U32 firstIndex = 0;    // 12 bytes
    U32 baseVertex = 0;    // 16 bytes
    U32 baseInstance = 0;  // 20 bytes
};

enum class CmdRenderOptions : U16 {
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

struct GenericDrawCommand {
  public: //Data
    // state hash is not size_t to avoid any platform specific awkward typedefing
    IndirectDrawCommand _cmd;                                        // 45 bytes
    VertexDataInterface* _sourceBuffer = nullptr;                    // 25 bytes
    U32 _commandOffset = 0;                                          // 17 bytes
    U32 _renderOptions = to_base(CmdRenderOptions::RENDER_GEOMETRY); // 13 bytes
    U32 _patchVertexCount = 3;                                       // 9  bytes
    U16 _drawCount = 1;                                              // 5  bytes
    U8  _bufferIndex = 0;                                            // 3  bytes
    U8  _lodIndex = 0;                                               // 2  bytes
    PrimitiveType _primitiveType = PrimitiveType::COUNT;             // 1  bytes

  public: //Rule of five

    GenericDrawCommand();
    GenericDrawCommand(PrimitiveType type,
                       U32 firstIndex,
                       U32 indexCount,
                       U32 primCount = 1);
    ~GenericDrawCommand() = default;
    GenericDrawCommand(const GenericDrawCommand& other) = default;
    GenericDrawCommand& operator=(const GenericDrawCommand& other) = default;
    GenericDrawCommand(GenericDrawCommand&& other) = default;
    GenericDrawCommand& operator=(GenericDrawCommand&& other) = default;

    
};

bool isEnabledOption(const GenericDrawCommand& cmd, CmdRenderOptions option);
void enableOption(GenericDrawCommand& cmd, CmdRenderOptions option);
void disableOption(GenericDrawCommand& cmd, CmdRenderOptions option);
void toggleOption(GenericDrawCommand& cmd, CmdRenderOptions option);
void setOption(GenericDrawCommand& cmd, CmdRenderOptions option, const bool state);

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs);

}; //namespace Divide

#endif //_GENERIC_DRAW_COMMAND_H_

#include "GenericDrawCommand.inl"