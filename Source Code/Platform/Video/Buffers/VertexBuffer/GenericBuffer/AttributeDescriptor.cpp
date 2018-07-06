#include "Headers/AttributeDescriptor.h"

namespace Divide {

AttributeDescriptor::AttributeDescriptor()
  : _index(0),
    _divisor(0),
    _parentBuffer(0),
    _componentsPerElement(0),
    _elementCountOffset(0),
    _wasSet(false),
    _dirty(false),
    _normalized(false),
    _stride(0),
    _type(GFXDataFormat::UNSIGNED_INT)
{
}

AttributeDescriptor::~AttributeDescriptor()
{
}

void AttributeDescriptor::set(U32 bufferIndex, U32 instanceDivisor, U32 componentsPerElement,
                              bool normalized, U32 elementCountOffset, GFXDataFormat dataType) {
    this->bufferIndex(bufferIndex);
    this->instanceDivisor(instanceDivisor);
    this->componentsPerElement(componentsPerElement);
    this->normalized(normalized);
    this->offset(elementCountOffset);
    this->dataType(dataType);
}

void AttributeDescriptor::stride(size_t stride) {
    _stride = stride;
    _dirty = true;
    _wasSet = false;
}

void AttributeDescriptor::attribIndex(U32 index) {
    _index = index;
    _dirty = true;
    _wasSet = false;
}

void AttributeDescriptor::offset(U32 elementCountOffset) {
    _elementCountOffset = elementCountOffset;
    _dirty = true;
}

void AttributeDescriptor::bufferIndex(U32 bufferIndex) {
    _parentBuffer = bufferIndex;
    _dirty = true;
    _wasSet = false;
}

void AttributeDescriptor::instanceDivisor(U32 instanceDivisor) {
    _divisor = instanceDivisor;
    _dirty = true;
}

void AttributeDescriptor::componentsPerElement(U32 componentsPerElement) {
    _componentsPerElement = componentsPerElement;
    _dirty = true;
}

void AttributeDescriptor::normalized(bool normalized) {
    _normalized = normalized;
    _dirty = true;
}

void AttributeDescriptor::dataType(GFXDataFormat type) {
    _type = type;
    _dirty = true;
}

size_t AttributeDescriptor::stride() const {
    if (_stride == 0) {
        return componentsPerElement() * dataSizeForType(dataType());
    }

    return _stride;
}

void AttributeDescriptor::wasSet(bool wasSet) {
    _wasSet = wasSet;
    if (!_wasSet) {
        _dirty = true;
    }
}

void AttributeDescriptor::clean() {
    _dirty = false;
}

}; //namespace Divide