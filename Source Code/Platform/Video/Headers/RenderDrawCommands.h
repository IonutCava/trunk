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

#ifndef _RENDER_DRAW_COMMANDS_H_
#define _RENDER_DRAW_COMMANDS_H_

#include "TextureData.h"
#include "GenericDrawCommand.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

enum class PushConstantType : U8 {
    BOOL = 0,
    INT,
    UINT,
    FLOAT,
    DOUBLE,
    //BVEC2, use vec2<I32>(1/0, 1/0)
    //BVEC3, use vec3<I32>(1/0, 1/0)
    //BVEC4, use vec4<I32>(1/0, 1/0)
    IVEC2,
    IVEC3,
    IVEC4,
    UVEC2,
    UVEC3,
    UVEC4,
    VEC2,
    VEC3,
    VEC4,
    DVEC2,
    DVEC3,
    DVEC4,
    MAT2,
    MAT3,
    MAT4,
    //MAT_N_x_M,
    COUNT
};
    
struct PushConstant {
    //I32              _binding = -1;
    stringImplFast   _binding;
    PushConstantType _type = PushConstantType::COUNT;
    vectorImplFast<AnyParam> _values;
    union {
        bool _flag = false;
        bool _transpose;
    };
};

struct PushConstants {
    vectorImpl<PushConstant> _data;
};

class ShaderBuffer;
struct ShaderBufferBindCmd {
    explicit ShaderBufferBindCmd(ShaderBuffer* buffer,
                                 ShaderBufferLocation binding,
                                 vec2<U32> dataRange)
        : _buffer(buffer),
          _binding(binding),
          _dataRange(dataRange)
    {
    }

    ShaderBuffer* _buffer;
    ShaderBufferLocation _binding;
    vec2<U32> _dataRange;
};

struct ShaderBufferBinding {
    ShaderBufferLocation _slot;
    ShaderBuffer* _buffer;
    vec2<U32>    _range;

    ShaderBufferBinding()
        : ShaderBufferBinding(ShaderBufferLocation::COUNT,
            nullptr,
            vec2<U32>(0, 0))
    {
    }

    ShaderBufferBinding(ShaderBufferLocation slot,
        ShaderBuffer* buffer,
        const vec2<U32>& range)
        : _slot(slot),
        _buffer(buffer),
        _range(range)
    {
    }

    inline void set(const ShaderBufferBinding& other) {
        set(other._slot, other._buffer, other._range);
    }

    inline void set(ShaderBufferLocation slot,
                    ShaderBuffer* buffer,
                    const vec2<U32>& range) {
        _slot = slot;
        _buffer = buffer;
        _range.set(range);
    }
};

struct ClipPlaneList {
    ClipPlaneList(U32 size, const Plane<F32>& defaultValue)
        : _planes(size, defaultValue),
          _active(size, false)
    {
    }

    void resize(U32 size, const Plane<F32>& defaultValue) {
        _planes.resize(size, defaultValue);
        _active.resize(size, false);
    }

    PlaneList _planes;
    vectorImpl<bool> _active;
};

typedef vectorImpl<ShaderBufferBinding> ShaderBufferList;

typedef std::array<IndirectDrawCommand, Config::MAX_VISIBLE_NODES> DrawCommandList;

struct RenderSubPassCmd {
    U8 _targetWriteLevel = 0;
    TextureDataContainer _textures;
    GenericDrawCommands  _commands;
    vectorImpl<vec4<I32>> _viewports;
    vectorImpl<ShaderBufferBindCmd> _shaderBuffers;
};

typedef vectorImpl<RenderSubPassCmd> RenderSubPassCmds;

struct RenderPassCmd {
    RenderTargetID _renderTarget;
    RTDrawDescriptor _renderTargetDescriptor;
    RenderSubPassCmds _subPassCmds;
};

typedef vectorImpl<RenderPassCmd> CommandBuffer;

}; //namespace Divide

#endif //_RENDER_DRAW_COMMANDS_H_
