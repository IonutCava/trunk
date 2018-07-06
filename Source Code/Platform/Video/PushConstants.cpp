#include "stdafx.h"

#include "Headers/PushConstants.h"
#include "Headers/PushConstantPool.h"

namespace Divide {
PushConstants::PushConstants()
{
}

PushConstants::PushConstants(const GFX::PushConstant& constant)
{
    set(constant._binding,
        constant._type,
        constant._values,
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
{
    for (const hashMapImpl<U64, GFX::PushConstant*>::value_type& it : other._data) {
        if (it.second != nullptr) {
            _data[it.first] = GFX::allocatePushConstant(*it.second);
        }
    }
}

PushConstants& PushConstants::operator=(const PushConstants& other) {
    for (const hashMapImpl<U64, GFX::PushConstant*>::value_type& it : other._data) {
        if (it.second != nullptr) {
            _data[it.first] = GFX::allocatePushConstant(*it.second);
        }
    }
    return *this;
}

void PushConstants::set(const GFX::PushConstant& constant) {
    set(constant._binding,
        constant._type,
        constant._values,
        constant._flag);
}

void PushConstants::clear() {
    for (hashMapImpl<U64, GFX::PushConstant*>::value_type& it : _data) {
        GFX::deallocatePushConstant(it.second);
    }

    _data.clear();
}
}; //namespace Divide
