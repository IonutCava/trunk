#include "Headers/RenderPackage.h"

#include "Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
// ToDo: This will return false if the number of shader buffers or number of
// textures does not match between the 2 packages although said buffers/textures
// might be compatible and batchable between the two.
// Obviously, this is not desirable. Fix it! -Ionut
bool RenderPackage::isCompatible(const RenderPackage& other) const {
    vectorAlg::vecSize bufferCount = other._shaderBuffers.size();
    if (_shaderBuffers.size() == bufferCount) {
        for (vectorAlg::vecSize i = 0; i < bufferCount; i++) {
            const ShaderBufferList::value_type& buffer1 = _shaderBuffers[i];
            const ShaderBufferList::value_type& buffer2 = other._shaderBuffers[i];

            I64 guid1 = buffer1._buffer ? buffer1._buffer->getGUID() : -1;
            I64 guid2 = buffer2._buffer ? buffer2._buffer->getGUID() : -1;
            if (buffer1._slot != buffer2._slot ||
                buffer1._range != buffer2._range ||
                guid1 != guid2)
            {
                return false;
            }
        }
    } else {
        return false;
    }

    const vectorImpl<TextureData>& textures = _textureData.textures();
    const vectorImpl<TextureData>& otherTextures = other._textureData.textures();
    vectorAlg::vecSize textureCount = otherTextures.size();
    if (textures.size() == textureCount) {
        U64 handle1 = 0, handle2 = 0;
        for (vectorAlg::vecSize i = 0; i < textureCount; ++i) {
            const TextureData& data1 = textures[i];
            const TextureData& data2 = otherTextures[i];
            data1.getHandle(handle1);
            data2.getHandle(handle2);
            if (handle1 != handle2 ||
                data1._samplerHash != data2._samplerHash ||
                data1._textureType != data2._textureType) {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}
void RenderPackage::clear() {
    _shaderBuffers.resize(0);
    _drawCommands.resize(0);
    _textureData.clear(false);
}

void RenderPackage::set(const RenderPackage& other) {
#if 0
    size_t shaderBufferSize = other._shaderBuffers.size();
    size_t textureDataSize = other._textureData.size();
    size_t drawCommandSize = other._drawCommands.size();

    _shaderBuffers.resize(shaderBufferSize);
    _textureData.resize(textureDataSize);
    _drawCommands.resize(drawCommandSize);
    for (size_t i = 0; i < shaderBufferSize; ++i) {
        _shaderBuffers.at(i).set(other._shaderBuffers.at(i));
    }

    for (size_t i = 0; i < textureDataSize; ++i) {
        _textureData.at(i).set(other._textureData.at(i));
    }

    for (size_t i = 0; i < drawCommandSize; ++i) {
        _drawCommands.at(i).set(other._drawCommands.at(i));
    }
#else
    _shaderBuffers = other._shaderBuffers;
    _textureData = other._textureData;
    _drawCommands = other._drawCommands;
#endif
}



void RenderPackageQueue::clear() {
    for (U32 idx = 0; idx < _currentCount; ++idx) {
        //_packages[idx].clear();
    }
    _currentCount = 0;
}

U32 RenderPackageQueue::size() const {
    return _currentCount;
}

bool RenderPackageQueue::locked() const {
    return _locked;
}

bool RenderPackageQueue::empty() const {
    return _currentCount == 0;
}

const RenderPackage& RenderPackageQueue::getPackage(U32 idx) const {
    assert(idx < Config::MAX_VISIBLE_NODES);
    return _packages[idx];
}

RenderPackage& RenderPackageQueue::getPackage(U32 idx) {
    assert(idx < Config::MAX_VISIBLE_NODES);
    return _packages[idx];
}

RenderPackage& RenderPackageQueue::back() {
    return _packages[std::max(to_int(_currentCount) - 1, 0)];
}

bool RenderPackageQueue::push_back(const RenderPackage& package) {
    if (_currentCount <= Config::MAX_VISIBLE_NODES) {
        _packages[_currentCount++].set(package);
        return true;
    }
    return false;
}

void RenderPackageQueue::reserve(U16 size) {
    _packages.resize(size);
}

void RenderPackageQueue::lock() {
    _locked = true;
}

void RenderPackageQueue::unlock() {
    _locked = false;
}

}; //namespace Divide