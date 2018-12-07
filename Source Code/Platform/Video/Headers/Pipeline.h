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
#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "PushConstants.h"
#include "DescriptorSets.h"
#include "Core/Headers/Hashable.h"
#include "Utility/Headers/TextLabel.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

typedef std::array<vector<U32>, to_base(ShaderType::COUNT)> ShaderFunctions;

enum class MemoryBarrierType : U8 {
    BUFFER = toBit(1),
    TEXTURE = toBit(2),
    RENDER_TARGET = toBit(3),
    TRANSFORM_FEEDBACK = toBit(4),
    COUNTER = toBit(5),
    QUERY = toBit(6),
    SHADER_BUFFER = toBit(7),
    COUNT = 7
};

class PipelineDescriptor : public Hashable {
  public:
    size_t getHash() const override;
    bool operator==(const PipelineDescriptor &other) const;
    bool operator!=(const PipelineDescriptor &other) const;

    U8 _multiSampleCount = 0;
    U32 _shaderProgramHandle = 0;
    size_t _stateHash = 0;
    ShaderFunctions _shaderFunctions;
}; //struct PipelineDescriptor

class Pipeline {
public:
    Pipeline(const PipelineDescriptor& descriptor);

    inline U32 shaderProgramHandle() const {
        return _descriptor._shaderProgramHandle;
    }

    inline const PipelineDescriptor& descriptor() const {
        return _descriptor;
    }

    inline size_t stateHash() const {
        return _descriptor._stateHash;
    }

    inline U8 multiSampleCount() const{
        return _descriptor._multiSampleCount;
    }

    inline const ShaderFunctions& shaderFunctions() const {
        return _descriptor._shaderFunctions;
    }

    inline size_t getHash() const {
        return _cachedHash;
    }

    bool operator==(const Pipeline &other) const;
    bool operator!=(const Pipeline &other) const;

private: //data
    size_t _cachedHash = 0;
    PipelineDescriptor _descriptor;
    

}; //class Pipeline

}; //namespace Divide

#endif //_PIPELINE_H_

