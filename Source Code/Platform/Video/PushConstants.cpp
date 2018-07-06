#include "stdafx.h"

#include "Headers/PushConstants.h"

namespace Divide {
PushConstants::PushConstants()
{
    _data.reserve(3);
}

PushConstants::PushConstants(const GFX::PushConstant& constant)
    : _data({constant})
{
}

PushConstants::PushConstants(const vectorImpl<GFX::PushConstant>& data)
    : _data(data)
{
}

PushConstants::~PushConstants()
{
    //clear();
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
    for (GFX::PushConstant& iter : _data) {
        if (iter._bindingHash == constant._bindingHash) {
            iter.assign(constant);
            return;
        }
    }

    _data.emplace_back(constant);
}

void PushConstants::clear() {
    _data.clear();
}

}; //namespace Divide
