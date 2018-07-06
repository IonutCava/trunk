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

PushConstants::PushConstants(const vector<GFX::PushConstant>& data)
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

bool PushConstants::merge(const PushConstants& other) {
    const vector<GFX::PushConstant>& ourConstants = _data;
    const vector<GFX::PushConstant>& otherConstants = other._data;

    // Check stage
    for (const GFX::PushConstant& ourConstant : ourConstants) {
        for (const GFX::PushConstant& otherConstant : otherConstants) {
            // If we have the same binding, but different data, merging isn't possible
            if (ourConstant._bindingHash == otherConstant._bindingHash &&
                 (ourConstant._flag != otherConstant._flag ||
                  ourConstant._buffer != otherConstant._buffer))
            {
                return false;
            }
        }
    }

    // Merge stage
    insert_unique(_data, other._data);

    return true;
}
}; //namespace Divide
