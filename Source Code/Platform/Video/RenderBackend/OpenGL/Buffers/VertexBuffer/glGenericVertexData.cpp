#include "stdafx.h"

#include "Headers/glGenericVertexData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

glGenericVertexData::glGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
    : GenericVertexData(context, ringBufferLength)
{
    _indexBuffer = 0;
    _indexBufferSize = 0;
    _indexBufferUsage = GL_NONE;
    _smallIndices = false;
    _vertexArray = 0;
}

glGenericVertexData::~glGenericVertexData()
{
    if (!_bufferObjects.empty()) {
        for (U8 i = 0; i < _bufferObjects.size(); ++i) {
            MemoryManager::DELETE(_bufferObjects[i]);
        }
    }

    // Make sure we don't have any of our VAOs bound
    GL_API::getStateTracker().setActiveVAO(0);
    // Delete the rendering VAO
    if (_vertexArray > 0) {
        GL_API::s_vaoPool.deallocate(_vertexArray);
    }

    GLUtil::freeBuffer(_indexBuffer);
}

/// Create the specified number of buffers and queries and attach them to this
/// vertex data container
void glGenericVertexData::create(U8 numBuffers) {
    // Prevent double create
    assert(_bufferObjects.empty() && "glGenericVertexData error: create called with no buffers specified!");
    GL_API::s_vaoPool.allocate(1, &_vertexArray);
    // Create our buffer objects
    _bufferObjects.resize(numBuffers, nullptr);
}

/// Submit a draw command to the GPU using this object and the specified command
void glGenericVertexData::draw(const GenericDrawCommand& command) {
    bool useCmdBuffer = isEnabledOption(command, CmdRenderOptions::RENDER_INDIRECT);
    // Update buffer bindings
    setBufferBindings();
    // Update vertex attributes if needed (e.g. if offsets changed)
    setAttributes();

    // Delay this for as long as possible
    GL_API::getStateTracker().setActiveVAO(_vertexArray);
   
    // Submit the draw command
    GLUtil::submitRenderCommand(command, _indexBuffer > 0, useCmdBuffer, _smallIndices ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);

    vec_size bufferCount = _bufferObjects.size();
    for (vec_size i = 0; i < bufferCount; ++i) {
        glGenericBuffer* buffer = _bufferObjects[i];
        buffer->lockData(buffer->elementCount(), 0, queueIndex(), true);
    }
}

void glGenericVertexData::setIndexBuffer(const IndexBuffer& indices, BufferUpdateFrequency updateFrequency) {
    if (_indexBuffer != 0) {
        GLUtil::freeBuffer(_indexBuffer);
    }

    if (indices.count > 0) {
        _indexBufferUsage = 
            updateFrequency == BufferUpdateFrequency::ONCE
                             ? GL_STATIC_DRAW
                             : updateFrequency == BufferUpdateFrequency::OCASSIONAL
                                                ? GL_DYNAMIC_DRAW
                                                : GL_STREAM_DRAW;
        // Generate an "Index Buffer Object"
        _indexBufferSize = (GLuint)(indices.count * (indices.smallIndices ? sizeof(U16) : sizeof(U32)));
        _smallIndices = indices.smallIndices;

        GLUtil::createAndAllocBuffer(
            _indexBufferSize,
            _indexBufferUsage,
            _indexBuffer,
            indices.data,
            _name.empty() ? nullptr : (_name + "_index").c_str());
    }

    GL_API::getStateTracker().setActiveVAO(_vertexArray);
    GL_API::getStateTracker().setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
}

void glGenericVertexData::updateIndexBuffer(const IndexBuffer& indices) {
    DIVIDE_ASSERT(indices.count > 0, "glGenericVertexData::UpdateIndexBuffer error: Invalid index buffer data!");
    assert(_indexBuffer != 0 && "glGenericVertexData::UpdateIndexBuffer error: no valid index buffer found!");

    size_t elementSize = indices.smallIndices ? sizeof(GLushort) : sizeof(GLuint);

    if (indices.offsetCount == 0) {
        _indexBufferSize = (GLuint)(indices.count * elementSize);

        glInvalidateBufferData(_indexBuffer);
        glNamedBufferData(_indexBuffer,
                          _indexBufferSize,
                          indices.data,
                          _indexBufferUsage);
    } else {
        size_t size = (indices.count + indices.offsetCount) * elementSize;
        DIVIDE_ASSERT(size < _indexBufferSize);
        glInvalidateBufferSubData(_indexBuffer,
                                  indices.offsetCount * elementSize,
                                  indices.count * elementSize);
        glNamedBufferSubData(_indexBuffer,
                             indices.offsetCount * elementSize,
                             indices.count * elementSize,
                             indices.data);
    }

    GL_API::getStateTracker().setActiveVAO(_vertexArray);
    GL_API::getStateTracker().setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
}

/// Specify the structure and data of the given buffer
void glGenericVertexData::setBuffer(const SetBufferParams& params) {
    U32 buffer = params._buffer;

    // Make sure the buffer exists
    assert(buffer >= 0 && buffer < _bufferObjects.size() &&
           "glGenericVertexData error: set buffer called for invalid buffer index!");

    assert(_bufferObjects[buffer] == nullptr &&
           "glGenericVertexData::setBuffer : buffer re-purposing is not supported at the moment");

    BufferParams paramsOut = {};
    paramsOut._usage = GL_ARRAY_BUFFER;
    paramsOut._elementCount = params._elementCount;
    paramsOut._elementSizeInBytes = params._elementSize;
    paramsOut._frequency = params._updateFrequency;
    paramsOut._ringSizeFactor = params._useRingBuffer ? queueLength() : 1;
    paramsOut._data = params._data;
    paramsOut._name = _name.empty() ? nullptr : _name.c_str();
    paramsOut._unsynced = !params._sync;
    paramsOut._storageType = params._storageType;

    glGenericBuffer * tempBuffer = MemoryManager_NEW glGenericBuffer(_context, paramsOut);
    _bufferObjects[buffer] = tempBuffer;
}

/// Update the elementCount worth of data contained in the buffer starting from
/// elementCountOffset size offset
void glGenericVertexData::updateBuffer(U32 buffer,
                                       U32 elementCount,
                                       U32 elementCountOffset,
                                       const bufferPtr data) {
    _bufferObjects[buffer]->writeData(elementCount, elementCountOffset, queueIndex(), data);
}

void glGenericVertexData::setBufferBindOffset(U32 buffer, U32 elementCountOffset) {
    _bufferObjects[buffer]->setBindOffset(elementCountOffset);
}

void glGenericVertexData::setBufferBindings() {
    if (!_bufferObjects.empty()) {
        for (U32 i = 0; i < _bufferObjects.size(); ++i) {
            glGenericBuffer* buffer = _bufferObjects[i];
            GL_API::getStateTracker().bindActiveBuffer(_vertexArray,
                                                       i,
                                                       buffer->bufferHandle(),
                                                       buffer->getBindOffset(queueIndex()),
                                                       static_cast<GLsizei>(buffer->bufferImpl()->elementSize()));
        }
    }

}

/// Update the appropriate attributes 
void glGenericVertexData::setAttributes() {
    // Get the appropriate list of attributes and update them in turn
    for (attributeMap::value_type& it : _attributeMapDraw) {
        setAttributeInternal(it.second);
    }
}

/// Update internal attribute data
void glGenericVertexData::setAttributeInternal(AttributeDescriptor& descriptor) {
    // Early out if the attribute didn't change
    if (!descriptor.dirty()) {
        return;
    }

    // If the attribute wasn't activate until now, enable it
    if (!descriptor.wasSet()) {
        assert(descriptor.attribIndex() < to_U32(GL_API::s_maxAttribBindings) &&
               "GL Wrapper: insufficient number of attribute binding locations available on current hardware!");

        glEnableVertexArrayAttrib(_vertexArray, descriptor.attribIndex());
        glVertexArrayAttribBinding(_vertexArray, descriptor.attribIndex(), descriptor.bufferIndex());
        descriptor.wasSet(true);
    }

    // Update the attribute data
    GFXDataFormat format = descriptor.dataType();

    bool isIntegerType = format != GFXDataFormat::FLOAT_16 &&
                         format != GFXDataFormat::FLOAT_32;
    
    if (!isIntegerType || descriptor.normalized()) {
        glVertexArrayAttribFormat(_vertexArray,
                                  descriptor.attribIndex(),
                                  descriptor.componentsPerElement(),
                                  GLUtil::glDataFormat[to_U32(format)],
                                  descriptor.normalized() ? GL_TRUE : GL_FALSE,
                                  (GLuint)descriptor.strideInBytes());
    } else {
        glVertexArrayAttribIFormat(_vertexArray,
                                   descriptor.attribIndex(),
                                   descriptor.componentsPerElement(),
                                   GLUtil::glDataFormat[to_U32(format)],
                                   (GLuint)descriptor.strideInBytes());
    }

    glVertexArrayBindingDivisor(_vertexArray, descriptor.bufferIndex(), descriptor.instanceDivisor());

    // Inform the descriptor that the data was updated
    descriptor.clean();
}

};
