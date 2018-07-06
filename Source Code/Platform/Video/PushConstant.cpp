#include "stdafx.h"

#include "Headers/PushConstant.h"

namespace Divide {
namespace GFX {
    PushConstant::~PushConstant()
    {
    }

    void PushConstant::clear() {
        _buffer.clear();
        _binding.clear();
        _type = PushConstantType::COUNT;
        _flag = false;
    }

}; //namespace GFX
}; //namespace Divide