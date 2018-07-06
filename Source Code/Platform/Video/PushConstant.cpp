#include "stdafx.h"

#include "Headers/PushConstant.h"

namespace Divide {
namespace GFX {
    PushConstant::~PushConstant()
    {
        //clear();
    }


    PushConstant::PushConstant(const PushConstant& other)
    {
        assign(other);
    }

    PushConstant& PushConstant::operator=(const PushConstant& other) {
        return assign(other);
    }

    PushConstant& PushConstant::assign(const PushConstant& other) {
        _binding = other._binding;
        _type = other._type;
        _flag = other._flag;
        _buffer = other._buffer;
        return *this;
    }

    void PushConstant::clear() {
        _buffer.clear();
        _binding.clear();
        _type = PushConstantType::COUNT;
        _flag = false;
    }

}; //namespace GFX
}; //namespace Divide