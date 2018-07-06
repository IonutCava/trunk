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

        //All of this because _values = other._values doesn't work properly
        //with AnyParam and causes a heap corruption on destruction -Ionut
        _values.resize(0);
        _values.reserve(other._values.size());
        std::copy(std::cbegin(other._values),
                  std::cend(other._values),
                  std::back_inserter(_values));
        shrinkToFit(_values);
        return *this;
    }

    void PushConstant::clear() {
        _values.clear();
        _binding.clear();
        _type = PushConstantType::COUNT;
        _flag = false;
    }

}; //namespace GFX
}; //namespace Divide