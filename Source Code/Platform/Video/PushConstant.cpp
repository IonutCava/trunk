#include "stdafx.h"

#include "Headers/PushConstant.h"

namespace Divide {
namespace GFX {
    void PushConstant::clear() {
        _buffer.resize(0);
        _type = PushConstantType::COUNT;
        _flag = false;
    }

}; //namespace GFX
}; //namespace Divide