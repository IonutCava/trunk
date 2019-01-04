#include "stdafx.h"

#include "Headers/PushConstant.h"

namespace Divide {
namespace GFX {
    void PushConstant::clear() {
        _buffer.clear();
        _binding.clear();
        _type = PushConstantType::COUNT;
        _flag = false;
    }

}; //namespace GFX
}; //namespace Divide