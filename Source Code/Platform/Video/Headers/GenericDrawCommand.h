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
#include "Core/Headers/ObjectPool.h"

namespace Divide {

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
    CONVERT_TO_INDIRECT = toBit(5),
    QUERY_PRIMITIVE_COUNT = toBit(6),
    QUERY_SAMPLE_COUNT = toBit(7),
    QUERY_ANY_SAMPLE_RENDERED = toBit(8),
    COUNT = 8
};

#pragma pack(push, 1)
struct GenericDrawCommand {
    IndirectDrawCommand _cmd = {};                                   // 32 bytes
    PoolHandle _sourceBuffer = {};                                   // 12 bytes
    I24 _commandOffset = {0};                                        // 9  bytes
    U16 _renderOptions = to_base(CmdRenderOptions::RENDER_GEOMETRY); // 6  bytes
    U16 _drawCount = 1;                                              // 4  bytes
    U8  _bufferIndex  = 0;                                           // 2  bytes
    PrimitiveType _primitiveType = PrimitiveType::TRIANGLE_STRIP;    // 1  bytes
};
#pragma pack(pop)

bool isEnabledOption(const GenericDrawCommand& cmd, CmdRenderOptions option) noexcept;
void enableOption(GenericDrawCommand& cmd, CmdRenderOptions option) noexcept;
void disableOption(GenericDrawCommand& cmd, CmdRenderOptions option) noexcept;
void toggleOption(GenericDrawCommand& cmd, CmdRenderOptions option) noexcept;
void setOption(GenericDrawCommand& cmd, CmdRenderOptions option, const bool state) noexcept;
void enableOptions(GenericDrawCommand& cmd, U16 optionsMask) noexcept;
void disableOptions(GenericDrawCommand& cmd, U16 optionsMask) noexcept;

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs) noexcept;

}; //namespace Divide

#endif //_GENERIC_DRAW_COMMAND_H_

#include "GenericDrawCommand.inl"