#include "config.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

    ShaderBuffer::ShaderBuffer(GFXDevice& context,
                               const U32 ringBufferLength,
                               bool unbound,
                               bool persistentMapped,
                               BufferUpdateFrequency frequency)
                                                      : GraphicsResource(context),
                                                        RingBuffer(ringBufferLength),
                                                        GUIDWrapper(),
                                                        _sizeFactor(1),
                                                        _bufferSize(0),
                                                        _primitiveSize(0),
                                                        _primitiveCount(0),
                                                        _alignmentRequirement(0),
                                                        _frequency(frequency),
                                                        _unbound(unbound),
                                                        _persistentMapped(persistentMapped && !Config::Profile::DISABLE_PERSISTENT_BUFFER)
    {
    }

    ShaderBuffer::~ShaderBuffer()
    {
    }

    void ShaderBuffer::create(U32 primitiveCount, ptrdiff_t primitiveSize, U32 sizeFactor) {
        _primitiveCount = primitiveCount;
        _primitiveSize = primitiveSize;
        _bufferSize = primitiveSize * primitiveCount;
        _sizeFactor = sizeFactor;
        assert(_bufferSize > 0 && "ShaderBuffer::Create error: Invalid buffer size!");
    }

    void ShaderBuffer::setData(const bufferPtr data) {
        assert(_bufferSize > 0 && "ShaderBuffer::SetData error: Invalid buffer size!");
        updateData(0, _primitiveCount, data);
    }

}; //namespace Divide;