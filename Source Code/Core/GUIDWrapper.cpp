#include "stdafx.h"

#include "Headers/GUIDWrapper.h"

namespace Divide {

int64_t GUIDWrapper::_idGenerator = 0;

I64 GUIDWrapper::generateGUID() {
    // Always start from 1 as we use IDs mainly as key values
    static std::atomic<I64> idGenerator{ 1 };
    return idGenerator++;
}
};  // namespace Divide
