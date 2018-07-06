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

#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "PushConstants.h"
#include "DescriptorSets.h"
#include "Utility/Headers/TextLabel.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

enum class MemoryBarrierType : U32 {
    BUFFER = 0,
    TEXTURE = 1,
    RENDER_TARGET = 2,
    TRANSFORM_FEEDBACK = 3,
    COUNTER = 4,
    QUERY = 5,
    SHADER_BUFFER = 6,
    ALL = 7,
    COUNT
};

struct ComputeParams {
    MemoryBarrierType _barrierType = MemoryBarrierType::COUNT;
    vec3<U32> _groupSize;
};

typedef hashMapImpl<ShaderType, vectorImpl<U32>> ShaderFunctions;

struct PipelineDescriptor {
    U8 _multiSampleCount = 0;
    size_t _stateHash = 0;
    ShaderProgram_wptr _shaderProgram;
    ShaderFunctions _shaderFunctions;
}; //struct PipelineDescriptor

class Pipeline {
public:
    Pipeline();
    Pipeline(const PipelineDescriptor& descriptor);
    ~Pipeline();

    void fromDescriptor(const PipelineDescriptor& descriptor);
    PipelineDescriptor toDescriptor() const;

    inline ShaderProgram* shaderProgram() const {
        return _shaderProgram.expired() ? nullptr : _shaderProgram.lock().get();
    }

    inline size_t stateHash() const {
        return _stateHash;
    }

    inline U8 multiSampleCount() const{
        return _multiSampleCount;
    }


    inline const ShaderFunctions& shaderFunctions() const {
        return _shaderFunctions;
    }

    bool operator==(const Pipeline &other) const;
    bool operator!=(const Pipeline &other) const;

    size_t getHash() const;

private: //data
    size_t _stateHash;
    U8 _multiSampleCount;
    ShaderProgram_wptr _shaderProgram;
    ShaderFunctions _shaderFunctions;

}; //class Pipeline

}; //namespace Divide

#endif //_PIPELINE_H_

