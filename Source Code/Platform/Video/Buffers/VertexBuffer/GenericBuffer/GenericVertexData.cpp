#include "stdafx.h"

#include "Headers/GenericVertexData.h"

namespace Divide {

GenericVertexData::GenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
  : VertexDataInterface(context),
    RingBuffer(ringBufferLength),
    _name(name)
{
    _doubleBufferedQuery = true;
}

GenericVertexData::~GenericVertexData()
{
    _attributeMapDraw.clear();
    _attributeMapFdbk.clear();
}

void GenericVertexData::toggleDoubleBufferedQueries(const bool state) {
    _doubleBufferedQuery = state;
}

AttributeDescriptor& GenericVertexData::attribDescriptor(U32 attribIndex) {
    AttributeDescriptor& desc = _attributeMapDraw[attribIndex];
    desc.attribIndex(attribIndex);
    return desc;
}

AttributeDescriptor& GenericVertexData::fdbkAttribDescriptor(U32 attribIndex) {
    AttributeDescriptor& desc = _attributeMapFdbk[attribIndex];
    desc.attribIndex(attribIndex);
    return desc;
}

}; //namespace Divide