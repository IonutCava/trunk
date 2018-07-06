#include "stdafx.h"

#include "Headers/PushConstants.h"

namespace Divide {
PushConstants::PushConstants()
{
}

PushConstants::PushConstants(const GFX::PushConstant& constant)
{
    set(constant._binding,
        constant._type,
        constant._buffer,
        constant._flag);
}

PushConstants::PushConstants(const vectorImpl<GFX::PushConstant>& data)
{
    for (const GFX::PushConstant& constant : data) {
        set(constant);
    }
}

PushConstants::~PushConstants()
{
    clear();
}

PushConstants::PushConstants(const PushConstants& other) 
    : _data(other._data)
{
}

PushConstants& PushConstants::operator=(const PushConstants& other) {
    _data = other._data;
    return *this;
}

void PushConstants::set(const GFX::PushConstant& constant) {
    set(constant._binding,
        constant._type,
        constant._buffer,
        constant._flag);
}

void PushConstants::clear() {
    _data.clear();
}

}; //namespace Divide
