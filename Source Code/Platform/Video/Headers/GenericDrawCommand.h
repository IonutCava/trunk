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

#ifndef _GENERIC_DRAW_COMMAND_H_
#define _GENERIC_DRAW_COMMAND_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class VertexDataInterface;
enum class PrimitiveType : U32;
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

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
        COUNT = 5
    };

private:
    // state hash is not size_t to avoid any platform specific awkward typedefing
    IndirectDrawCommand _cmd;           // 60 bytes
    ShaderProgram* _shaderProgram;      // 40 bytes
    VertexDataInterface* _sourceBuffer; // 32 bytes
    U64 _stateHash;                     // 24 bytes
    PrimitiveType _type;                // 16 bytes
    U32 _commandOffset;                 // 12 bytes
    U32 _renderOptions;                 // 8  bytes
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

    inline void LoD(U8 lod)                                           { _lodIndex = lod; }
    inline void drawCount(U16 count)                                  { _drawCount = count; }
    inline void drawToBuffer(U8 index)                                { _drawToBuffer = index; }
    inline void commandOffset(U32 offset)                             { _commandOffset = offset; }
    inline void stateHash(size_t hashValue)                           { _stateHash = static_cast<U64>(hashValue); }
    inline void primitiveType(PrimitiveType type)                     { _type = type; }
    inline void shaderProgram(const ShaderProgram_ptr& program)       { _shaderProgram = program.get(); }
    inline void sourceBuffer(VertexDataInterface* const sourceBuffer) { _sourceBuffer = sourceBuffer; }

    inline U8 LoD()                            const { return _lodIndex; }
    inline U16 drawCount()                     const { return _drawCount; }
    inline size_t stateHash()                  const { return static_cast<size_t>(_stateHash); }
    inline U8  drawToBuffer()                  const { return _drawToBuffer; }
    inline U32 commandOffset()                 const { return _commandOffset; }
    inline IndirectDrawCommand& cmd()                { return _cmd; }
    inline PrimitiveType primitiveType()       const { return _type; }
    inline ShaderProgram* shaderProgram()      const { return _shaderProgram; }
    inline VertexDataInterface* sourceBuffer() const { return _sourceBuffer; }

    inline const IndirectDrawCommand& cmd()    const { return _cmd; }
};

typedef vectorImpl<GenericDrawCommand> GenericDrawCommands;
}; //namespace Divide

#endif //_GENERIC_DRAW_COMMAND_H_
