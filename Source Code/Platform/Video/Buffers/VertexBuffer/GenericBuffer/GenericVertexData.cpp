#include "Headers/GenericVertexData.h"

namespace Divide {

GenericVertexData::GenericVertexData(GFXDevice& context, const U32 ringBufferLength)
  : VertexDataInterface(context),
    RingBuffer(ringBufferLength)
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

AttributeDescriptor& GenericVertexData::getDrawAttribDescriptor(U32 attribIndex) {
    AttributeDescriptor& desc = _attributeMapDraw[attribIndex];
    desc.attribIndex(attribIndex);
    return desc;
}

AttributeDescriptor& GenericVertexData::getFdbkAttribDescriptor(U32 attribIndex) {
    AttributeDescriptor& desc = _attributeMapFdbk[attribIndex];
    desc.attribIndex(attribIndex);
    return desc;
}

}; //namespace Divide